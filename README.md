# Fingersonic Guitar Developer Kit

Build a groovebox, synthesizer, sampler, player, MIDI controller or wireless remote with Fingersonic. This public kit provides hardware interfaces, development recommendations, public-library examples, optional app protocols and sound-bank compatibility information.

**[Complete development guide](FIRMWARE_DEVELOPMENT_GUIDE.md)** · **[Documentation index](docs/developer/README.md)**

Download the entire kit with GitHub's **Code → Download ZIP**, or clone this repository. Keep the `docs` and `recovery` folders beside the guide so images and recovery links work offline. [SHA256SUMS.txt](SHA256SUMS.txt) contains checksums for the published files, including the recovery bank.

![Fingersonic front controls](docs/developer/assets/controls-front.png)

| Document | Content |
|---|---|
| [Development guide](FIRMWARE_DEVELOPMENT_GUIDE.md) | GPIO, electrodes, power, DAC, LEDs, display, development platforms and Wi-Fi uploads |
| [Optional app protocol](docs/developer/APP_INTERACTION.md) | Settings, chord symbols, custom chords, follow targets and receipts |
| [SoundFont guide](docs/developer/SOUNDFONT.md) | Exact guitar/drum/piano presets, Lead roles and replacement compatibility |
| [Public API examples](docs/developer/EXAMPLES.md) | Arduino/Adafruit/IDF snippets without manufacturer-private source dependencies |
| [FM Studio reference firmware](examples/main2/README.md) | Complete open-source main2 App0: FM synthesis, a five-track groovebox, UI, NVS, build scripts and host tests |
| [Dependencies](docs/developer/DEPENDENCIES.md) | Public libraries, versions and installation |
| [GPIO table](PCB_GPIO_Pinout.md) · [CSV](PCB_GPIO_Pinout.csv) | Wiring and MPR121 electrode mapping |
| [Partition CSV](docs/developer/partitions.csv) | App0 compilation layout; not a system flashing image |
| [Release checklist](docs/developer/RELEASE_CHECKLIST.md) | Compatibility, operation and validation recommendations |
| [Recovery sound bank](recovery/README.md) | Included Fingersonic_Gtr_H.sf2, preset list and checksum |

User firmware updates are limited to **App0 (2 MiB)**. Users may also replace the **SF2 sound bank**. Both use the preinstalled App1 Wi-Fi uploader. App1, the bootloader and partition table remain manufacturer-managed.

Windows/macOS users can use [FingersonicUploader](https://fingersonic.net/blogs/apps/fingersonicuploader-upload-custom-sounds-to-your-fingersonic). Linux users can build a client against the documented HTTP interface or use curl.

The included recovery bank has five guitars at Bank 0/Programs 0–4, Standard Kit at Bank 128/Program 0, and Piano at Bank 128/Program 1. It restores sounds only; recovering App0 requires a matching default application BIN from the manufacturer.

This kit does not include private manufacturer firmware source or system images. App integration is optional; third-party applications can use the published hardware interfaces independently.

The [FM Studio main2 example](examples/main2/README.md) provides a complete reference application under Apache License 2.0, with its bundled DaisySP subset retaining the MIT license. It can be built and modified independently of the default instrument firmware.

## Support and contact

For Fingersonic hardware, recovery files and platform questions:

- Website: [Fingersonic.net](https://fingersonic.net/)
- Support email: [support@fingersonic.net](mailto:support@fingersonic.net)
