#pragma once
#include "GameCore.h"
namespace pea {
struct Config { uint8_t difficulty=1; bool sound=true,led=true; uint32_t best[3]={}; };
struct Frame {
  State game{}; Config config{};
  uint8_t selected=0,option=0,batteryPercent=0;
  uint16_t batteryMv=0,powerMv=0;
  bool booting=true,active=false,menu=false,confirm=false,touchReady=false,audioReady=false;
  bool charging=false,full=false,inputStalled=false;
  uint32_t now=0;
  char notice[24]={};
};
void runtimeBegin();
void runtimeService();
bool runtimeFrame(Frame& frame);
void displayBegin();
}
