# Fingersonic Guitar Developer Kit

Build a groovebox, synthesizer, sampler, player, MIDI controller or wireless remote with Fingersonic. This public kit provides hardware interfaces, development recommendations, public-library examples, optional app protocols and sound-bank compatibility information.

**[Complete development guide](FIRMWARE_DEVELOPMENT_GUIDE.md)** · **[Documentation index](docs/developer/README.md)**

**[Return to official firmware and sounds](http://update.fingersonic.cn/FingersonicGuitarUploader/)** · [Recovery instructions](recovery/README.md#return-to-the-official-instrument)

Download the entire kit with GitHub's **Code → Download ZIP**, or clone this repository. Keep the `docs` and `recovery` folders beside the guide so images and recovery links work offline. [SHA256SUMS.txt](SHA256SUMS.txt) contains checksums for the published files, including the recovery bank.

![Fingersonic front controls](docs/developer/assets/controls-front.png)

## Custom firmware demo

[Watch the custom firmware demo on YouTube Shorts](https://youtube.com/shorts/IivDG06t62U?feature=share).

See an example of what you can create with your own App0 firmware. Use it as inspiration for your own instrument or controller. The [main2 reference/test project](examples/main2/README.md) provides source code to explore and modify; it is not official Fingersonic Guitar firmware.

## Explore the developer kit

| Document | Content |
|---|---|
| [Development guide](FIRMWARE_DEVELOPMENT_GUIDE.md) | GPIO, electrodes, power, DAC, LEDs, display, development platforms and Wi-Fi uploads |
| [Optional app protocol](docs/developer/APP_INTERACTION.md) | Settings, chord symbols, custom chords, follow targets and receipts |
| [SoundFont guide](docs/developer/SOUNDFONT.md) | Exact guitar/drum/piano presets, Lead roles and replacement compatibility |
| [Public API examples](docs/developer/EXAMPLES.md) | Arduino/Adafruit/IDF snippets without manufacturer-private source dependencies |
| [main2 reference/test example](examples/main2/README.md) | Open-source FM Studio development example, not official product firmware or a recovery image |
| [Dependencies](docs/developer/DEPENDENCIES.md) | Public libraries, versions and installation |
| [GPIO table](PCB_GPIO_Pinout.md) · [CSV](PCB_GPIO_Pinout.csv) | Wiring and MPR121 electrode mapping |
| [Partition CSV](docs/developer/partitions.csv) | App0 compilation layout; not a system flashing image |
| [Release checklist](docs/developer/RELEASE_CHECKLIST.md) | Compatibility, operation and validation recommendations |
| [Recovery sound bank](recovery/README.md) | Included Fingersonic_Gtr_H.sf2, preset list and checksum |

User firmware updates are limited to **App0 (2 MiB)**. Users may also replace the **SF2 sound bank**. Both use the preinstalled App1 Wi-Fi uploader. App1, the bootloader and partition table remain manufacturer-managed.

Windows/macOS users can use [FingersonicUploader](https://fingersonic.net/blogs/apps/fingersonicuploader-upload-custom-sounds-to-your-fingersonic). Linux users can build a client against the documented HTTP interface or use curl.

The included recovery bank has five guitars at Bank 0/Programs 0–4, Standard Kit at Bank 128/Program 0, and Piano at Bank 128/Program 1. It restores sounds only. The [official recovery tool](http://update.fingersonic.cn/FingersonicGuitarUploader/) restores official App0 firmware and the official sound bank as separate Wi-Fi uploads.

This kit does not include private manufacturer firmware source or system images. App integration is optional; third-party applications can use the published hardware interfaces independently.

The [FM Studio main2 example](examples/main2/README.md) is provided for developer reference and testing under Apache License 2.0, with its bundled DaisySP subset retaining the MIT license. **main2 is not official Fingersonic Guitar firmware and must not be used as an official recovery image.**

## Return to the official instrument

After experimenting with main2 or other third-party firmware, you can return to the original Fingersonic Guitar experience using the [official firmware and sound-bank recovery tool](http://update.fingersonic.cn/FingersonicGuitarUploader/). Prepare the selected file while connected to the Internet, then enter App1 and switch to the Fingersonic Wi-Fi hotspot to upload it. Firmware restores App0; restore the official SF2 separately if you changed the sound bank. See the [step-by-step recovery instructions](recovery/README.md#return-to-the-official-instrument).

## Support and contact

For Fingersonic hardware, recovery files and platform questions:

- Website: [Fingersonic.net](https://fingersonic.net/)
- Support email: [support@fingersonic.net](mailto:support@fingersonic.net)
