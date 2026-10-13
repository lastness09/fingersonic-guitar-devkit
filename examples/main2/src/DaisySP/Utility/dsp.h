/*
Copyright (c) 2020 Electrosmith, Corp, Emilie Gillet.
MIT license: see ../LICENSE.
Fingersonic adaptation: retain only the utilities used by the vendored oscillator.
*/
#pragma once
#include <stdint.h>
#include <math.h>
#define PI_F 3.1415927410125732421875f
#define TWOPI_F (2.0f * PI_F)
namespace daisysp {
inline float fclamp(float value, float low, float high) {
    return value < low ? low : (value > high ? high : value);
}
inline float fastmod1f(float value) { return value - floorf(value); }
}
