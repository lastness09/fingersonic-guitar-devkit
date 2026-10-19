#pragma once
#include "GameCore.h"
namespace pea {
// Four-voice synthesized effects; host tests and hardware I2S use the same rendering code.
class SoundSynth {
 public:
  void trigger(Effect effect);
  void render(int16_t* stereo,int frames);
  void clear();
 private:
  struct Voice { uint32_t phase=0,increment=0,total=0,remaining=0; int32_t sweep=0; bool noise=false; };
  Voice voices_[4]; uint32_t noise_=0x735ae214;
};
void soundBegin();
void soundActive(bool active);
void soundEnabled(bool enabled);
void soundPush(const Event& event);
bool soundReady();
}
