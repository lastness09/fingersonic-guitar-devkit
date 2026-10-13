// SPDX-License-Identifier: Apache-2.0
#pragma once
// Declare system CRT functions for the minimal Windows host-test build.
#define M_E 2.71828182845904523536
extern "C" {
float sinf(float); float floorf(float); float powf(float,float);
float expf(float); float logf(float);
float fmaxf(float,float); float fminf(float,float);
}
inline float fabsf(float value) { return value<0?-value:value; }
