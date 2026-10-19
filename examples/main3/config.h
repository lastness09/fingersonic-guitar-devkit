#pragma once
#include <stdint.h>
#define LED_PIN 48
#define NUM_LEDS 16
// Use the existing S3 wiring without private instrument application modules.
namespace hw {
constexpr int tftClock=13,tftMosi=14,tftCs=10,tftDc=18,tftReset=21,tftBacklight=9;
constexpr int i2sClock=40,i2sData=41,i2sWord=42;
constexpr int sda=11,scl=12,encoderA=38,encoderB=39,encoderButton=47;
constexpr int joystickX=6,joystickY=5,joystickButton=7;
constexpr int battery=1,charge=2,standby=4,power=8,reservedOutput=17;
constexpr uint8_t strings[6]={7,6,4,3,0,1}; // Game lane order, top to bottom: physical strings 1, 2, 3, 4, 5, 6.
constexpr int freezePad=8,bombPad=9,laneUpPad=5,laneDownPad=2;
constexpr int powerOnMv=2600,powerOffMv=2400;
constexpr int sampleRate=24000,audioBlock=128;
}
