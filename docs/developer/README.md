# Developer Documentation

Create your own instrument or controller: groovebox, synthesizer, sampler, music player, MIDI controller or wireless remote.

**[Open the complete guide](../../FIRMWARE_DEVELOPMENT_GUIDE.md)**

Firmware updates target App0 only. SF2 sound resources can also be replaced. All uploads use preinstalled App1 Wi-Fi; App1, the bootloader and partition table remain unchanged.

![Slider electrodes and gestures](assets/sliders-electrodes.png)

Start with the GPIO/electrode map, integrate the public API snippets, verify inputs/power, then add audio and UI. The API snippets require integration into your own project; FM Studio main2 provides a complete buildable reference application.

| Resource | Purpose |
|---|---|
| [Complete guide](../../FIRMWARE_DEVELOPMENT_GUIDE.md) | Hardware, drivers, MIDI, memory layout, updates and design recommendations |
| [GPIO CSV](../../PCB_GPIO_Pinout.csv) | Machine-readable wiring/electrode table |
| [Dependency versions](DEPENDENCIES.md) | Public Arduino/IDF library baseline |
| [API examples](EXAMPLES.md) | ADC, MPR121, I2S, TFT and App1 entry |
| [FM Studio main2](../../examples/main2/README.md) | Buildable open-source FM synthesizer/groovebox reference application |
| [Optional app protocol](APP_INTERACTION.md) | Symbols, settings, custom chords, follow and receipts |
| [SF2 guide](SOUNDFONT.md) | Guitar/drum/piano layout, Lead roles and replacement rules |
| [Recovery bank](../../recovery/README.md) | Fingersonic_Gtr_H.sf2 and checksum |
| [Partition CSV](partitions.csv) | App0 compile configuration only |
| [Release checklist](RELEASE_CHECKLIST.md) | Third-party release/validation recommendations |
| [Diagram assets](assets) | Six PNG/SVG pairs; PNG for reading, SVG for editing |

Orientation is **upright front view, logo at the top and strings vertical**. Pitch is left E2/right E5; Strum is left TempoE9/right MeterE8. These are MPR121 electrodes. ESP32 GPIO11/12 are the I2C bus.
