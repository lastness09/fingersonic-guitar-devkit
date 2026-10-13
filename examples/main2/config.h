// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <stdint.h>
#define LED_PIN 48
#define NUM_LEDS 16
// Self-contained board definitions for this FM Studio reference application.
namespace hw {
constexpr int tftClock=13, tftMosi=14, tftCs=10, tftDc=18, tftReset=21, tftBacklight=9;
constexpr int battery=1, charge=2, standby=4, power=8;
constexpr int i2sClock=40, i2sData=41, i2sWord=42;
constexpr int sda=11, scl=12, encoderA=38, encoderB=39, encoderButton=47;
constexpr int joystickX=6, joystickY=5, joystickButton=7;
constexpr int led=48;
constexpr uint8_t strings[6]={1,0,3,4,6,7}; // Logical string order: string 6 through string 1.
constexpr uint8_t kickPad=9, snarePad=8, bendUp=5, bendDown=2;
constexpr int powerOnMv=2600, powerOffMv=2400;
constexpr uint32_t sampleRate=32000; // 32 kHz leaves processing time for input and display tasks.
constexpr int audioBlock=128, dmaBuffers=4, dmaFrames=128;
}
