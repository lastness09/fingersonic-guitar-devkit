// SPDX-License-Identifier: Apache-2.0
#include "GrooveRuntime.h"
#include "config.h"
#include "RingLedOutput.h"
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPR121.h>
#include <Preferences.h>
#include <driver/i2s_std.h>
#include <esp_ota_ops.h>
#include <atomic>
#include <math.h>
#include <string.h>

namespace groove {
namespace {
Engine engine;
View view; // Only the input task changes this state; hardware IO runs outside the view lock.
View publishedView;
SemaphoreHandle_t viewMutex;
QueueHandle_t commands, snapshots, notices;
struct Notice { char text[24]; };
TaskHandle_t audioHandle=nullptr, inputHandle=nullptr;
struct Snapshot { Song song; Meter meter; uint16_t cpu; uint32_t underruns, revision; };
std::atomic<bool> running{false}, audioReady{false}, freeze{false}, frozen{false}, restorePending{false};
std::atomic<uint32_t> drops{0};
std::atomic<uint32_t> submittedRevision{0};
std::atomic<int> storageRequest{0};
std::atomic<StartupStage> startupStage{BootLogo};
std::atomic<uint32_t> inputHeartbeat{0};
std::atomic<bool> audioFault{false}, ledInitRequested{false}, ledReady{false};
Song frozenSong, restoreSong;
Adafruit_MPR121 touch;
uint8_t touchAddress=0;
uint32_t bootAt=0, noticeUntil=0;

uint32_t crc32(const uint8_t* data,size_t length) {
  uint32_t crc=0xffffffff;
  while(length--) {
    crc^=*data++;
    for(int i=0;i<8;++i) crc=(crc>>1)^((crc&1)?0xedb88320:0);
  }
  return ~crc;
}
struct Saved { uint32_t magic, version; Song song; uint32_t crc; };
bool load(Song& song) {
  Preferences prefs;
  if(!prefs.begin("fsg-fm",true)) return false;
  Saved saved{};
  const bool size=prefs.getBytesLength("song")==sizeof(saved);
  if(size) prefs.getBytes("song",&saved,sizeof(saved));
  prefs.end();
  if(!size||saved.magic!=0x464d4752||saved.version!=3||saved.crc!=crc32(reinterpret_cast<const uint8_t*>(&saved.song),sizeof(Song))) return false;
  song=saved.song; sanitize(song); return true;
}
bool save(const Song& song) {
  Saved saved{}; saved.magic=0x464d4752; saved.version=3; saved.song=song;
  saved.crc=crc32(reinterpret_cast<const uint8_t*>(&saved.song),sizeof(Song));
  Preferences prefs;
  if(!prefs.begin("fsg-fm",false)) return false;
  const bool ok=prefs.putBytes("song",&saved,sizeof(saved))==sizeof(saved);
  prefs.end(); return ok;
}
void notice(const char* text) {
  strlcpy(view.notice,text,sizeof(view.notice)); noticeUntil=millis()+2000;
}
void publishView() {
  // Copy memory under the lock; perform hardware IO and initialization outside it.
  if(xSemaphoreTake(viewMutex,0)!=pdTRUE) return;
  publishedView=view;
  xSemaphoreGive(viewMutex);
}
void postNotice(const char* text) {
  Notice message{}; strlcpy(message.text,text,sizeof(message.text));
  if(notices) xQueueSend(notices,&message,0);
}
void send(Action action,int value=0,int track=0,int item=0,int tag=0,int gate=0) {
  Command command{action,uint8_t(track),uint8_t(item),uint8_t(tag),int16_t(value),uint16_t(gate)};
  command.revision=submittedRevision.load()+1;
  if(xQueueSend(commands,&command,0)!=pdTRUE) { ++drops; notice("INPUT QUEUE FULL"); return; }
  submittedRevision=command.revision;
  // Accumulate input targets immediately and accept only acknowledged snapshots.
  Song& s=view.song;
  switch(action) {
    case Action::Tempo: s.bpm=constrain(value,30,300); break;
    case Action::Swing: s.swing=constrain(value,0,60); break;
    case Action::Master: s.master=constrain(value,0,100); break;
    case Action::Echo: s.echo=constrain(value,0,60); break;
    case Action::Led: s.led=!!value; break;
    case Action::Preset: if(track>=Bass) s.patches[track-Bass]=preset(value); break;
    case Action::Param: if(track>=Bass) s.patches[track-Bass].values[item]=constrain(value,item==Ratio?1:0,parameterLimit(item)); break;
    case Action::TrackLevel: s.levels[track]=constrain(value,0,100); break;
    case Action::Mute: s.mute[track]^=1; break;
    case Action::ToggleStep: s.steps[s.bank][track][item].enabled^=1; break;
    case Action::StepNote: s.steps[s.bank][track][item].note=constrain(value,24,96); break;
    case Action::StepVelocity: s.steps[s.bank][track][item].velocity=constrain(value,1,127); break;
    case Action::Bank: if(view.meter.playing) view.meter.pendingBank=constrain(value,0,bankCount-1); else s.bank=constrain(value,0,bankCount-1); break;
    default: break;
  }
}
void audioTask(void*) {
  i2s_chan_config_t config=I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0,I2S_ROLE_MASTER);
  config.dma_desc_num=hw::dmaBuffers; config.dma_frame_num=hw::dmaFrames;
  config.auto_clear=true;
  i2s_chan_handle_t channel=nullptr;
  esp_err_t error=i2s_new_channel(&config,&channel,nullptr);
  i2s_std_config_t standard{};
  standard.clk_cfg=I2S_STD_CLK_DEFAULT_CONFIG(hw::sampleRate);
  standard.slot_cfg=I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT,I2S_SLOT_MODE_STEREO);
  standard.gpio_cfg.mclk=I2S_GPIO_UNUSED;
  standard.gpio_cfg.bclk=gpio_num_t(hw::i2sClock);
  standard.gpio_cfg.ws=gpio_num_t(hw::i2sWord);
  standard.gpio_cfg.dout=gpio_num_t(hw::i2sData);
  standard.gpio_cfg.din=I2S_GPIO_UNUSED;
  if(error==ESP_OK) error=i2s_channel_init_std_mode(channel,&standard);
  if(error!=ESP_OK) {
    audioFault=true;
    Serial.printf("[FM] I2S failed: %s\n",esp_err_to_name(error));
    if(channel) i2s_del_channel(channel);
    audioHandle=nullptr; vTaskDelete(nullptr); return;
  }
  int16_t pcm[hw::audioBlock*2]={};
  bool enabled=false, wasRunning=false;
  uint32_t lastSnapshot=0, underruns=0, revision=0;
  audioReady=true;
  Serial.println("[FM] Audio ready: 32000Hz, 8 FM voices + 3 drums");
  for(;;) {
    const bool active=running.load();
    if(freeze.load()) {
      if(!frozen.load()) {
        // Apply pending parameter commands before taking the save snapshot.
        Command pending;
        while(xQueueReceive(commands,&pending,0)==pdTRUE) { engine.command(pending); revision=pending.revision; }
        engine.command({Action::Panic});
        if(enabled) { i2s_channel_disable(channel); enabled=false; }
        frozenSong=engine.song(); frozen=true;
      }
      vTaskDelay(pdMS_TO_TICKS(5)); continue;
    }
    if(frozen.exchange(false)) {
      if(restorePending.exchange(false)) engine.restore(restoreSong);
      xQueueReset(commands); revision=submittedRevision.load(); wasRunning=false;
    }
    if(!active) {
      if(wasRunning) engine.command({Action::Panic});
      if(enabled) { i2s_channel_disable(channel); enabled=false; }
      wasRunning=false; vTaskDelay(pdMS_TO_TICKS(10)); continue;
    }
    if(!enabled) {
      memset(pcm,0,sizeof(pcm)); i2s_channel_enable(channel); enabled=true;
    }
    wasRunning=true;
    const uint32_t start=micros();
    Command command;
    // Bound commands per block so rapid input cannot consume the render budget.
    for(int i=0;i<16&&xQueueReceive(commands,&command,0)==pdTRUE;++i) { engine.command(command); revision=command.revision; }
    engine.render(pcm,hw::audioBlock);
    const uint32_t elapsed=micros()-start;
    if(elapsed>hw::audioBlock*1000000UL/hw::sampleRate) ++underruns;
    if(millis()-lastSnapshot>=32) {
      Snapshot snapshot{engine.song(),engine.meter(),uint16_t(elapsed*hw::sampleRate/(hw::audioBlock*10000UL)),underruns,revision};
      xQueueOverwrite(snapshots,&snapshot); lastSnapshot=millis();
    }
    size_t offset=0;
    while(offset<sizeof(pcm)) {
      size_t written=0;
      if(i2s_channel_write(channel,reinterpret_cast<uint8_t*>(pcm)+offset,sizeof(pcm)-offset,&written,100)!=ESP_OK) { ++underruns; break; }
      offset+=written;
    }
    // Buffered audio covers this yield so the core 0 idle task can run.
    vTaskDelay(1);
  }
}
bool initTouch() {
  Wire.begin(hw::sda,hw::scl); Wire.setClock(400000); Wire.setTimeOut(3);
  for(int attempt=0;attempt<3;++attempt) {
    for(uint8_t address=0x5a;address<=0x5d;++address) {
      Wire.beginTransmission(address);
      if(Wire.endTransmission()!=0) continue;
      if(touch.begin(address,&Wire,7,7)) { touchAddress=address; return true; }
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  // Keep the encoder/joystick sequencer usable when touch initialization fails.
  Serial.println("[FM] Touch unavailable; sequencer still usable"); return false;
}
struct Button {
  int pin;
  bool raw=false, down=false, longSent=false, modified=false;
  uint32_t changed=0, pressed=0;
  int poll(uint32_t now) {
    const bool value=digitalRead(pin)==LOW;
    if(value!=raw) { raw=value; changed=now; }
    if(raw!=down&&now-changed>=25) {
      down=raw;
      if(down) { pressed=now; longSent=false; modified=false; }
      else if(!longSent&&!modified) return 1;
    }
    if(down&&!longSent&&!modified&&now-pressed>=1000) { longSent=true; return 2; }
    return 0;
  }
};
int wrap(int value,int count) { return (value%count+count)%count; }
void optionChange(int delta) {
  const Song& s=view.song;
  switch(view.option) {
    case 0: send(Action::Tempo,s.bpm+delta); break;
    case 1: send(Action::Swing,s.swing+delta); break;
    case 2: send(Action::Master,s.master+delta); break;
    case 3: send(Action::Echo,s.echo+delta); break;
    case 4: send(Action::Bank,wrap((view.meter.pendingBank<bankCount?view.meter.pendingBank:s.bank)+delta,bankCount)); break;
    case 5: send(Action::Record,delta>0?1:-1); break;
    case 6: send(Action::Led,delta>0); break;
    default: break;
  }
}
void rotate(int delta,Button& joystick) {
  view.confirm=false;
  switch(view.page) {
    case Live:
      if(joystick.down) { joystick.modified=true; send(Action::Tempo,view.song.bpm+delta); }
      else send(Action::Preset,wrap(view.song.patches[1].preset+delta,presetCount),Lead);
      break;
    case Grid:
      if(joystick.down) {
        joystick.modified=true;
        const Step& step=view.song.steps[view.song.bank][view.track][view.step];
        send(view.track>=Bass?Action::StepNote:Action::StepVelocity,
             (view.track>=Bass?step.note:step.velocity)+delta,view.track,view.step);
      } else view.step=wrap(view.step+delta,stepCount);
      break;
    case Sound: {
      const Patch& p=view.song.patches[view.track-Bass];
      if(joystick.down) { joystick.modified=true; send(Action::Preset,wrap(p.preset+delta,presetCount),view.track); break; }
      const int scale=(view.parameter==Attack||view.parameter==Decay||view.parameter==Release)?10:1;
      send(Action::Param,p.values[view.parameter]+delta*scale,view.track,view.parameter); break;
    }
    case Mix: send(Action::TrackLevel,view.song.levels[view.track]+delta,view.track); break;
    case Setup: optionChange(delta); break;
  }
}
void joystickMove(int x,int y) {
  view.confirm=false;
  if(view.page==Live) {
    const float angle=atan2f(float(x),float(-y));
    view.chord=wrap(int(roundf(angle/(3.14159265f/4))),8); return;
  }
  if(abs(y)>=abs(x)) {
    const int d=y>0?1:-1;
    if(view.page==Grid||view.page==Mix) view.track=wrap(view.track+d,trackCount);
    if(view.page==Sound) view.parameter=wrap(view.parameter+d,parameterCount);
    if(view.page==Setup) view.option=wrap(view.option+d,optionCount);
  } else {
    const int d=x>0?1:-1;
    if(view.page==Grid) view.step=wrap(view.step+d,stepCount);
    if(view.page==Mix) view.track=wrap(view.track+d,trackCount);
    if(view.page==Sound) view.track=view.track==Bass?Lead:Bass;
    if(view.page==Setup) optionChange(d);
  }
}
void joystickClick() {
  switch(view.page) {
    case Live: send(Action::Record); break;
    case Grid: send(Action::ToggleStep,0,view.track,view.step); break;
    case Sound: send(Action::Note,view.track==Bass?36:60,view.track,105,25,450); break;
    case Mix: send(Action::Mute,0,view.track); break;
    case Setup:
      if(view.option==5) send(Action::Record);
      else if(view.option==6) send(Action::Led,!view.song.led);
      else if(view.option==7) storageRequest=1;
      else if(view.option==8) storageRequest=2;
      else if(view.option>=9) {
        if(!view.confirm) view.confirm=true;
        else {
          view.confirm=false;
          if(view.option==9) send(Action::ClearBank);
          if(view.option==10) storageRequest=3;
          if(view.option==11) storageRequest=4;
        }
      }
      break;
  }
}
int stringNote(int string,int chord) {
  static const uint8_t voicing[8][6]={
    {48,48,52,55,60,64}, {50,50,57,62,65,69}, {40,47,52,55,59,64}, {41,48,53,57,60,65},
    {43,47,50,55,59,67}, {45,45,52,57,60,64}, {47,47,53,59,62,65}, {43,47,50,55,59,65}
  };
  static const uint8_t open[]={40,45,50,55,59,64};
  return chord>=0?voicing[chord][string]:open[string];
}
void ledFrame(uint32_t now) {
  if(!ledReady) return;
  uint8_t pixels[48]={};
  if(view.active&&view.song.led) {
    const uint8_t colors[trackCount][3]={{200,65,40},{190,130,35},{130,180,40},{25,170,150},{55,140,210}};
    const int playhead=view.meter.step;
    for(int i=0;i<16;++i) {
      const bool beat=view.meter.playing&&i==playhead;
      const bool armed=view.chord>=0&&i==view.chord*2;
      const float brightness=beat?.35f:(armed?.25f:.015f);
      const auto& c=colors[view.track];
      pixels[i*3]=uint8_t(c[1]*brightness); pixels[i*3+1]=uint8_t(c[0]*brightness); pixels[i*3+2]=uint8_t(c[2]*brightness);
    }
  }
  // One task owns LED frames; skip busy DMA submissions without waiting.
  RingLedFrameGuard guard;
  if(guard) RingLedOutput_show(pixels);
}
void inputTask(void*) {
  Button encoder{hw::encoderButton}, joystick{hw::joystickButton};
  uint8_t previousEncoder=(digitalRead(hw::encoderA)<<1)|digitalRead(hw::encoderB);
  int edges=0;
  static const int8_t transitions[]={0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};
  uint32_t lastTouch=0,lastAdc=0,lastPower=0,lastLed=0,powerStable=0,lowSince=0,lastNav=0;
  uint16_t oldTouch=0;
  uint32_t touchAt[6]={}; uint8_t velocity[6]={};
  uint32_t batterySum=0; int batterySamples=0,oldBatteryMode=-1;
  int direction=0; bool touchStarted=false;
  uint32_t lastRelease=0; int lastString=-2; uint8_t lastVelocity=100;
  for(;;) {
    const uint32_t now=millis();
    inputHeartbeat=now;
    Snapshot snapshot;
    Notice message;
    while(xQueueReceive(notices,&message,0)==pdTRUE) notice(message.text);
    if(xQueueReceive(snapshots,&snapshot,0)==pdTRUE) {
      if(snapshot.revision>=submittedRevision.load()) { view.song=snapshot.song; view.meter=snapshot.meter; }
      view.cpuPercent=snapshot.cpu; view.underruns=snapshot.underruns;
    }
    view.audioReady=audioReady.load(); view.dropped=drops.load();
    if(view.notice[0]&&int32_t(now-noticeUntil)>=0) view.notice[0]=0;
    if(now-lastPower>=100) {
      lastPower=now; view.powerMv=analogReadMilliVolts(hw::power);
      if(view.powerMv>=hw::powerOnMv) { if(!powerStable) powerStable=now; lowSince=0; }
      else if(view.powerMv<hw::powerOffMv) { powerStable=0; if(!lowSince) lowSince=now; }
      const bool oldActive=view.active;
      if(!view.active&&now-bootAt>=5000&&powerStable&&now-powerStable>=100) view.active=true;
      // Enable shutdown detection only after audio initialization completes.
      if(view.active&&audioReady.load()&&lowSince&&now-lowSince>=500) view.active=false;
      if(oldActive&&!view.active) { send(Action::Panic); oldTouch=0; view.chord=-1; }
      running=view.active;
      view.booting=now-bootAt<5000;
    }
    if(view.active&&!audioHandle&&!audioFault.load()) {
      startupStage=TouchInit; publishView();
      if(!touchStarted) {
        view.touchReady=initTouch(); touchStarted=true;
      }
      startupStage=AudioInit; publishView();
      if(xTaskCreatePinnedToCore(audioTask,"fm_audio",6144,nullptr,10,&audioHandle,0)!=pdPASS) { notice("AUDIO TASK FAILED"); view.active=false; running=false; }
      // Initialize RMT in the low-priority loop so input and view publication continue.
      ledInitRequested=true;
    }
    if(audioReady.load()) startupStage=Running;
    if(now-lastAdc>=1000) {
      lastAdc=now; view.batteryMv=analogReadMilliVolts(hw::battery)*3;
      const bool ch=digitalRead(hw::charge),st=digitalRead(hw::standby);
      view.charging=ch&&!st; view.full=!ch&&st;
      const int mode=(view.charging||view.full)?(view.active?1:2):0;
      if(mode!=oldBatteryMode) { batterySum=0; batterySamples=0; oldBatteryMode=mode; }
      if(now-bootAt>=1000&&view.batteryMv>2500) { batterySum+=view.batteryMv; ++batterySamples; }
      const int empty=mode==2?3670:(mode==1?3620:3170),full=mode==2?4210:(mode==1?4150:3850);
      if((now-bootAt<6000&&batterySamples==4)||batterySamples>=30) {
        const int target=constrain((int(batterySum/batterySamples)-empty)*100/(full-empty),0,100);
        view.batteryPercent=now-bootAt<6000?target:constrain(target,int(view.batteryPercent)-3,int(view.batteryPercent)+3);
        batterySum=0; batterySamples=0;
      }
      view.full=view.full&&view.batteryMv>=full; // Require the full-voltage endpoint before displaying full charge.
    }
    const uint8_t state=(digitalRead(hw::encoderA)<<1)|digitalRead(hw::encoderB);
    if(view.active&&state!=previousEncoder&&!freeze.load()) {
      const int change=transitions[(previousEncoder<<2)|state];
      if(!change) edges=0;
      else { if(edges*change<0) edges=0; edges+=change; if(abs(edges)>=3) { rotate(edges>0?1:-1,joystick); edges=0; } }
    }
    previousEncoder=state;
    const int enc=encoder.poll(now),joy=joystick.poll(now);
    if(view.active&&!freeze.load()) {
      if(enc==2) { view.page=view.page==Setup?Live:Setup; view.confirm=false; }
      if(enc==1) { view.page=view.page==Setup?Live:Page((view.page+1)%4); if(view.page==Sound&&view.track<Bass) view.track=Lead; view.confirm=false; }
      if(joy==1) joystickClick();
      if(joy==2) { send(Action::Transport); view.confirm=false; }
    }
    if(now-lastNav>=10&&view.active&&!freeze.load()) {
      lastNav=now;
      const int x=analogRead(hw::joystickX)-2048,y=analogRead(hw::joystickY)-2048;
      int next=0;
      if(abs(x)>650||abs(y)>650) next=wrap(int(roundf(atan2f(float(x),float(-y))/(3.14159265f/4))),8)+1;
      if(next!=direction) { direction=next; if(next) joystickMove(x,y); else if(view.page==Live) view.chord=-1; }
    }
    if(view.active&&view.touchReady&&now-lastTouch>=2&&!freeze.load()) {
      lastTouch=now;
      // Read touch state and all 12 filtered values in one I2C burst.
      Wire.beginTransmission(touchAddress); Wire.write(uint8_t(0));
      uint8_t data[28]; bool read=false;
      if(Wire.endTransmission(false)==0&&Wire.requestFrom(touchAddress,uint8_t(28))==28) {
        for(auto& byte:data) byte=Wire.read(); read=true;
      }
      if(read) {
        const uint16_t bits=(data[0]|(data[1]<<8))&0x0fff;
        const uint16_t pressed=bits&~oldTouch,released=oldTouch&~bits;
        bool all=true; for(auto channel:hw::strings) if(!(bits&(1<<channel))) all=false;
        static bool muteHeld=false;
        if(all) { if(!muteHeld) send(Action::Panic); muteHeld=true; }
        else {
          for(int s=0;s<6;++s) {
            const int channel=hw::strings[s];
            if(pressed&(1<<channel)) {
              send(Action::Off,0,0,0,10+s); touchAt[s]=now;
              const int filtered=data[4+channel*2]|(data[5+channel*2]<<8);
              const int delta=max(0,int(touch.baselineData(channel))-filtered);
              velocity[s]=constrain(50+(delta-7)*77/163,50,127);
            }
            if((released&(1<<channel))&&!muteHeld) {
              int v=velocity[s]?velocity[s]:100;
              if(now-touchAt[s]<35&&now-lastRelease<120&&abs(s-lastString)==1) v=max(v,int(lastVelocity));
              send(Action::Note,stringNote(s,view.chord),Lead,v,10+s,1000);
              lastRelease=now; lastString=s; lastVelocity=v;
            }
          }
          // After an all-string mute, require every string to release before sounding.
          bool any=false; for(auto channel:hw::strings) any|=bits&(1<<channel);
          if(!any) muteHeld=false;
        }
        if(pressed&(1<<hw::kickPad)) send(Action::Note,36,Kick,110,20,100);
        if(pressed&(1<<hw::snarePad)) send(Action::Note,38,Snare,110,21,100);
        if((pressed|released)&((1<<hw::bendUp)|(1<<hw::bendDown)))
          send(Action::Bend,((bits&(1<<hw::bendUp))?200:0)-((bits&(1<<hw::bendDown))?200:0));
        oldTouch=bits;
      }
    }
    if(now-lastLed>=50) { lastLed=now; ledFrame(now); }
    publishView();
    // Yield even after a touch timeout so the display task can continue.
    vTaskDelay(1);
  }
}
} // namespace
bool runtimeView(View& output) {
  if(!viewMutex) return false;
  if(xSemaphoreTake(viewMutex,pdMS_TO_TICKS(5))!=pdTRUE) return false;
  output=publishedView; xSemaphoreGive(viewMutex);
  // Publish startup progress separately so the display can report a stalled driver.
  output.booting=millis()-bootAt<5000;
  output.startup=startupStage.load(); output.audioReady=audioReady.load(); output.audioFault=audioFault.load();
  output.inputStalled=millis()-inputHeartbeat.load()>1500;
  return true;
}
void runtimeBegin() {
  bootAt=millis(); defaults(view.song); load(view.song); engine.init(hw::sampleRate,view.song);
  publishedView=view; inputHeartbeat=millis();
  viewMutex=xSemaphoreCreateMutex(); commands=xQueueCreate(64,sizeof(Command)); snapshots=xQueueCreate(1,sizeof(Snapshot));
  notices=xQueueCreate(4,sizeof(Notice));
  pinMode(hw::encoderA,INPUT_PULLUP); pinMode(hw::encoderB,INPUT_PULLUP); pinMode(hw::encoderButton,INPUT_PULLUP);
  pinMode(hw::joystickButton,INPUT_PULLUP); pinMode(hw::charge,INPUT); pinMode(hw::standby,INPUT);
  pinMode(hw::power,INPUT_PULLDOWN); analogReadResolution(12);
  // Register each ADC channel before configuring its attenuation.
  for(int pin:{hw::battery,hw::power,hw::joystickX,hw::joystickY}) { analogRead(pin); analogSetPinAttenuation(pin,ADC_11db); }
  if(!viewMutex||!commands||!snapshots||!notices) { Serial.println("[FM] RTOS allocation failed"); return; }
  if(xTaskCreatePinnedToCore(inputTask,"fm_input",6144,nullptr,4,&inputHandle,1)!=pdPASS) Serial.println("[FM] Input task creation failed");
}
void runtimeService() {
  if(ledInitRequested.exchange(false)) {
    ledReady=RingLedOutput_begin();
    if(!ledReady.load()) postNotice("LED INIT FAILED");
  }
  static uint32_t lastDiagnostics=0;
  if(millis()-lastDiagnostics>=5000) {
    lastDiagnostics=millis();
    View snapshot;
    if(runtimeView(snapshot)&&snapshot.active&&Serial&&Serial.availableForWrite()>=160) {
      Serial.printf("[FM] render=%u%% overruns=%lu drops=%lu heap=%u stackFree audio=%u input=%u\n",
        snapshot.cpuPercent,(unsigned long)snapshot.underruns,(unsigned long)snapshot.dropped,
        ESP.getFreeHeap(),audioHandle?uxTaskGetStackHighWaterMark(audioHandle):0,
        inputHandle?uxTaskGetStackHighWaterMark(inputHandle):0);
    }
  }
  const int request=storageRequest.exchange(0);
  if(!request) return;
  if(!audioReady.load()) return;
  freeze=true;
  const uint32_t deadline=millis()+1000;
  while(!frozen.load()&&int32_t(millis()-deadline)<0) delay(5);
  const char* result="AUDIO BUSY";
  if(frozen.load()) {
    if(request==1) result=save(frozenSong)?"SAVED":"SAVE FAILED";
    if(request==2) { restoreSong=frozenSong; if(load(restoreSong)) { restorePending=true; result="LOADED"; } else result="NO SAVED SONG"; }
    if(request==3) {
      defaults(restoreSong); restorePending=true;
      Preferences prefs; if(prefs.begin("fsg-fm",false)) { prefs.clear(); prefs.end(); }
      result="DEFAULTS RESTORED";
    }
    if(request==4) {
      const esp_partition_t* app=esp_partition_find_first(ESP_PARTITION_TYPE_APP,ESP_PARTITION_SUBTYPE_APP_OTA_1,nullptr);
      if(app&&esp_ota_set_boot_partition(app)==ESP_OK) { delay(100); ESP.restart(); }
      result="UPLOADER INVALID";
    }
  }
  postNotice(result);
  freeze=false;
}
}
