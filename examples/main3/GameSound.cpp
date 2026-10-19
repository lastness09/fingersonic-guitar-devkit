#include "GameSound.h"
#include "config.h"
#include <string.h>
#ifndef PEA_HOST_TEST
#include <Arduino.h>
#include <driver/i2s_std.h>
#include <atomic>
#endif
namespace pea {
void SoundSynth::clear() { for(auto& v:voices_) v.remaining=0; }
void SoundSynth::trigger(Effect effect) {
  static const uint16_t tones[][3]={
    {650,220,65},{230,90,40},{940,420,100},{150,45,160},
    {1400,800,240},{90,30,300},{520,1040,240},{340,70,600}
  };
  const int e=int(effect); if(e<0||e>7) return;
  Voice* selected=&voices_[0];
  for(auto& v:voices_) if(v.remaining<selected->remaining) selected=&v;
  Voice& v=*selected; const uint32_t frames=tones[e][2]*hw::sampleRate/1000;
  const uint32_t increment=uint64_t(tones[e][0])*0x100000000ULL/hw::sampleRate;
  const uint32_t end=uint64_t(tones[e][1])*0x100000000ULL/hw::sampleRate;
  v={0x40000000,increment,frames,frames,int32_t(int64_t(end)-increment)/int32_t(frames),effect==Effect::Hit||effect==Effect::Bomb};
}
void SoundSynth::render(int16_t* stereo,int frames) {
  for(int n=0;n<frames;++n) {
    int32_t mix=0;
    for(auto& v:voices_) if(v.remaining) {
      const int p=v.phase>>16;
      int sample=p<32768?p*2-32768:98304-p*2;
      if(v.noise) { noise_^=noise_<<13; noise_^=noise_>>17; noise_^=noise_<<5; sample=(sample+int16_t(noise_))/2; }
      const uint32_t elapsed=v.total-v.remaining;
      const float attack=elapsed<48?elapsed/48.f:1.f;
      const float envelope=attack*v.remaining/float(v.total);
      mix+=int32_t(sample*envelope*.13f);
      v.phase+=v.increment; v.increment+=v.sweep; --v.remaining;
    }
    if(mix>30000) mix=30000; if(mix<-30000) mix=-30000;
    stereo[n*2]=stereo[n*2+1]=int16_t(mix);
  }
}
#ifndef PEA_HOST_TEST
namespace {
QueueHandle_t events=nullptr;
std::atomic<bool> active{false},enabled{true},ready{false};
void task(void*) {
  i2s_chan_config_t config=I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0,I2S_ROLE_MASTER);
  config.dma_desc_num=4; config.dma_frame_num=128; config.auto_clear=true;
  i2s_chan_handle_t channel=nullptr;
  esp_err_t error=i2s_new_channel(&config,&channel,nullptr);
  i2s_std_config_t standard{};
  standard.clk_cfg=I2S_STD_CLK_DEFAULT_CONFIG(hw::sampleRate);
  standard.slot_cfg=I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT,I2S_SLOT_MODE_STEREO);
  standard.gpio_cfg.mclk=I2S_GPIO_UNUSED; standard.gpio_cfg.bclk=gpio_num_t(hw::i2sClock);
  standard.gpio_cfg.ws=gpio_num_t(hw::i2sWord); standard.gpio_cfg.dout=gpio_num_t(hw::i2sData); standard.gpio_cfg.din=I2S_GPIO_UNUSED;
  if(error==ESP_OK) error=i2s_channel_init_std_mode(channel,&standard);
  if(error!=ESP_OK) {
    Serial.printf("[PEA] Audio unavailable: %s\n",esp_err_to_name(error));
    if(channel) i2s_del_channel(channel); vTaskDelete(nullptr); return;
  }
  SoundSynth synth; int16_t pcm[hw::audioBlock*2]; bool started=false;
  ready=true;
  for(;;) {
    if(!active.load()) {
      if(started) { i2s_channel_disable(channel); started=false; }
      synth.clear(); xQueueReset(events); vTaskDelay(pdMS_TO_TICKS(10)); continue;
    }
    if(!started) { i2s_channel_enable(channel); started=true; }
    Event event;
    while(xQueueReceive(events,&event,0)==pdTRUE) if(enabled.load()) synth.trigger(event.effect);
    synth.render(pcm,hw::audioBlock);
    if(!enabled.load()) memset(pcm,0,sizeof(pcm));
    size_t written=0; i2s_channel_write(channel,pcm,sizeof(pcm),&written,100);
    vTaskDelay(1);
  }
}
}
void soundBegin() {
  if(events) return;
  events=xQueueCreate(24,sizeof(Event));
  if(events&&xTaskCreatePinnedToCore(task,"pea_audio",4096,nullptr,6,nullptr,0)!=pdPASS) Serial.println("[PEA] Audio task failed");
}
void soundActive(bool value) { active=value; }
void soundEnabled(bool value) { enabled=value; }
void soundPush(const Event& event) { if(events) xQueueSend(events,&event,0); }
bool soundReady() { return ready.load(); }
#endif
}
