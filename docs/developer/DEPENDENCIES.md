# Public Dependencies and Version Baseline

Updated: 2026-10-13. Fix versions while developing, and revalidate USB enumeration, I2S, LED DMA, touch and display when upgrading.

| Component | Recommended baseline | Upstream |
|---|---|---|
| Arduino ESP32 Core | 3.3.2 | [Espressif release](https://github.com/espressif/arduino-esp32/releases/tag/3.3.2), based on IDF 5.5.1 |
| Adafruit MPR121 | 1.1.3 | [Adafruit](https://github.com/adafruit/Adafruit_MPR121) |
| Adafruit BusIO | 1.17.4 | [Adafruit](https://github.com/adafruit/Adafruit_BusIO) |
| Adafruit NeoPixel | 1.15.1 | [Adafruit](https://github.com/adafruit/Adafruit_NeoPixel) |
| Adafruit GFX | 1.12.6 | [Adafruit](https://github.com/adafruit/Adafruit-GFX-Library) |
| Adafruit ST7735/ST7789 | 1.11.0 | [Adafruit](https://github.com/adafruit/Adafruit-ST7735-Library) |
| U8g2 | 2.35.30 | [U8g2](https://github.com/olikraus/u8g2) |
| MIDI Library | 5.0.2 | [FortySevenEffects](https://github.com/FortySevenEffects/arduino_midi_library) |
| BLE-MIDI | 2.2 | [lathoub](https://github.com/lathoub/Arduino-BLE-MIDI) |
| Control Surface | 2.0.0 | [tttapa](https://github.com/tttapa/Control-Surface) |
| Pure IDF application | 5.5.1 suggested starting point | Align with the Arduino Core baseline, then validate your application |

Use Arduino IDE 2.x or Arduino CLI. Wire/SPI/FS/LittleFS/WiFi/WebServer/USB come with the Core; FreeRTOS/I2S/RMT with IDF. MPR121 examples require MPR121/BusIO; TFT examples require GFX/ST7735; raw I2S uses the platform driver.

```powershell
arduino-cli core update-index --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core install esp32:esp32@3.3.2 --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli lib install "Adafruit MPR121@1.1.3" "Adafruit BusIO@1.17.4" "Adafruit NeoPixel@1.15.1" "Adafruit GFX Library@1.12.6" "Adafruit ST7735 and ST7789 Library@1.11.0"
```

Install remaining libraries only if your application uses them. If an older version is missing from the index, obtain its upstream tag and retain version/license information. No private manufacturer project is required.

Choose SF2/WAV/MP3 engines separately. Check memory use, sample format, realtime behavior and distribution licensing. These recommendations do not claim every third-party application has been tested on hardware.
