// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "GrooveCore.h"

namespace groove {
enum Page : uint8_t { Live, Grid, Sound, Mix, Setup };
enum StartupStage : uint8_t { BootLogo, TouchInit, AudioInit, Running };
struct View {
  Song song{};
  Meter meter{};
  Page page=Live;
  uint8_t track=Lead, step=0, parameter=0, option=0;
  int8_t chord=-1;
  bool active=false, booting=true, touchReady=false, audioReady=false, confirm=false;
  bool inputStalled=false, audioFault=false;
  StartupStage startup=BootLogo;
  uint16_t batteryMv=0, powerMv=0, cpuPercent=0;
  uint8_t batteryPercent=0;
  bool charging=false, full=false;
  uint32_t dropped=0, underruns=0;
  char notice[24]={};
};
extern const char* const chordNames[8];
extern const char* const optionNames[];
constexpr int optionCount=12;
void runtimeBegin();
void runtimeService();
bool runtimeView(View& view);
void displayBegin();
}
