#include "GameRuntime.h"
#include "GameSound.h"
#include "RingLedOutput.h"
#include "config.h"
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPR121.h>
#include <Preferences.h>
#include <esp_ota_ops.h>
#include <atomic>
#include <string.h>

namespace pea {
namespace {
QueueHandle_t frames=nullptr,saveRequests=nullptr,notices=nullptr;
struct Notice { char text[24]; };
Game game;
Frame frame;
Adafruit_MPR121 touch;
std::atomic<uint32_t> heartbeat{0},settingsAt{0};
std::atomic<bool> initRequested{false},ledReady{false},uploadRequested{false};
uint8_t touchAddress=0;
uint32_t bootAt=0,noticeUntil=0;
bool resumeAfterMenu=false;
struct Saved { uint32_t magic=0x50454133; Config config{}; };
void load(Config& config) {
  Preferences prefs;
  if(!prefs.begin("fsg-main3",true)) return;
  Saved saved{};
  const bool valid=prefs.getBytesLength("config")==sizeof(saved);
  if(valid) prefs.getBytes("config",&saved,sizeof(saved)); prefs.end();
  if(valid&&saved.magic==0x50454133&&saved.config.difficulty<=2) config=saved.config;
}
void save(const Config& config) {
  Saved saved; saved.config=config;
  Preferences prefs;
  if(prefs.begin("fsg-main3",false)) { prefs.putBytes("config",&saved,sizeof(saved)); prefs.end(); }
}
void settingsChanged() {
  settingsAt=millis(); xQueueOverwrite(saveRequests,&frame.config);
}
void publish() {
  frame.game=game.state(); frame.now=millis(); frame.audioReady=soundReady();
  // The queue copies state without holding a UI lock across ADC, I2C or driver initialization.
  xQueueOverwrite(frames,&frame);
}
void notice(const char* text) { strlcpy(frame.notice,text,sizeof(frame.notice)); noticeUntil=millis()+1800; }
int wrap(int value,int count) { return (value%count+count)%count; }
void newReady() { game.ready(frame.config.difficulty,frame.config.best[frame.config.difficulty]); }
void start() { newReady(); game.start(); }
void shoot(int lane) {
  if(frame.menu) return;
  if(game.state().mode==Mode::Over) newReady();
  game.fire(lane); frame.selected=lane;
}
void toggleMenu() {
  if(!frame.menu) {
    resumeAfterMenu=game.state().mode==Mode::Playing;
    if(resumeAfterMenu) game.pause(); frame.menu=true;
  } else {
    frame.menu=false;
    if(resumeAfterMenu&&game.state().mode==Mode::Paused) game.pause();
  }
  frame.confirm=false;
}
void adjust(int delta) {
  frame.confirm=false;
  if(frame.menu) {
    if(frame.option==0) {
      frame.config.difficulty=wrap(frame.config.difficulty+delta,3);
      if(game.state().mode==Mode::Ready||game.state().mode==Mode::Over) newReady(); settingsChanged();
    }
    if(frame.option==1) { frame.config.sound=delta>0; soundEnabled(frame.config.sound); settingsChanged(); }
    if(frame.option==2) { frame.config.led=delta>0; settingsChanged(); }
  } else if(game.state().mode==Mode::Ready||game.state().mode==Mode::Over) {
    frame.config.difficulty=wrap(frame.config.difficulty+delta,3); newReady(); settingsChanged();
  } else frame.selected=wrap(frame.selected+delta,lanes);
}
void joystickClick() {
  if(frame.menu) {
    if(frame.option==1) { frame.config.sound=!frame.config.sound; soundEnabled(frame.config.sound); settingsChanged(); }
    if(frame.option==2) { frame.config.led=!frame.config.led; settingsChanged(); }
    if(frame.option>=3) {
      if(!frame.confirm) frame.confirm=true;
      else {
        frame.confirm=false;
        if(frame.option==3) { frame.menu=false; newReady(); }
        if(frame.option==4) uploadRequested=true;
      }
    }
  } else if(game.state().mode==Mode::Paused) game.pause();
  else shoot(frame.selected);
}
struct Button {
  int pin; bool raw=false,down=false,longSent=false; uint32_t changed=0,pressed=0;
  int poll(uint32_t now) {
    const bool sample=digitalRead(pin)==LOW;
    if(sample!=raw) { raw=sample; changed=now; }
    if(raw!=down&&now-changed>=25) {
      down=raw;
      if(down) { pressed=now; longSent=false; }
      else if(!longSent) return 1;
    }
    if(down&&!longSent&&now-pressed>=900) { longSent=true; return 2; }
    return 0;
  }
};
bool initTouch() {
  Wire.begin(hw::sda,hw::scl); Wire.setClock(400000); Wire.setTimeOut(3);
  for(int attempt=0;attempt<3;++attempt) {
    for(uint8_t address=0x5a;address<=0x5d;++address) {
      Wire.beginTransmission(address);
      if(Wire.endTransmission()!=0) continue;
      if(touch.begin(address,&Wire,7,7)) { touchAddress=address; return true; }
    }
    vTaskDelay(pdMS_TO_TICKS(80));
  }
  Serial.println("[PEA] MPR121 unavailable; joystick fire still works"); return false;
}
void task(void*) {
  Button encoder{hw::encoderButton},joystick{hw::joystickButton};
  uint8_t previous=(digitalRead(hw::encoderA)<<1)|digitalRead(hw::encoderB);
  const int8_t transition[]={0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};
  int edges=0,nav=0;
  uint16_t oldTouch=0;
  bool touchStarted=false,syncTouch=true;
  uint32_t lastTouch=0,lastPower=0,lastBattery=0,lastStick=0,lastSim=millis(),lastPublish=0;
  uint32_t highAt=0,lowAt=0,navAt=0,batterySum=0;
  int batteryCount=0,batteryMode=-1;
  for(;;) {
    const uint32_t now=millis(); heartbeat=now;
    Notice message; while(xQueueReceive(notices,&message,0)==pdTRUE) notice(message.text);
    if(frame.notice[0]&&int32_t(now-noticeUntil)>=0) frame.notice[0]=0;
    frame.booting=now-bootAt<1000;
    if(now-lastPower>=100) {
      lastPower=now; frame.powerMv=analogReadMilliVolts(hw::power);
      if(frame.powerMv>=hw::powerOnMv) { if(!highAt) highAt=now; lowAt=0; }
      else if(frame.powerMv<hw::powerOffMv) { highAt=0; if(!lowAt) lowAt=now; }
      const bool wasActive=frame.active;
      if(!frame.active&&!frame.booting&&highAt&&now-highAt>=100) frame.active=true;
      if(frame.active&&lowAt&&now-lowAt>=500) frame.active=false;
      if(wasActive&&!frame.active) { newReady(); frame.menu=false; syncTouch=true; }
      soundActive(frame.active);
    }
    if(frame.active&&!touchStarted) {
      publish(); // Publish before touch initialization so the display does not depend on driver success.
      frame.touchReady=initTouch(); touchStarted=true; syncTouch=true; initRequested=true;
    }
    if(now-lastBattery>=1000) {
      lastBattery=now;
      frame.batteryMv=analogReadMilliVolts(hw::battery)*3;
      const bool ch=digitalRead(hw::charge),st=digitalRead(hw::standby);
      frame.charging=ch&&!st; frame.full=!ch&&st;
      const int mode=(frame.charging||frame.full)?(frame.active?1:2):0;
      if(mode!=batteryMode) { batteryMode=mode; batteryCount=0; batterySum=0; }
      if(now-bootAt>=1000&&frame.batteryMv>=2500) { batterySum+=frame.batteryMv; ++batteryCount; }
      const int low=mode==2?3670:(mode==1?3620:3170),high=mode==2?4210:(mode==1?4150:3850);
      if((now-bootAt<6000&&batteryCount==4)||batteryCount>=30) {
        const int target=constrain((int(batterySum/batteryCount)-low)*100/(high-low),0,100);
        frame.batteryPercent=now-bootAt<6000?target:constrain(target,int(frame.batteryPercent)-3,int(frame.batteryPercent)+3);
        batterySum=0; batteryCount=0;
      }
      frame.full=frame.full&&frame.batteryMv>=high;
    }
    const uint8_t state=(digitalRead(hw::encoderA)<<1)|digitalRead(hw::encoderB);
    if(state!=previous&&frame.active) {
      const int delta=transition[(previous<<2)|state];
      if(!delta) edges=0;
      else { if(edges*delta<0) edges=0; edges+=delta; if(abs(edges)>=3) { adjust(edges>0?1:-1); edges=0; } }
    }
    previous=state;
    const int enc=encoder.poll(now),joy=joystick.poll(now);
    if(frame.active) {
      if(enc==2) toggleMenu();
      if(enc==1) {
        if(frame.menu) toggleMenu();
        else if(game.state().mode==Mode::Ready||game.state().mode==Mode::Over) start();
        else game.pause();
      }
      if(joy==1) joystickClick();
      if(joy==2&&!frame.menu) game.pause();
      if(now-lastStick>=10) {
        lastStick=now;
        const int x=analogRead(hw::joystickX)-2048,y=analogRead(hw::joystickY)-2048;
        const int direction=abs(x)<650&&abs(y)<650?0:(abs(y)>=abs(x)?(y>0?1:-1):(x>0?2:-2));
        if(direction&&(direction!=nav||now-navAt>=250)) {
          navAt=now; frame.confirm=false;
          if(abs(direction)==1) {
            if(frame.menu) frame.option=wrap(frame.option+(direction>0?1:-1),5);
            else frame.selected=wrap(frame.selected+(direction>0?1:-1),lanes);
          } else adjust(direction>0?1:-1);
        }
        nav=direction;
      }
      if(frame.touchReady&&now-lastTouch>=2) {
        lastTouch=now;
        Wire.beginTransmission(touchAddress); Wire.write(uint8_t(0));
        if(Wire.endTransmission(false)==0&&Wire.requestFrom(touchAddress,uint8_t(2))==2) {
          const uint8_t low=Wire.read(),high=Wire.read();
          const uint16_t bits=(low|(high<<8))&0x0fff;
          // Synchronize the first reading so a held string at startup does not start a game.
          const uint16_t pressed=syncTouch?0:(bits&~oldTouch); syncTouch=false; oldTouch=bits;
          if(!frame.menu) {
            for(int lane=0;lane<lanes;++lane) if(pressed&(1<<hw::strings[lane])) shoot(lane);
            if(pressed&(1<<hw::freezePad)) game.freeze();
            if(pressed&(1<<hw::bombPad)) game.bomb();
            if(pressed&(1<<hw::laneUpPad)) frame.selected=wrap(frame.selected-1,lanes);
            if(pressed&(1<<hw::laneDownPad)) frame.selected=wrap(frame.selected+1,lanes);
          }
        }
      }
    }
    const uint32_t elapsed=now-lastSim; lastSim=now;
    if(frame.active&&!frame.menu) game.update(uint16_t(min(100UL,elapsed)));
    Event event; while(game.popEvent(event)) soundPush(event);
    const State& current=game.state();
    if(current.best>frame.config.best[current.difficulty]) {
      frame.config.best[current.difficulty]=current.best; settingsChanged();
    }
    if(now-lastPublish>=16) { lastPublish=now; publish(); }
    // Do not catch up missed deadlines; leave display time after an I2C timeout.
    vTaskDelay(1);
  }
}
void leds(const Frame& state) {
  uint8_t pixels[48]={};
  if(state.active&&state.config.led) {
    for(int lane=0;lane<6;++lane) {
      const bool shot=state.game.shotFlash[lane]>0,hurt=state.game.hurtFlash[lane]>0;
      for(int n=0;n<2;++n) {
        const int i=(lane*2+n)*3;
        pixels[i]=hurt?2:(shot?90:8); pixels[i+1]=hurt?90:(shot?40:3); pixels[i+2]=shot?5:1;
      }
    }
    for(int i=0;i<state.game.lives;++i) { pixels[(12+i)*3]=20; pixels[(12+i)*3+1]=10; }
    if(state.game.time<state.game.freezeUntil) { pixels[45]=25; pixels[47]=60; }
  }
  RingLedFrameGuard guard; if(guard) RingLedOutput_show(pixels);
}
}
bool runtimeFrame(Frame& output) {
  if(!frames||xQueuePeek(frames,&output,pdMS_TO_TICKS(2))!=pdTRUE) return false;
  output.inputStalled=millis()-heartbeat.load()>1500; output.now=millis();
  output.booting=millis()-bootAt<1000; return true;
}
void runtimeBegin() {
  bootAt=millis(); heartbeat=bootAt; load(frame.config);
  game.init(frame.config.difficulty,frame.config.best[frame.config.difficulty]);
  frames=xQueueCreate(1,sizeof(Frame)); saveRequests=xQueueCreate(1,sizeof(Config)); notices=xQueueCreate(4,sizeof(Notice));
  pinMode(hw::encoderA,INPUT_PULLUP); pinMode(hw::encoderB,INPUT_PULLUP); pinMode(hw::encoderButton,INPUT_PULLUP);
  pinMode(hw::joystickButton,INPUT_PULLUP); pinMode(hw::charge,INPUT); pinMode(hw::standby,INPUT);
  pinMode(hw::reservedOutput,OUTPUT); digitalWrite(hw::reservedOutput,LOW);
  pinMode(hw::power,INPUT_PULLDOWN); analogReadResolution(12);
  for(int pin:{hw::power,hw::battery,hw::joystickX,hw::joystickY}) { analogRead(pin); analogSetPinAttenuation(pin,ADC_11db); }
  if(!frames||!saveRequests||!notices) { Serial.println("[PEA] Queue allocation failed"); return; }
  publish(); soundEnabled(frame.config.sound);
  if(xTaskCreatePinnedToCore(task,"pea_game",6144,nullptr,4,nullptr,1)!=pdPASS) Serial.println("[PEA] Game task failed");
}
void runtimeService() {
  if(initRequested.exchange(false)) { soundBegin(); ledReady=RingLedOutput_begin(); }
  Frame snapshot; if(!runtimeFrame(snapshot)) return;
  static uint32_t lastLed=0;
  if(ledReady.load()&&millis()-lastLed>=50) { lastLed=millis(); leds(snapshot); }
  if(uploadRequested.exchange(false)) {
    soundActive(false);
    const esp_partition_t* app=esp_partition_find_first(ESP_PARTITION_TYPE_APP,ESP_PARTITION_SUBTYPE_APP_OTA_1,nullptr);
    if(app&&esp_ota_set_boot_partition(app)==ESP_OK) { delay(100); ESP.restart(); }
    Notice message{}; strlcpy(message.text,"APP1 INVALID",sizeof(message.text)); xQueueSend(notices,&message,0);
    soundActive(snapshot.active);
  }
  // Save only outside active play after sound effects finish; do not write Flash during a game.
  if(snapshot.game.mode!=Mode::Playing&&millis()-settingsAt.load()>=1500) {
    Config config; if(xQueueReceive(saveRequests,&config,0)==pdTRUE) save(config);
  }
}
}
