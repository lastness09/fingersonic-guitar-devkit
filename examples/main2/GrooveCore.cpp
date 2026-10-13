// SPDX-License-Identifier: Apache-2.0
#include "GrooveCore.h"
#include <math.h>
#include <string.h>
namespace groove {
const char* const chordNames[]={"C","Dm","Em","F","G","Am","Bdim","G7"};
const char* const optionNames[]={"TEMPO","SWING","MASTER","ECHO","PATTERN","RECORD","LED SHOW","SAVE","LOAD","CLEAR","DEFAULTS","UPLOADER"};
const char* const trackNames[]={"KICK","SNARE","HAT","BASS","LEAD"};
const char* const presetNames[]={"GLASS EP","ROUND BASS","PLUCK","BELL","SOFT ORGAN","MARIMBA","FM BRASS","SPACE"};
const char* const parameterNames[]={"RATIO","FM DEPTH","ATTACK","DECAY","SUSTAIN","RELEASE","TONE","LEVEL"};
static int clamp(int v,int lo,int hi) { return v<lo?lo:(v>hi?hi:v); }
int parameterLimit(int i) {
  const int limits[]={32,500,2000,2500,100,3000,100,100};
  return i>=0&&i<parameterCount?limits[i]:0;
}
Patch preset(int i) {
  static const uint16_t values[presetCount][parameterCount]={
    {4,160,4,620,18,500,78,85}, {2,90,3,300,55,140,38,100},
    {4,220,2,260,5,180,68,90}, {14,280,3,1400,0,1100,85,78},
    {4,50,25,250,85,220,60,80}, {8,230,2,190,0,180,65,90},
    {4,300,60,280,70,250,65,78}, {6,180,300,900,60,1600,55,75}
  };
  Patch p{}; p.preset=clamp(i,0,presetCount-1);
  memcpy(p.values,values[p.preset],sizeof(p.values)); return p;
}
void defaults(Song& s) {
  memset(&s,0,sizeof(s)); s.bpm=110; s.master=75; s.echo=18; s.led=1;
  s.patches[0]=preset(1); s.patches[1]=preset(0);
  for(int t=0;t<trackCount;++t) s.levels[t]=t==Hat?55:80;
  const uint16_t masks[bankCount][trackCount]={
    {0x1111,0x1010,0x5555,0x1111,0x0404}, {0x1141,0x1010,0xffff,0x4511,0x2204},
    {0x4921,0x1040,0x5555,0x4921,0x0422}, {0x1111,0x1010,0xaaaa,0x4444,0x1249}
  };
  const uint8_t bass[]={36,36,43,36,36,36,46,43,36,36,43,36,34,34,43,46};
  const uint8_t lead[]={60,67,70,72,67,63,60,67,72,70,67,63,60,63,67,70};
  for(int b=0;b<bankCount;++b) for(int t=0;t<trackCount;++t) for(int i=0;i<stepCount;++i)
    s.steps[b][t][i]={uint8_t(t==Bass?bass[i]:(t==Lead?lead[i]:36+t*2)),uint8_t(i%4==0?112:88),uint8_t((masks[b][t]>>i)&1)};
}
void sanitize(Song& s) {
  s.bpm=clamp(s.bpm,30,300); s.swing=clamp(s.swing,0,60); s.master=clamp(s.master,0,100);
  s.echo=clamp(s.echo,0,60); s.led=!!s.led; s.bank=clamp(s.bank,0,bankCount-1);
  for(int t=0;t<trackCount;++t) { s.mute[t]=!!s.mute[t]; s.levels[t]=clamp(s.levels[t],0,100); }
  for(auto& p:s.patches) {
    p.preset=clamp(p.preset,0,presetCount-1);
    for(int i=0;i<parameterCount;++i) p.values[i]=clamp(p.values[i],i==Ratio?1:0,parameterLimit(i));
  }
  for(auto& bank:s.steps) for(auto& track:bank) for(auto& step:track) {
    step.note=clamp(step.note,24,96); step.velocity=clamp(step.velocity,1,127); step.enabled=!!step.enabled;
  }
}
float midiFrequency(int note) { return 440.f*powf(2.f,(note-69)/12.f); }
void Engine::init(uint32_t sampleRate,const Song& s) {
  sampleRate_=sampleRate; daisysp::Oscillator::InitSineTable();
  for(auto& v:voices_) { v.fm.Init(sampleRate); v.envelope.Init(sampleRate); }
  restore(s);
}
void Engine::restore(const Song& s) {
  song_=s; sanitize(song_); meter_=Meter{}; clock_=0; nextStep_=0; stepRemainder_=0; bend_=0;
  for(auto& v:voices_) { v.active=false; v.tail=0; }
  for(auto& d:drum_) d.env=0;
  memset(delay_,0,sizeof(delay_)); delayPos_=0; dcX_=dcY_=0;
}
void Engine::trigger(int track,int note,int velocity,uint32_t gateFrames,int tag) {
  if(track<0||track>=trackCount||song_.mute[track]) return;
  velocity=clamp(velocity,1,127); note=clamp(note,24,96); ++meter_.triggers[track];
  if(meter_.recording&&meter_.playing&&tag>=10) {
    const int step=clock_>=nextStep_?(meter_.step+1)%stepCount:meter_.step;
    song_.steps[song_.bank][track][step]={uint8_t(note),uint8_t(velocity),1};
  }
  if(track<3) {
    Drum& d=drum_[track]; d.env=1; d.phase=0; d.filter=0;
    d.decay=expf(-1.f/(sampleRate_*(track==Kick?.18f:(track==Snare?.12f:.04f))));
    d.gain=(velocity/127.f)*(song_.levels[track]/100.f); return;
  }
  Voice* chosen=&voices_[0];
  for(auto& v:voices_) { if(!v.active) { chosen=&v; break; } if(v.amplitude<chosen->amplitude) chosen=&v; }
  Voice& v=*chosen;
  // Fade the stolen voice from its last sample to avoid a discontinuity.
  const float tail=v.active?v.filtered*v.amplitude*v.gain:0;
  const Patch& p=song_.patches[track-Bass];
  const float frequency=midiFrequency(note)*powf(2.f,bend_/12.f);
  const float ratio=fminf(p.values[Ratio]/4.f,sampleRate_*.45f/frequency);
  v.fm.Reset(); v.fm.SetFrequency(frequency); v.fm.SetRatio(ratio);
  v.envelope.Init(sampleRate_);
  v.envelope.SetTime(daisysp::ADSR_SEG_ATTACK,fmaxf(.002f,p.values[Attack]/1000.f));
  v.envelope.SetTime(daisysp::ADSR_SEG_DECAY,fmaxf(.01f,p.values[Decay]/1000.f));
  v.envelope.SetTime(daisysp::ADSR_SEG_RELEASE,fmaxf(.01f,p.values[Release]/1000.f));
  v.envelope.SetSustainLevel(p.values[Sustain]/100.f);
  v.track=track; v.note=note; v.tag=tag; v.gate=gateFrames; v.active=true;
  v.gain=(velocity/127.f)*(p.values[Level]/100.f)*.20f;
  // Convert DaisySP FM2 phase increment to a pitch-independent FM index.
  // This keeps modulation depth and preset brightness consistent across pitches.
  v.depth=(p.values[Index]/100.f)*5.f*frequency*ratio/sampleRate_;
  v.tone=.015f+p.values[Tone]*.006f;
  v.filtered=0; v.amplitude=0; v.tail=tail;
}
void Engine::command(const Command& c) {
  const int t=clamp(c.track,0,trackCount-1),i=clamp(c.item,0,stepCount-1);
  switch(c.action) {
    case Action::Transport:
      meter_.playing=!meter_.playing;
      if(meter_.playing) { meter_.step=15; nextStep_=clock_; stepRemainder_=0; }
      else { meter_.recording=false; for(auto& v:voices_) if(v.tag<10) v.gate=0; }
      break;
    case Action::Record:
      meter_.recording=c.value==0?!meter_.recording:c.value>0;
      if(meter_.recording&&!meter_.playing) { meter_.playing=true; meter_.step=15; nextStep_=clock_; stepRemainder_=0; }
      break;
    case Action::Tempo: song_.bpm=clamp(c.value,30,300); break;
    case Action::Swing: song_.swing=clamp(c.value,0,60); break;
    case Action::Master: song_.master=clamp(c.value,0,100); break;
    case Action::Echo: song_.echo=clamp(c.value,0,60); break;
    case Action::Led: song_.led=!!c.value; break;
    case Action::Bank:
      if(meter_.playing) meter_.pendingBank=clamp(c.value,0,bankCount-1);
      else song_.bank=clamp(c.value,0,bankCount-1); break;
    case Action::Preset: if(t>=Bass) song_.patches[t-Bass]=preset(c.value); break;
    case Action::Param: if(t>=Bass&&c.item<parameterCount) song_.patches[t-Bass].values[c.item]=clamp(c.value,c.item==Ratio?1:0,parameterLimit(c.item)); break;
    case Action::Mute:
      song_.mute[t]=!song_.mute[t];
      if(song_.mute[t]) { if(t<3) drum_[t].env=0; for(auto& v:voices_) if(v.track==t) v.gate=0; } break;
    case Action::TrackLevel: song_.levels[t]=clamp(c.value,0,100); break;
    case Action::ToggleStep: song_.steps[song_.bank][t][i].enabled^=1; break;
    case Action::StepNote: song_.steps[song_.bank][t][i].note=clamp(c.value,24,96); break;
    case Action::StepVelocity: song_.steps[song_.bank][t][i].velocity=clamp(c.value,1,127); break;
    case Action::Note: trigger(t,c.value,c.item,c.gateMs*sampleRate_/1000,c.tag); break;
    case Action::Off: for(auto& v:voices_) if(v.tag==c.tag) v.gate=0; break;
    case Action::Bend: {
      const float next=clamp(c.value,-200,200)/100.f;
      const float scale=powf(2.f,(next-bend_)/12.f); bend_=next;
      for(auto& v:voices_) if(v.active) { v.fm.SetFrequency(midiFrequency(v.note)*powf(2.f,bend_/12.f)); v.depth*=scale; }
      break;
    }
    case Action::Panic:
      meter_.playing=false; meter_.recording=false;
      for(auto& v:voices_) { v.gate=0; v.envelope.SetTime(daisysp::ADSR_SEG_RELEASE,.01f); }
      for(auto& d:drum_) d.env=0;
      memset(delay_,0,sizeof(delay_)); break;
    case Action::ClearBank:
      for(auto& track:song_.steps[song_.bank]) for(auto& step:track) step.enabled=0; break;
  }
}
void Engine::advanceStep() {
  meter_.step=(meter_.step+1)%stepCount;
  if(meter_.step==0&&meter_.pendingBank<bankCount) { song_.bank=meter_.pendingBank; meter_.pendingBank=255; }
  const double base=sampleRate_*15.0/song_.bpm;
  const double duration=base*(1.0+(meter_.step%2?-1:1)*song_.swing/100.0)+stepRemainder_;
  const uint32_t frames=uint32_t(duration); stepRemainder_=duration-frames; nextStep_=clock_+frames;
  for(int t=0;t<trackCount;++t) {
    const Step& step=song_.steps[song_.bank][t][meter_.step];
    if(step.enabled) trigger(t,step.note,step.velocity,frames*3/4,t);
  }
}
float Engine::random() {
  noise_^=noise_<<13; noise_^=noise_>>17; noise_^=noise_<<5;
  return (int32_t(noise_)/2147483648.f);
}
float Engine::drums() {
  float mix=0;
  for(int i=0;i<3;++i) {
    Drum& d=drum_[i]; if(d.env<.0001f) continue;
    float sound=0;
    if(i==Kick) {
      d.phase+=(45.f+110.f*d.env*d.env)/sampleRate_; if(d.phase>=1) d.phase-=1;
      sound=daisysp::Oscillator::Sine(d.phase)*.7f;
    } else {
      const float noise=random(); d.filter+=.12f*(noise-d.filter);
      if(i==Snare) {
        d.phase+=180.f/sampleRate_; if(d.phase>=1) d.phase-=1;
        sound=.35f*daisysp::Oscillator::Sine(d.phase)+.38f*(noise-d.filter);
      } else sound=.27f*(noise-d.filter);
    }
    mix+=sound*d.env*d.gain; d.env*=d.decay;
  }
  return mix;
}
void Engine::render(int16_t* output,int frames) {
  float peak=0; int active=0;
  for(int n=0;n<frames;++n,++clock_) {
    if(meter_.playing&&clock_>=nextStep_) advanceStep();
    float mix=drums();
    for(auto& v:voices_) {
      if(!v.active) continue;
      const float env=v.envelope.Process(v.gate>0); if(v.gate) --v.gate;
      v.amplitude=env;
      // Reduce brightness with the envelope while retaining the FM attack.
      v.fm.SetIndex(v.depth*(.25f+.75f*env));
      const float sample=v.fm.Process(); v.filtered+=v.tone*(sample-v.filtered);
      mix+=v.filtered*env*v.gain*(song_.levels[v.track]/100.f)+v.tail; v.tail*=.995f;
      if(!v.envelope.IsRunning()&&fabsf(v.tail)<.00001f) v.active=false;
    }
    const uint32_t tap=(delayPos_+8192-uint32_t(sampleRate_*.185f))&8191;
    const float echo=delay_[tap]; delay_[delayPos_]=mix+echo*.35f; delayPos_=(delayPos_+1)&8191;
    mix+=echo*(song_.echo/100.f);
    // Remove DC, smooth gain and limit the mix before integer conversion.
    const float dc=mix-dcX_+.995f*dcY_; dcX_=mix; dcY_=dc;
    volume_+=.001f*(song_.master/100.f-volume_);
    const float scaled=dc*volume_, limited=scaled/(1.f+fabsf(scaled));
    const int16_t pcm=int16_t(limited*30000.f); output[2*n]=pcm; output[2*n+1]=pcm;
    peak=fmaxf(peak,fabsf(limited)); if(n<32) meter_.wave[n]=int8_t(limited*110.f);
  }
  for(const auto& v:voices_) if(v.active) ++active;
  meter_.voices=active; meter_.peak=fmaxf(peak,meter_.peak*.85f);
}
}
