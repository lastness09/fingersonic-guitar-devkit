#include "GameCore.h"
#include <string.h>
namespace pea {
static uint32_t smaller(uint32_t a,uint32_t b) { return a<b?a:b; }
void Game::init(uint8_t difficulty,uint32_t best,uint32_t seed) {
  rng_=seed?seed:1; ready(difficulty,best);
}
void Game::ready(uint8_t difficulty,uint32_t best) {
  state_=State{}; state_.difficulty=difficulty>2?2:difficulty; state_.best=best;
  accumulator_=0; remaining_=0; eventRead_=eventWrite_=0;
}
void Game::start() {
  const uint8_t difficulty=state_.difficulty; const uint32_t best=state_.best;
  ready(difficulty,best); state_.mode=Mode::Playing;
  remaining_=8; spawned_=0; nextSpawn_=600; restUntil_=0; lastKill_=0;
  state_.waveBannerUntil=900; emit(Effect::Wave);
}
void Game::pause() {
  if(state_.mode==Mode::Playing) state_.mode=Mode::Paused;
  else if(state_.mode==Mode::Paused) state_.mode=Mode::Playing;
  accumulator_=0; // Do not accumulate paused time or fast-forward on resume.
}
uint32_t Game::random() { rng_^=rng_<<13; rng_^=rng_>>17; rng_^=rng_<<5; return rng_; }
void Game::emit(Effect effect,int lane) {
  const uint8_t next=(eventWrite_+1)%24;
  if(next==eventRead_) return; // A full sound queue may drop an effect but must not block game events.
  events_[eventWrite_]={effect,uint8_t(lane)}; eventWrite_=next;
}
bool Game::popEvent(Event& event) {
  if(eventRead_==eventWrite_) return false;
  event=events_[eventRead_]; eventRead_=(eventRead_+1)%24; return true;
}
bool Game::fire(int lane) {
  if(lane<0||lane>=lanes) return false;
  if(state_.mode==Mode::Ready) start();
  if(state_.mode!=Mode::Playing||state_.cooldown[lane]) return false;
  for(auto& bean:state_.beans) if(!bean.active) {
    bean={uint16_t(24*256),uint8_t(lane),true};
    state_.cooldown[lane]=140; state_.shotFlash[lane]=120; emit(Effect::Shot,lane); return true;
  }
  return false;
}
bool Game::freeze() {
  if(state_.mode!=Mode::Playing||state_.time<state_.freezeReadyAt) return false;
  state_.freezeUntil=state_.time+2500; state_.freezeReadyAt=state_.time+8000;
  emit(Effect::Freeze); return true;
}
void Game::burst(int x,int lane,bool big) {
  Burst* slot=&state_.bursts[0];
  for(auto& b:state_.bursts) { if(!b.life) { slot=&b; break; } if(b.life<slot->life) slot=&b; }
  *slot={uint8_t(x),uint8_t(lane),uint8_t(big?15:7),big};
}
void Game::hit(Enemy& enemy,bool instant) {
  if(!enemy.active) return;
  enemy.flash=5;
  if(!instant&&enemy.hp>1) { --enemy.hp; burst(enemy.x/256,enemy.lane,false); emit(Effect::Hit,enemy.lane); return; }
  enemy.active=false;
  state_.combo=state_.time-lastKill_<=1100?uint8_t(smaller(8,state_.combo+1)):1;
  lastKill_=state_.time;
  const int points=enemy.kind==Kind::Helmet?30:(enemy.kind==Kind::Runner?15:10);
  state_.score=smaller(99999,state_.score+points+(state_.combo-1)*2);
  if(state_.score>state_.best) state_.best=state_.score;
  burst(enemy.x/256,enemy.lane,true); emit(Effect::Kill,enemy.lane);
}
bool Game::bomb() {
  if(state_.mode!=Mode::Playing||!state_.bombs) return false;
  --state_.bombs;
  for(auto& enemy:state_.enemies) if(enemy.active) hit(enemy,true);
  emit(Effect::Bomb); return true;
}
void Game::spawn() {
  Enemy* slot=nullptr;
  for(auto& enemy:state_.enemies) if(!enemy.active) { slot=&enemy; break; }
  if(!slot) return; // Defer spawning when the fixed pool is full; do not replace active enemies.
  const uint32_t r=random();
  const int lane=spawned_<6?(spawned_+state_.wave-1)%6:r%6;
  Kind kind=Kind::Walker;
  if(state_.wave>=3&&(r>>8)%4==0) kind=Kind::Helmet;
  else if(state_.wave>=4&&(r>>12)%4==0) kind=Kind::Runner;
  const int base=6+state_.difficulty*2+smaller(8,(state_.wave-1)/2);
  const int speed=kind==Kind::Runner?base+5:(kind==Kind::Helmet?base-2:base);
  *slot={uint16_t(152*256),uint16_t(speed*256*tickMs/1000),uint8_t(lane),uint8_t(kind==Kind::Helmet?3:1),0,kind,true};
  --remaining_; ++spawned_;
  const int spacing=state_.difficulty==0?1700:(state_.difficulty==1?1400:1100);
  const int interval=spacing-int(smaller(850,(state_.wave-1)*90));
  nextSpawn_=state_.time+interval;
}
void Game::tick() {
  state_.time+=tickMs;
  if(state_.time-lastKill_>1100) state_.combo=0;
  for(int l=0;l<lanes;++l) {
    state_.cooldown[l]=state_.cooldown[l]>tickMs?state_.cooldown[l]-tickMs:0;
    state_.shotFlash[l]=state_.shotFlash[l]>tickMs?state_.shotFlash[l]-tickMs:0;
    state_.hurtFlash[l]=state_.hurtFlash[l]>tickMs?state_.hurtFlash[l]-tickMs:0;
  }
  for(auto& b:state_.bursts) if(b.life) --b.life;
  if(remaining_&&state_.time>=nextSpawn_) spawn();
  bool alive=false;
  for(auto& enemy:state_.enemies) {
    if(!enemy.active) continue;
    const int step=state_.time<state_.freezeUntil?enemy.speed/4:enemy.speed;
    enemy.x=enemy.x>step?enemy.x-step:0; if(enemy.flash) --enemy.flash;
    if(enemy.x<=23*256) {
      enemy.active=false; state_.hurtFlash[enemy.lane]=500;
      if(state_.lives) --state_.lives;
      emit(Effect::Breach,enemy.lane);
      if(!state_.lives) { state_.mode=Mode::Over; emit(Effect::Over); return; }
    } else alive=true;
  }
  for(auto& bean:state_.beans) {
    if(!bean.active) continue;
    const int old=bean.x; bean.x+=384; // Advance 1.5 pixels per 20 ms, or 75 pixels per second.
    Enemy* target=nullptr;
    for(auto& enemy:state_.enemies) {
      if(enemy.active&&enemy.lane==bean.lane&&bean.x>=enemy.x-256&&old<=enemy.x+8*256)
        if(!target||enemy.x<target->x) target=&enemy;
    }
    // Test swept intervals so fast peas cannot pass through targets.
    if(target) { bean.active=false; hit(*target,false); }
    else if(bean.x>=160*256) bean.active=false;
  }
  if(!remaining_) {
    alive=false; for(const auto& enemy:state_.enemies) alive|=enemy.active;
    if(!alive) {
      if(!restUntil_) restUntil_=state_.time+1500;
      if(state_.time>=restUntil_) {
        state_.wave=uint8_t(smaller(99,state_.wave+1)); remaining_=8+smaller(24,(state_.wave-1)*2);
        spawned_=0; nextSpawn_=state_.time+700; restUntil_=0;
        state_.bombs=uint8_t(smaller(3,state_.bombs+1)); state_.waveBannerUntil=state_.time+1000;
        emit(Effect::Wave);
      }
    }
  }
}
void Game::update(uint16_t milliseconds) {
  if(state_.mode!=Mode::Playing) return;
  // Catch up at most five frames, avoiding instant lane crossings after a stall.
  accumulator_+=milliseconds>100?100:milliseconds;
  while(accumulator_>=tickMs&&state_.mode==Mode::Playing) { accumulator_-=tickMs; tick(); }
}
}
