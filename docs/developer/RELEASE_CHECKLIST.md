# Third-Party Release and Hardware Validation Checklist

Firmware uploads target App0 only; SF2 resources may also be replaced. Both use App1 Wi-Fi. Fill in this record for each release.

## Release information

| Field | Record |
|---|---|
| Name/version | For example MyGroovebox0.1.0 |
| Build identifier | Version/tag; publishing your own source is optional |
| Supported hardware | PCB/module revision, 16 MiB Flash, PSRAM requirements, ST7735S configuration |
| Toolchain | Arduino Core/IDF and actual dependency versions |
| Application | App0 BIN filename, byte size, SHA-256 |
| Resources | SF2/WAV/MP3 formats, directories, preset maps and licenses |
| Data compatibility | Own application directory, existing settings/resources retained |
| Recovery | Supported App1 entry, matching default App0 download, included recovery SF2 |
| Validation | Device revision/identifier and test date |

## Hardware checks

- [ ] Encoder cold start enters the existing App1; normal cold start enters App0.
- [ ] App1 Wi-Fi installs App0, handles timeout/invalid/oversize BINs, and restores default App0.
- [ ] Pitch left E2/right E5; Strum left TempoE9/right MeterE8 are correct.
- [ ] Six strings, short contacts, release edges, bidirectional sweeps and simultaneous touches work.
- [ ] GPIO8 startup/shutdown and initialization-load dips are handled; battery/charging standby work.
- [ ] Battery voltage, four CHRG/STDBY combinations, full confirmation and averaged display work.
- [ ] Joystick center/eight directions/switch; encoder both directions/fast movement/holds work.
- [ ] TFT colors, offsets, boundaries, backlight and parameter refresh work.
- [ ] LED indices/colors remain stable with touch/audio; shutdown leaves the ring dark.
- [ ] DAC left/right channels, starts/stops, silence and sustained audio have no unexpected pops/dropouts.
- [ ] USB enumeration/reconnection/channels and BLE connection/retry work where implemented.
- [ ] Heap/PSRAM/task stacks and long-running behavior are adequate; unexpected resets are recoverable.
- [ ] App0 update retains resources; SF2 replacement retains other settings; missing resources/space report errors.
- [ ] Windows/macOS Uploader and Linux HTTP uploads work; failed SF2 upload can be recovered.
- [ ] Recovery H bank has guitars Bank 0/P0–4, Standard Kit Bank 128/P0, Piano Bank 128/P1.

## Public package and GitHub

Publish README, the guide, developer documents, GPIO tables and recovery instructions. Put application BINs and authorized sound banks in releases or the documented resource folder, with SHA-256 checksums. Do not publish user-installable App1/bootloader/partition images, private manufacturer source, build caches, computer-specific paths or serial logs.

Preserve dependency licenses and obtain distribution rights for third-party samples. Keep PNG/SVG relative links intact. Diagram regeneration code is in tools/generate_diagrams.py and contains diagram definitions, not private firmware source.
