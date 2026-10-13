# Public Peripheral API Examples

These snippets use public Arduino/Adafruit libraries and standard ESP-IDF drivers. Integrate them into your own project, organize initialization/tasks and handle errors. They are interface examples, not complete firmware. See the [complete guide](../../FIRMWARE_DEVELOPMENT_GUIDE.md).

## 1. Shared ADC and calibrated voltage

Use one ADC mutex for GPIO1/5/6/8 configuration and reads. The following helper functions are examples you define yourself, with no private module dependency:

```cpp
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
SemaphoreHandle_t adcLock = nullptr;
bool setupAdc() {
  adcLock = xSemaphoreCreateMutex();
  if (!adcLock) return false;
  analogReadResolution(12);
  for (uint8_t pin : {1,5,6,8}) analogSetPinAttenuation(pin, ADC_11db);
  pinMode(8, INPUT_PULLDOWN);
  return true;
}
bool readAdcMillivolts(uint8_t pin, uint32_t& mv) {
  // Non-blocking: skip this sample when busy; do not reuse old values.
  if (!adcLock || xSemaphoreTake(adcLock,0)!=pdTRUE) return false;
  mv = analogReadMilliVolts(pin);
  bool valid = mv!=0 || (pin==8 && digitalRead(8)==LOW);
  xSemaphoreGive(adcLock);
  return valid;
}
```

Multiply GPIO1 millivolts by 3 for battery voltage. For GPIO8, failed/busy samples do not accumulate confirmation time. Zero is ground only when digital LOW;≥ 2600 mV for 100 ms enables runtime and≤ 2600 mV for 200 ms requests shutdown after initialization.

Pure IDF uses adc_oneshot_io_to_channel, ADC1/12-bit/ADC_ATTEN_DB_12 and curve-fitting calibration. Protect all participating channels with the same mutex. Do not estimate calibrated thresholds with `raw*3300/4095`.

## 2. MPR121 initialization and thresholds

Put declarations at file scope and initialization statements in setup or an initialization function:

```cpp
#include <Wire.h>
#include <Adafruit_MPR121.h>
Adafruit_MPR121 sensor;
// Initialization:
Wire.begin(11, 12);
delay(20); // Initialization wait only; not in the realtime scan.
bool found = false;
for (uint8_t address=0x5A; address<=0x5D && !found; ++address)
  found = sensor.begin(address, &Wire);
if (found) {
  Wire.setClock(400000);
  for (uint8_t e=0; e<12; ++e) {
    bool slider = e==2 || e==5 || e==8 || e==9;
    uint8_t touchThreshold = e>=10 ? 12 : slider ? 4 : 7;
    uint8_t releaseThreshold = e>=10 ? 6 : slider ? 4 : 7;
    sensor.writeRegister(0x41+2*e, touchThreshold);
    sensor.writeRegister(0x42+2*e, releaseThreshold);
  }
}
```

Read only after successful initialization:

```cpp
if (found) {
  uint16_t bits = sensor.touched() & 0x0FFF;
  bool pitchLeft  = bits & (1u<<2); // J, down
  bool pitchRight = bits & (1u<<5); // I, up
  bool tempoLeft  = bits & (1u<<9); // O, Tempo
  bool meterRight = bits & (1u<<8); // N, Meter
  uint16_t filtered = sensor.filteredData(2);
  uint16_t baseline = sensor.baselineData(2);
  int delta = int(baseline) - int(filtered);
}
```

Compare the mask with a previous mask to calculate press/release edges, then apply confirmation/retrigger rules. baselineData already restores the comparable scale. An initialization/read failure is not a valid high-velocity touch event.

## 3. Native I2S output

Arduino Core 3.3.2 and matching IDF 5.x provide this driver. Use setup in Arduino or app_main in pure IDF, and install only one driver on the controller:

```cpp
#include <driver/i2s_std.h>
i2s_chan_handle_t tx = nullptr;
i2s_chan_config_t channel = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
channel.dma_desc_num = 2;
channel.dma_frame_num = 256;
ESP_ERROR_CHECK(i2s_new_channel(&channel, &tx, nullptr));
i2s_std_config_t cfg = {};
cfg.clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(44100);
cfg.slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
    I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);
cfg.gpio_cfg.mclk = I2S_GPIO_UNUSED;
cfg.gpio_cfg.bclk = GPIO_NUM_40;
cfg.gpio_cfg.ws   = GPIO_NUM_42;
cfg.gpio_cfg.dout = GPIO_NUM_41;
cfg.gpio_cfg.din  = I2S_GPIO_UNUSED;
ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx, &cfg));
ESP_ERROR_CHECK(i2s_channel_enable(tx));
```

Continuously submit 256 stereo frames, 1024 bytes per block:

```cpp
int16_t pcm[256*2] = {}; // Silence; your engine fills L0,R0,L1,R1,...
size_t written = 0;
esp_err_t err = i2s_channel_write(tx, pcm, sizeof(pcm), &written, 1000);
if (err != ESP_OK || written != sizeof(pcm)) {
  // Report errors/short writes and retain the correct submission position.
}
```

Reuse an existing I2S instance rather than installing another. In pure C, use NULL and C structure initialization. Supply silence as well as sound. Complete players/samplers also need decoding, buffering, resampling, gain, limiting and task organization.

## 4. ST7735S initialization

```cpp
#include <SPI.h>
#include <Adafruit_ST7735.h>
class BoardDisplay : public Adafruit_ST7735 {
 public:
  BoardDisplay() : Adafruit_ST7735(10,18,21) {}
  void setBoardOffsets() { setColRowStart(24,0); }
};
BoardDisplay display;
// Initialization:
SPI.begin(13,-1,14,10);
pinMode(9,OUTPUT); digitalWrite(9,LOW);
display.initR(INITR_MINI160x80);
display.setBoardOffsets();
display.setRotation(3);
const uint8_t madctl = ST77XX_MADCTL_MX | ST77XX_MADCTL_MV | ST7735_MADCTL_BGR;
display.sendCommand(ST77XX_MADCTL,&madctl,1);
display.invertDisplay(false);
display.fillScreen(ST77XX_BLACK);
digitalWrite(9,HIGH);
```

Implement battery/power pages in your application. One task owns the display. Render complete changed content in memory before transferring it; do not let multiple objects/tasks write the same screen.

## 5. Enter the existing App1

```cpp
#include <esp_ota_ops.h>
#include <esp_system.h>
const esp_partition_t* p = esp_partition_find_first(
    ESP_PARTITION_TYPE_APP,ESP_PARTITION_SUBTYPE_APP_OTA_1,nullptr);
if (p && esp_ota_set_boot_partition(p)==ESP_OK) {
  // Save data, release sound, stop input, and complete a black LED frame first.
  esp_restart();
} else {
  // Stay in App0 and show an error instead of restarting blindly.
}
```

The selection API validates the target image; your controls/protocol must decide when to call it. This only boots preinstalled App1. All user uploads then use its Wi-Fi HTTP interface for App0 BIN or SF2 resources.
