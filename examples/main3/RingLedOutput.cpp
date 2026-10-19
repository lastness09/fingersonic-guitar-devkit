// SPDX-License-Identifier: Apache-2.0
#include "RingLedOutput.h"

#include <string.h>
#include <driver/rmt_tx.h>
#include <esp_attr.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "config.h"

namespace {
constexpr size_t kFrameBytes = NUM_LEDS * 3;
constexpr size_t kFrameSymbols = kFrameBytes * 8 + 1;
constexpr size_t kDmaSymbols = 1024;
// One DMA half-buffer holds the complete 16-pixel frame, reset and EOF.
static_assert(kFrameSymbols + 1 <= kDmaSymbols / 2,
              "LED frame must fit in one DMA half-buffer");
DRAM_ATTR rmt_symbol_word_t frameSymbols[kFrameSymbols];
DRAM_ATTR uint8_t lastFrame[kFrameBytes];
rmt_channel_handle_t txChannel = nullptr;
rmt_encoder_handle_t copyEncoder = nullptr;
SemaphoreHandle_t frameMutex = nullptr;
bool ready = false;
bool stopped = false;
bool inFlight = false;
bool lastFrameValid = false;
const char* const kTag = "RingLED";

bool waitForFrame(int timeoutMs) {
  if (!inFlight) return true;
  const esp_err_t err = rmt_tx_wait_all_done(txChannel, timeoutMs);
  if (err == ESP_OK) {
    inFlight = false;
    return true;
  }
  if (err != ESP_ERR_TIMEOUT) {
    ESP_LOGE(kTag, "DMA completion failed: %s", esp_err_to_name(err));
  }
  return false;
}

bool submitFrame(const uint8_t* pixels) {
  // Do not change the transmission buffer until the previous frame completes.
  if (!waitForFrame(0)) return false;
  if (lastFrameValid && memcmp(lastFrame, pixels, kFrameBytes) == 0) {
    return true;
  }

  size_t index = 0;
  for (size_t byte = 0; byte < kFrameBytes; ++byte) {
    for (uint8_t mask = 0x80; mask != 0; mask >>= 1) {
      const bool one = (pixels[byte] & mask) != 0;
      rmt_symbol_word_t& symbol = frameSymbols[index++];
      // Encode GRB, MSB first, with the board timing at 10 MHz.
      symbol.level0 = 1;
      symbol.duration0 = one ? 8 : 4;
      symbol.level1 = 0;
      symbol.duration1 = one ? 4 : 8;
    }
  }
  // Include the 300 us low latch in the same DMA transaction.
  frameSymbols[index].level0 = 0;
  frameSymbols[index].duration0 = 1500;
  frameSymbols[index].level1 = 0;
  frameSymbols[index].duration1 = 1500;

  rmt_transmit_config_t config = {};
  config.flags.queue_nonblocking = true;
  const esp_err_t err = rmt_transmit(txChannel, copyEncoder, frameSymbols,
                                    sizeof(frameSymbols), &config);
  if (err != ESP_OK) {
    lastFrameValid = false;
    ESP_LOGE(kTag, "DMA submit failed: %s", esp_err_to_name(err));
    return false;
  }
  memcpy(lastFrame, pixels, kFrameBytes);
  lastFrameValid = true;
  inFlight = true;
  return true;
}
}  // namespace

bool RingLedOutput_begin() {
  if (ready) return true;
  if (!frameMutex) frameMutex = xSemaphoreCreateMutex();
  if (!frameMutex) {
    ESP_LOGE(kTag, "Frame mutex allocation failed");
    return false;
  }

  rmt_tx_channel_config_t config = {};
  config.gpio_num = static_cast<gpio_num_t>(LED_PIN);
  config.clk_src = RMT_CLK_SRC_DEFAULT;
  config.resolution_hz = 10000000;
  config.mem_block_symbols = kDmaSymbols;
  config.trans_queue_depth = 1;
  config.flags.with_dma = true;
  esp_err_t err = rmt_new_tx_channel(&config, &txChannel);
  if (err == ESP_OK) {
    rmt_copy_encoder_config_t encoderConfig = {};
    err = rmt_new_copy_encoder(&encoderConfig, &copyEncoder);
  }
  if (err == ESP_OK) err = rmt_enable(txChannel);
  if (err != ESP_OK) {
    if (copyEncoder) rmt_del_encoder(copyEncoder);
    if (txChannel) rmt_del_channel(txChannel);
    copyEncoder = nullptr;
    txChannel = nullptr;
    ESP_LOGE(kTag, "DMA initialization failed: %s", esp_err_to_name(err));
    return false;
  }
  ready = true;
  ESP_LOGI(kTag, "GPIO%d DMA ready, %u LEDs, %u symbols/frame",
           LED_PIN, NUM_LEDS, static_cast<unsigned>(kFrameSymbols));
  return true;
}

RingLedFrameGuard::RingLedFrameGuard() : locked_(false) {
  if (!ready || xSemaphoreTake(frameMutex, 0) != pdTRUE) return;
  if (stopped) {
    xSemaphoreGive(frameMutex);
    return;
  }
  locked_ = true;
}

RingLedFrameGuard::~RingLedFrameGuard() {
  if (locked_) xSemaphoreGive(frameMutex);
}

bool RingLedOutput_show(const uint8_t* grbPixels) {
  return ready && !stopped && grbPixels && submitFrame(grbPixels);
}

void RingLedOutput_blackout() {
  if (!ready) return;
  if (xSemaphoreTake(frameMutex, pdMS_TO_TICKS(50)) != pdTRUE) {
    ESP_LOGE(kTag, "Blackout frame lock timeout");
    return;
  }
  stopped = true;
  const uint8_t black[kFrameBytes] = {};
  // Only blackout waits briefly; ordinary animation submissions never block.
  if (!waitForFrame(50) || !submitFrame(black) || !waitForFrame(50)) {
    ESP_LOGE(kTag, "Blackout frame did not complete");
  }
  xSemaphoreGive(frameMutex);
}
