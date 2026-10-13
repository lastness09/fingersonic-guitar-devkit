# Official Firmware and Sound Bank Recovery

This folder includes **Fingersonic_Gtr_H.sf2**, the recovery sound bank supplied with this documentation package. Upload it through the existing App1 Wi-Fi uploader to restore the guitar, drum and piano sound resources.

## Return to the official instrument

After experimenting with reference or third-party firmware, return to the original Fingersonic Guitar experience with the **[official firmware and sound-bank recovery tool](http://update.fingersonic.cn/FingersonicGuitarUploader/)**.

**main2/FM Studio is a reference and test example, not official product firmware or an official recovery image.** Restoring official App0 brings back the official instrument behavior; restoring SF2 brings back its sounds. These are separate uploads.

1. Connect your computer or phone to a normal Internet connection and open the official tool. Select **Firmware Update** and wait until the selected official file is ready. Keep the page open.
2. Enter preinstalled App1: disconnect charging power, turn off, hold the encoder, then turn on and keep holding until Burn Mode appears.
3. Join **Fingersonic**, password **88888888**. Complete the page's confirmations, start the upload and wait for the device to finish and restart.
4. If you changed the sound bank, reconnect to the Internet, select **Official SoundFont** and wait for it to be ready. Re-enter App1, rejoin its hotspot and upload the sound bank. Wait for restart again.

Prepare each file before switching to the device hotspot, which does not provide Internet access. If file preparation fails, reconnect to your normal network and retry there. Check the device after each upload rather than treating a connection loss alone as proof of recovery.

main2 itself leaves the existing SF2 in place, so returning from that example normally requires only official App0 restoration. Keep App1, the bootloader and the partition layout intact. Your application and sound-bank replacements both use App1 Wi-Fi.

## Included offline recovery bank

| Property | Value |
|---|---|
| File | [Fingersonic_Gtr_H.sf2](Fingersonic_Gtr_H.sf2) |
| Size | 2,943,846 bytes |
| Format | SoundFont 2.1 |
| Presets | 7 |
| SHA-256 | `92ed846d71542271d238b326d92607a41aff142271d57c02d87dc88e222355b7` |

## Restore the sound bank

1. Enter the preinstalled App1 uploader. For a cold start, disconnect charging power, hold the encoder button, turn the device on, and keep holding until the uploader is ready.
2. Connect to Wi-Fi **Fingersonic**, password **88888888**.
3. On Windows or macOS, use [FingersonicUploader](https://fingersonic.net/blogs/apps/fingersonicuploader-upload-custom-sounds-to-your-fingersonic) to upload this `.sf2` file.
4. Wait for the upload to finish and the device to restart. Check the five guitars, drum pads and piano part.

On Linux, or from your own upload client:

```bash
curl --fail -F 'file=@./recovery/Fingersonic_Gtr_H.sf2' http://192.168.4.1/upload
```

Verify the file before uploading:

```powershell
Get-FileHash -LiteralPath './recovery/Fingersonic_Gtr_H.sf2' -Algorithm SHA256
```

```bash
sha256sum ./recovery/Fingersonic_Gtr_H.sf2
```

Replacing an SF2 bank removes the previous SF2 resources. An interrupted upload may require uploading this recovery file again. Other settings and custom-chord data are normally retained.

This file restores **sound resources only**. It does not restore or replace App0 firmware. Use the [official recovery tool](http://update.fingersonic.cn/FingersonicGuitarUploader/) above to restore official App0. App1, the bootloader and the partition table remain manufacturer-managed.

## Presets in the supplied recovery bank

| Bank | Program (zero-based) | Name |
|---:|---:|---|
| 0 | 0 | Steel Gtr |
| 0 | 1 | Funk Gtr |
| 0 | 2 | Jazz Gtr |
| 0 | 3 | Mute Gtr |
| 0 | 4 | Overd. Gtr |
| 128 | 0 | Standard Kit |
| 128 | 1 | Piano |

There is no separate preset named Lead in this file. A lead or melody role uses the instrument selected by the application, or a separately supplied bank. See the [SoundFont guide](../docs/developer/SOUNDFONT.md) for channel assignments and replacement-bank compatibility.
