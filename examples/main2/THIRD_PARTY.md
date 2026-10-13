# Third-party components

## Bundled DaisySP subset

- Upstream: [daisyaudio/DaisySP](https://github.com/daisyaudio/DaisySP).
- Source revision: [2c72eaf9eac5fc0dca1919d65d606da907832618](https://github.com/daisyaudio/DaisySP/tree/2c72eaf9eac5fc0dca1919d65d606da907832618).
- License: MIT, with the complete upstream notices retained in [src/DaisySP/LICENSE](src/DaisySP/LICENSE).
- Included components: FM2, Oscillator, ADSR and their DSP utility dependencies.
- Local changes: relative include paths; initialization of oscillator history; a 2, 049-entry sine table with linear interpolation and negative-phase normalization; a reduced utility header; English adaptation comments.

The application converts FM2's modulation index into a pitch-independent depth in GrooveCore.cpp. This is application logic around the bundled DSP implementation.

## External build dependencies

ESP32 Arduino Core, Adafruit MPR121, BusIO, GFX and ST7735/ST7789 are installed separately and retain their upstream licenses. They are not vendored into this example. The optional host UI tests read glcdfont.c from the installed Adafruit GFX library.

Python/Pillow is used only by the optional host tests to render previews. Firmware compilation does not require Python or Pillow.
