#include "../GameCore.h"
#include "../GameSound.h"
#include "../config.h"
#include <stddef.h>
#include <stdarg.h>
extern "C" int __stdio_common_vsprintf(unsigned long long,char*,size_t,const char*,void*,va_list);
extern "C" int snprintf(char* buffer,size_t size,const char* format,...) {
  va_list args; va_start(args,format);
  int result=__stdio_common_vsprintf(2,buffer,size,format,nullptr,args); va_end(args); return result;
}
void* operator new(size_t,void* memory) { return memory; }
extern "C" { int _fltused=0; }
namespace {
alignas(pea::Game) unsigned char storage[sizeof(pea::Game)];
pea::Game* game;
void start() { game=new(storage) pea::Game(); game->init(1,42); game->start(); }
int beanCount() { int count=0; for(auto& b:game->state().beans) count+=b.active; return count; }
void advance(int milliseconds) { while(milliseconds>0) { int dt=milliseconds>100?100:milliseconds; game->update(dt); milliseconds-=dt; } }
}
#define EXPORT extern "C" __declspec(dllexport)
EXPORT int pea_tests() {
  using namespace pea;
  const uint8_t channels[]={7,6,4,3,0,1};
  for(int lane=0;lane<6;++lane) if(hw::strings[lane]!=channels[lane]) return 27;
  start(); if(game->state().lives!=3||game->state().best!=42) return 1;
  for(int l=0;l<6;++l) if(!game->fire(l)) return 2;
  if(beanCount()!=6||game->fire(2)||game->fire(-1)||game->fire(6)) return 3;
  for(int l=0;l<6;++l) if(game->state().beans[l].lane!=l) return 4;
  game->update(140); // A single update catches up at most 100 ms, so one call cannot advance 140 ms.
  if(game->fire(2)) return 5;
  game->update(40); if(!game->fire(2)) return 6;
  game->pause(); const uint32_t time=game->state().time;
  advance(2000); if(game->state().time!=time||game->fire(0)) return 7;
  game->pause(); game->update(20); if(game->state().time!=time+20) return 8;
  start(); auto& s=game->debugState();
  s.enemies[0]={uint16_t(30*256),0,2,1,0,Kind::Walker,true};
  game->fire(1); advance(100); if(!s.enemies[0].active) return 9;
  game->fire(2); advance(100); if(s.enemies[0].active||s.score!=10) return 10;
  start(); game->debugState().enemies[0]={uint16_t(30*256),0,0,3,0,Kind::Helmet,true};
  for(int shot=0;shot<3;++shot) {
    if(!game->fire(0)) return 11;
    advance(140);
    if(shot<2&&game->state().enemies[0].hp!=2-shot) return 12;
  }
  if(game->state().enemies[0].active||game->state().score!=30) return 13;
  start(); game->debugState().enemies[0]={uint16_t(100*256),40,0,1,0,Kind::Walker,true};
  if(!game->freeze()||game->freeze()) return 14;
  advance(100); if(game->state().enemies[0].x!=100*256-50) return 15;
  advance(2400); if(game->state().time<game->state().freezeUntil) return 16;
  start();
  game->debugState().enemies[0]={uint16_t(100*256),40,0,3,0,Kind::Helmet,true};
  game->debugState().enemies[1]={uint16_t(100*256),40,1,1,0,Kind::Walker,true};
  if(!game->bomb()||game->bomb()||game->state().bombs||game->state().score<40) return 17;
  if(game->state().enemies[0].active||game->state().enemies[1].active) return 18;
  start();
  for(int i=0;i<3;++i) game->debugState().enemies[i]={uint16_t(23*256),40,uint8_t(i),1,0,Kind::Walker,true};
  game->update(20); if(game->state().mode!=Mode::Over||game->state().lives||game->fire(0)) return 19;
  game->ready(2,345); if(!game->fire(5)||game->state().mode!=Mode::Playing||game->state().difficulty!=2||game->state().best!=345) return 20;
  start(); for(auto& b:game->debugState().beans) b={10000,0,true};
  if(game->fire(1)||beanCount()!=maxBeans) return 21;
  start();
  for(auto& enemy:game->debugState().enemies) enemy={uint16_t(150*256),0,0,3,0,Kind::Helmet,true};
  advance(1000); for(auto& e:game->state().enemies) if(!e.active||e.hp!=3) return 22;
  start();
  for(int i=0;i<160;++i) { game->update(100); for(auto& e:game->debugState().enemies) e.active=false; }
  if(game->state().wave<2||game->state().bombs<2) return 23;
  start();
  for(int i=0;i<10000;++i) {
    if(game->state().mode==Mode::Over) start();
    game->fire(i%6); if(i%50==0) game->freeze(); if(i%300==0) game->bomb(); game->update(20);
    if(game->state().lives>3) return 24;
    for(auto& e:game->state().enemies) if(e.active&&(e.lane>=6||e.x>160*256||!e.hp)) return 25;
    for(auto& b:game->state().beans) if(b.active&&(b.lane>=6||b.x>=160*256)) return 26;
  }
  return 0;
}
EXPORT int pea_sound_tests() {
  pea::SoundSynth synth; int16_t pcm[512];
  for(int e=0;e<8;++e) {
    synth.clear(); synth.trigger(pea::Effect(e)); int peak=0;
    for(int block=0;block<10;++block) {
      synth.render(pcm,256);
      for(int i=0;i<512;++i) { int value=pcm[i]<0?-pcm[i]:pcm[i]; if(value>peak) peak=value; }
    }
    if(peak<100||peak>30000) return e+1;
  }
  synth.clear(); synth.render(pcm,256); for(int sample:pcm) if(sample) return 10;
  return 0;
}
