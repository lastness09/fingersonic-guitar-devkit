#pragma once
#include <stdint.h>
namespace pea {
constexpr int lanes=6,maxBeans=96,maxEnemies=32,maxBursts=16,tickMs=20;
enum class Mode : uint8_t { Ready,Playing,Paused,Over };
enum class Kind : uint8_t { Walker,Runner,Helmet };
enum class Effect : uint8_t { Shot,Hit,Kill,Breach,Freeze,Bomb,Wave,Over };
struct Bean { uint16_t x; uint8_t lane; bool active; };
struct Enemy { uint16_t x,speed; uint8_t lane,hp,flash; Kind kind; bool active; };
struct Burst { uint8_t x,lane,life; bool big; };
struct Event { Effect effect; uint8_t lane; };
struct State {
  Mode mode=Mode::Ready;
  uint8_t difficulty=1,lives=3,wave=1,bombs=1,combo=0;
  uint32_t time=0,score=0,best=0,freezeUntil=0,freezeReadyAt=0,waveBannerUntil=0;
  uint16_t cooldown[lanes]={},shotFlash[lanes]={},hurtFlash[lanes]={};
  Bean beans[maxBeans]={}; Enemy enemies[maxEnemies]={}; Burst bursts[maxBursts]={};
};
class Game {
 public:
  void init(uint8_t difficulty,uint32_t best=0,uint32_t seed=0x18ca53);
  void start();
  void ready(uint8_t difficulty,uint32_t best);
  void pause();
  void update(uint16_t milliseconds);
  bool fire(int lane);
  bool freeze();
  bool bomb();
  bool popEvent(Event& event);
  const State& state() const { return state_; }
#ifdef PEA_HOST_TEST
  State& debugState() { return state_; }
#endif
 private:
  void tick();
  void spawn();
  void hit(Enemy& enemy,bool instant);
  void burst(int x,int lane,bool big);
  void emit(Effect effect,int lane=0);
  uint32_t random();
  State state_{};
  uint32_t rng_=0x18ca53,nextSpawn_=0,restUntil_=0,lastKill_=0;
  uint16_t accumulator_=0,remaining_=0,spawned_=0;
  Event events_[24]={}; uint8_t eventRead_=0,eventWrite_=0;
};
}
