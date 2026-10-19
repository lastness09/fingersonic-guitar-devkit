// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <stdint.h>

// Initialize once before submitting frames; allocate one RMT DMA channel.
bool RingLedOutput_begin();

// Hold the same guard while drawing and submitting a complete LED frame.
class RingLedFrameGuard {
 public:
  RingLedFrameGuard();
  ~RingLedFrameGuard();
  explicit operator bool() const { return locked_; }
  RingLedFrameGuard(const RingLedFrameGuard&) = delete;
  RingLedFrameGuard& operator=(const RingLedFrameGuard&) = delete;

 private:
  bool locked_;
};

// Submit NUM_LEDS GRB pixels asynchronously; skip a frame when busy.
bool RingLedOutput_show(const uint8_t* grbPixels);

// Block further frames and complete a black frame before restarting.
void RingLedOutput_blackout();
