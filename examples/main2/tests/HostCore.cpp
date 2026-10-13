// SPDX-License-Identifier: Apache-2.0
#include "../GrooveCore.h"
#include <stddef.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>
#include "Export.h"
#ifdef GROOVE_MINIMAL_WINDOWS_CRT
extern "C" int __stdio_common_vsprintf(unsigned long long,char*,size_t,const char*,void*,va_list);
extern "C" int snprintf(char* buffer,size_t size,const char* format,...) {
  va_list args; va_start(args,format);
  const int result=__stdio_common_vsprintf(2,buffer,size,format,nullptr,args);
  va_end(args); return result;
}
void* operator new(size_t,void* memory) { return memory; }
extern "C" { int _fltused=0; }
#else
#include <new>
#endif
namespace {
alignas(groove::Engine) unsigned char memory[sizeof(groove::Engine)];
groove::Engine* engine;
int16_t pcm[512];
void start(const groove::Song& song) {
  engine=new(memory) groove::Engine(); engine->init(32000,song);
}
void run(int samples) {
  while(samples>0) { int n=samples>256?256:samples; engine->render(pcm,n); samples-=n; }
}
}
#define EXPORT GROOVE_EXPORT
EXPORT int fm_tests() {
  using namespace groove;
  Song song; defaults(song);
  if(song.bpm!=110||song.patches[1].preset!=0||trackCount!=5) return 1;
  song.bpm=65535; song.echo=255; song.patches[0].values[Ratio]=0; sanitize(song);
  if(song.bpm!=300||song.echo!=60||song.patches[0].values[Ratio]!=1) return 2;
  if(fabsf(midiFrequency(69)-440.f)>.001f) return 3;
  daisysp::Oscillator::InitSineTable();
  for(int i=-10000;i<10000;++i) {
    const float phase=i/317.f;
    if(fabsf(daisysp::Oscillator::Sine(phase)-sinf(phase*6.28318530718f))>.00003f) return 4;
  }
  defaults(song); song.bpm=120; song.echo=0;
  for(auto& bank:song.steps) for(int t=0;t<trackCount;++t) for(auto& step:bank[t]) step.enabled=t==Kick;
  start(song); engine->command({Action::Transport}); run(64000);
  if(engine->meter().triggers[Kick]!=16||engine->meter().step!=15) return 5;
  engine->command({Action::Bank,0,0,0,2}); run(1);
  if(engine->song().bank!=2||engine->meter().step!=0) return 6;
  engine->command({Action::Transport}); const uint32_t count=engine->meter().triggers[Kick]; run(16000);
  if(engine->meter().triggers[Kick]!=count) return 7;
  defaults(song); song.bpm=120; song.swing=50;
  for(auto& bank:song.steps) for(int t=0;t<trackCount;++t) for(auto& step:bank[t]) step.enabled=t==Kick;
  start(song); engine->command({Action::Transport}); run(6000);
  if(engine->meter().triggers[Kick]!=1) return 8;
  run(2000); if(engine->meter().triggers[Kick]!=2) return 9;
  run(1); if(engine->meter().triggers[Kick]!=3) return 10;
  defaults(song); song.echo=0; start(song); engine->command({Action::Record});
  engine->command({Action::Note,Lead,110,10,67,400}); run(128);
  if(!engine->song().steps[0][Lead][0].enabled||engine->song().steps[0][Lead][0].note!=67) return 11;
  for(int p=0;p<presetCount;++p) {
    defaults(song); song.echo=0; song.patches[1]=preset(p); start(song);
    for(int v=0;v<voiceCount;++v) engine->command({Action::Note,Lead,127,uint8_t(10+v),int16_t(48+v*3),500});
    int peak=0;
    for(int block=0;block<50;++block) {
      engine->render(pcm,256);
      for(int i=0;i<512;++i) { int a=pcm[i]<0?-pcm[i]:pcm[i]; if(a>peak) peak=a; }
    }
    if(peak<100||peak>=30000) return 12+p;
    engine->command({Action::Panic}); run(32000);
    if(engine->meter().voices!=0) return 20+p;
  }
  return 0;
}
EXPORT void fm_demo(int presetIndex) {
  groove::Song song; groove::defaults(song); song.patches[1]=groove::preset(presetIndex); start(song);
  engine->command({groove::Action::Transport});
}
EXPORT void fm_render(int16_t* output,int frames) { engine->render(output,frames); }
