# Fingersonic GPIO and Electrode Reference

Single ESP32-S3/ST7735S color-display hardware · Updated: 2026-10-13

See the [development guide](FIRMWARE_DEVELOPMENT_GUIDE.md) for complete GPIO reservations, power behavior and public API examples. GPIO numbers refer to the MCU; E numbers refer to MPR121 electrodes.

## Audio, touch and controls

| Peripheral | Signal | GPIO | Connection/behavior |
|---|---|---:|---|
| I2S | BCLK | 40 | DAC BCLK |
| I2S | DOUT | 41 | DAC DIN |
| I2S | WS/LRCLK | 42 | DAC LCK/LRCLK |
| LED ring | DIN | 48 | 16 WS2812-type LEDs, GRB |
| MPR121 | SDA | 11 | I2C, 3.3 V pullups |
| MPR121 | SCL | 12 | I2C, 400 kHz |
| Encoder | A/CLK | 38 | Input pullup |
| Encoder | B/DT | 39 | Input pullup |
| Encoder | SW | 47 | Pressed LOW; cold-start recovery input |
| Joystick | X | 6 | ADC |
| Joystick | Y | 5 | ADC |
| Joystick | SW | 7 | Pressed LOW |

Native USB uses GPIO19/20. GPIO26–37 are reserved for Flash/OPI PSRAM. GPIO16/17 and other boot/platform signals are not user expansion interfaces.

## Display and power

| Peripheral | Signal | GPIO | Connection/behavior |
|---|---|---:|---|
| TFT | SCLK | 13 | SPI clock |
| TFT | MOSI/SDA | 14 | SPI data, write-only |
| TFT | CS | 10 | Chip select |
| TFT | DC/RS | 18 | Command/data |
| TFT | RESET | 21 | Active LOW |
| TFT | BL | 9 | HIGH enables backlight circuit |
| Battery | BAT_ADC | 1 | Midpoint of 200k/100k divider |
| Charger | CHRG | 2 | Digital status input |
| Charger | STDBY | 4 | Digital status input |
| Soft power | State | 8 | ADC/pulldown; not a power-control output |

Use 3.3 V logic and a common ground. Do not connect raw battery voltage or5V to MCU GPIOs. The backlight pin controls its drive circuit, not an assumed GPIO power supply for an arbitrary bare panel.

## Slider groups and touch strings

![Upright pad mapping](docs/developer/assets/sliders-electrodes.png)

Upright front view, logo at the top: Pitch left Down/E2, right Up/E5; Strum left Tempo/E9, right Meter/E8. Horizontal TOP/BOTTOM naming rotates to right/left in this view.

| MPR121 electrode | Function |
|---:|---|
| E0 | String 5, A |
| E1 | String 6, low E |
| E2 | Left Pitch/Down/J; Kick 36 in drum mode |
| E3 | String 4, D |
| E4 | String 3, G |
| E5 | Right Pitch/Up/I; Snare 38 in drum mode |
| E6 | String 2, B |
| E7 | String 1, high E |
| E8 | Right Strum/Meter/N |
| E9 | Left Strum/Tempo/O |
| E10/E11 | Reserved, not necessarily exposed |

## Battery and soft power

```text
BAT+ -- 200k --+-- GPIO1
               +-- 100k -- GND
               +-- 0.1uF -- GND (recommended filter)
```

Battery voltage is calibrated sense voltage×3. GPIO8 ≥ 2600 mV for 100 ms allows startup; during established runtime,≤ 2600 mV for 200 ms requests shutdown/restart. Sample power every 100 ms and share ADC access with battery/joystick.

CHRG/STDBY: 1/0 charging; 0/1 full after four valid confirmations; 0/0 idle; 1/1 fault. Standby full confirmation also requires battery≥ 4.210 V.

| Display condition | 0% | 100% |
|---|---:|---:|
| Running and charging | 3.620 V | 4.150 V |
| Standby and charging | 3.670 V | 4.210 V |
| Not charging | 3.170 V | 3.850 V |

Updates use App1 Wi-Fi only. Users can replace App0 firmware and SF2 resources; App1, the bootloader and partition layout remain manufacturer-managed.
