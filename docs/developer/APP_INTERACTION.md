# Optional Fingersonic App Integration

Version: 2.0 · Updated: 2026-10-13

Third-party App0 firmware can run entirely independently or implement selected Fingersonic App features. Standard MIDI, settings synchronization, program catalogs, custom chords, chord follow, melody follow and remote uploader entry are separate capabilities. You do not have to implement all of them.

Firmware-update scope remains **App0 only**. Users can also replace SF2 resources. Both use the preinstalled App1 Wi-Fi uploader; entering App1 does not grant permission to replace it, the bootloader or the partition table.

## 1. Choose the features you need

| Feature | Suitable applications | Required interfaces |
|---|---|---|
| Standalone local operation | Synthesizer, groovebox, player, remote | Your own controls, engine and UI; no app protocol required |
| Standard USB/BLE MIDI | MIDI controller or sound module | MIDI messages, channels, disconnect/note cleanup |
| App settings/program selection | Remotely configurable instrument | ST GET/SET/STATE/ACK; optional PROGRAMS catalog |
| Eight-direction custom chords | Chord instrument | CH bank messages, symbol/voicing parser, eight slots |
| Chord Follow | Lessons, accompaniment, practice | CC85 mode selection; CC TARGET/RECEIVED/PLAYED/ERROR |
| Melody Follow | Note-by-note practice | CC85 mode selection; CM targets/play reports |
| Remote uploader entry | Preserve the app's update button | ST ENTER_BURN/ACK, then boot the existing App1 |

Declare unsupported functions in your release notes. A firmware without private SysEx support can still provide standard MIDI. You can implement only ENTER_BURN while omitting the other app pages. Never report success for an operation you did not implement.

## 2. Transport and framing

The default private app protocol uses **BLE MIDI SysEx**. USB MIDI support does not automatically expose these messages over USB; document a USB private interface separately if you provide one.

```text
F0 7D <group byte 1> <group byte 2> <7-bit ASCII payload> F7
```

`7D` is the development/non-commercial manufacturer identifier. Use ASCII payloads and let the BLE MIDI library handle timestamps, GATT fragmentation and MIDI reconstruction. This is a logical MIDI message, not a complete raw GATT packet. If your library adds F0/F7, do not add them twice.

| Group | Bytes | Content |
|---|---|---|
| ST | 53 54 | Settings, program catalog, uploader entry; FSGS1 version prefix |
| CH | 43 48 | Eight-slot chord banks; FSGC1/FSGC2/FSGC3 |
| CC | 43 43 | Chord Follow targets and receipts |
| CM | 43 4D | Melody Follow targets and receipts |

Default BLE name: `Fingersonic_Guitar`.

| BLE MIDI item | UUID |
|---|---|
| Service | `03b80e5a-ede8-4b33-a751-6ce34ec4c700` |
| Characteristic | `7772e5db-3868-4112-a1a9-f2669d106bf3` |

The default application's complete SysEx limit is 512 bytes, including escaped payload bytes. Check length before sending. Splitting a long command into independent SysEx messages does not create an implicit reassembly protocol. Document different limits in compatible firmware.

## 3. Chord-symbol grammar

```text
<chord> = <root>[:]<quality>[/<bass>]
```

| Part | Meaning | Example |
|---|---|---|
| root | Required root note | C, F#, Bb |
| optional colon | Compatible label separator; no musical change | A#:min7 |
| quality | Interval type; omitted means major triad | m, 7,maj7,m7b5 |
| /bass | Specified bass note | C/E, Cmaj7/G |

A symbol defines root, intervals and optional bass. It usually does not define exact frets for every string. Arrange your own voicing when rendering symbols, or use VOICING for exact six-string instructions.

### 3.1 Roots, enharmonics and case

Recommended ASCII roots: `C C# D D# E F F# G G# A A# B`. Enharmonic flats and B#/Fb/E#/Cb are accepted:

| Pitch class | Spellings |
|---:|---|
| 0 | C, B# |
| 1 | C#, Db |
| 2 | D |
| 3 | D#, Eb |
| 4 | E, Fb |
| 5 | F, E# |
| 6 | F#, Gb |
| 7 | G |
| 8 | G#, Ab |
| 9 | A |
| 10 | A#, Bb |
| 11 | B, Cb |

ASCII #/b and Unicode ♯/♭ represent accidentals. Uppercase M/M6/M7 mean Major/Major6/Major7, while lowercase m means Minor. Do not blindly lowercase every suffix before interpretation. Preserve valid original spellings for display/receipts; enharmonic names have the same pitches but can display differently.

### 3.2 Basic qualities and intervals

Numbers are semitone offsets from the root—not MIDI note numbers or fret positions.

| Quality | Suffix | Semitones | Example |
|---|---|---|---|
| Major | empty,maj,M | 0, 4, 7 | C |
| Minor | m,min | 0, 3, 7 | Dm |
| Dominant7 | 7 | 0, 4, 7, 10 | G7 |
| Minor7 | m7 | 0, 3, 7, 10 | Am7 |
| Major7 | maj7,M7 | 0, 4, 7, 11 | Cmaj7 |
| Minor-major7 | mMaj7 | 0, 3, 7, 11 | CmMaj7 |
| Major6 | 6,M6 | 0, 4, 7, 9 | C6 |
| Minor6 | m6 | 0, 3, 7, 9 | Am6 |
| Diminished | dim | 0, 3, 6 | Bdim |
| Diminished7 | dim7 | 0, 3, 6, 9 | Cdim7 |
| Half-diminished7 | m7b5 | 0, 3, 6, 10 | Bm7b5 |
| Augmented | aug,+ | 0, 4, 8 | Caug |
| Suspended2 | sus2 | 0, 2, 7 | Csus2 |
| Suspended4 | sus4,sus | 0, 5, 7 | Csus4 |
| Power chord | 5 | 0, 7 | C5 |

Compatible aliases include major/Δ, minor/-, dom7/dominant7, min7/minor7, ma7/major7/Δ7, minmaj7/mmaj7, maj6/ma6/major6, min6/minor6, diminished/o/°, diminished7/o7/°7, min7b5/hdim7/halfdim7/halfdiminished7/ø/ø7 and augmented.

### 3.3 Extended qualities

The default compatibility set also accepts these suffixes with valid roots/slash basses. Third parties may implement a subset if they declare it.

- Suspended/added notes: `sus9 sus4add9 2 4 69 m69 add2 add4 add9 add11 add13 madd9 madd11 madd13 6sus2 6sus4`
- Seventh extensions: `dim9 aug7 +7 aug9 +9 7sus 7sus2 7sus4 7susadd3 maj7sus2 maj7sus4`
- Major extensions: `maj7b5 maj7#5 maj7#11 maj9 maj9#11 maj11 maj13 maj13#11`
- Minor extensions: `m9 mMaj9 m11 m13 m7add11 m9b5 m11b5 m7#5 m7b9 m7#11 m7b13`
- Dominant extensions: `9 11 13 9b5 9#5 9#11 9b13 9sus 9sus4 9add6 13b5 13#5 13b9 13#9 13#11 13b13 13sus 13sus2 13sus4`
- Altered dominants: `7b5 7#5 7b9 7#9 7#11 7b13 7alt 7b9b5 7b9#5 7#9b5 7#9#5 7b9#9 7b9#11 7#9#11 7b9b13 7#9b13 7#11b13 7b9sus 7#9sus 7b13sus`
- Omitted notes: `no3 no5`

Reject an unknown quality rather than silently converting it to major. Allow only one slash bass: `Cmaj7/G/Db` is invalid. `C/E` specifies E bass and `F#m11/C#` specifies C# bass; neither specifies a unique guitar fingering.

### 3.4 N.C. and escaping

Empty string, N, NC, N.C. and X normalize to **N.C.** With no VOICING, this silent step stops old accompaniment, creates no new notes/arm direction, and returns RECEIVED followed by PLAYED. It is not a root named N.

A decoded symbol is limited to 31 bytes and cannot contain spaces, commas or the pipe delimiter. Encode Unicode using UTF-8-byte `%HH`; receipts use the same encoding:

```text
D♭m -> D%E2%99%ADm
CΔ7 -> C%CE%947
Eø7 -> E%C3%B87
```

Prefer ASCII equivalents such as Dbm, Cmaj7 and Em7b5 for portability. Decode names before displaying them and encode reserved/non-ASCII bytes when sending. Chord-name restrictions differ from program-name restrictions: program names may contain spaces.

## 4. Settings and program catalog: ST, optional

```text
App: FSGS1|GET
App: FSGS1|SET|volume=100|bpm=128
Device: FSGS1|ACK|SET|OK
Error: FSGS1|ACK|SET|ERROR|code=INVALID_VALUE|field=bpm
```

GET and successful SET produce `FSGS1|STATE|x/y|key=value...` snapshots. Collect a complete round before changing the app state. Do not hard-code frame count or firmware version. GET can resynchronize after connection/reconnection.

| Common key | Default protocol values |
|---|---|
| play_mode | CHORD / SOLO |
| pad_mode | PITCH_BEND / DRUM_PAD |
| home_display | PROGRAM / JOYSTICK_TARGETS |
| volume,reverb | 0–127 |
| program | Existing Bank 0 Program 0–127 |
| program_name | Actual preset name, escaped with%HH |
| transpose_semitones | −11–11 |
| capo_semitones | 0–11 |
| transpose_custom,loop_enabled,led_animation | ON / OFF |
| strum_pattern | PATTERN_4_4 / PATTERN_3_4 / PATTERN_2_4 / STRUM_4_4 |
| bpm | 20–300 |
| language | ENGLISH / CHINESE, protocol tokens retained for compatibility |

Validate all SET fields before saving/applying. Failure keeps old settings; success ACKs and pushes final state. The default settings path maps frontend reverb to `round(reverb*19/100)` for outgoing CC91; ordinary incoming MIDI CC91 remains0–127. Declare a different mapping in third-party firmware.

Catalog examples:

```text
FSGS1|PROGRAMS|GET
FSGS1|PROGRAMS|ITEM|1/2|bank=0|program=24|name=Nylon Guitar
FSGS1|PROGRAMS|ITEM|2/2|bank=0|program=25|name=Steel Guitar
FSGS1|SET|program=24
```

These are format examples, not addresses in the included recovery bank. Recovery Bank 0 guitars use Programs 0–4; see the [SoundFont guide](SOUNDFONT.md). Program IDs can be nonconsecutive in replacement banks. A duplicate program name yields PROGRAM_NAME_AMBIGUOUS; select by ID instead. Escape%,|,= and non-7-bit bytes as%HH. Ordinary program-name spaces may remain.

## 5. Eight-slot custom chords: CH, optional

Submit all eight slots at once. Slots1–8 map to N/NE/E/SE/S/SW/W/NW. Replace the old bank only after every slot validates; one invalid slot leaves the complete old bank intact.

```text
F0 7D 43 48 FSGC1|Cmaj7|Dm7|Em7|Fadd9|G7|Am|Bdim|G7 F7
```

FSGC1 is the symbol-only version, without a separate ACK or chord-bank GET. The app retains its submitted symbols. Enable through ST `FSGS1|SET|transpose_custom=ON` after a valid bank has been saved.

Use FSGC2/FSGC3 for acknowledged writes and exact voicings:

```text
FSGC2|C@X,3,2,0,1,0|Dm|Em|F|G|Am|Bdim|G7
FSGC3|C@X,3,2,0,1,0~2|Dm|Em|F|G|Am|Bdim|G7
FSGC2|ACK|COUNT=8
FSGC3|ACK|COUNT=8
FSGC3|ERROR|CODE=INVALID_SHIFT|SLOT=1|FIELD=FRET_SHIFT
```

After@, values are frets in String 6→1 order. FSGC3 adds a0–10 fret shift after~. Slots without@ still use symbol parsing. With@, the name is display/search metadata and the voicing determines sound. These are App0 runtime data operations, not system-firmware updates.

## 6. Chord Follow: CC, optional

Standard MIDI CC85 selects follow mode: 0 exits, 1 selects Chord Follow, 2 selects Melody Follow. Send on Channel 1 for app compatibility. Follow mode and local CHORD/SOLO settings are distinct states.

```text
F0 7D 43 43 TARGET|CHORD=Dm7|PATTERN=PATTERN_4_4|BEATS=4.00|BPM=100 F7
```

| Field | Required | Meaning/range |
|---|---|---|
| CHORD | Yes | Valid symbol, or name metadata with VOICING |
| PATTERN | Yes | One of the four pattern tokens |
| BEATS | Yes | 0.01–64.00 quarter-note beats; up to two decimals |
| BPM | Yes | Temporary integer20–300; does not overwrite global BPM |
| AUTO_PLAY | No | 0/1; 1 plays without joystick arm/Strum |
| VOICING | No | Six X/0–5 frets, String 6→1; at least one sounding string |
| FRET_SHIFT | No | 0–10; requires VOICING |

BEATS need not equal pattern meter. STRUM_4_4 with6.00 plays six beats. Fret shift applies only to frets1–5; X and open0 remain unchanged. Open-string MIDI notes are 40/45/50/55/59/64.

```text
TARGET|CHORD=C|PATTERN=STRUM_4_4|BEATS=4.00|BPM=120|VOICING=X,3,2,0,1,0
TARGET|CHORD=MyChord|PATTERN=PATTERN_4_4|BEATS=4.00|BPM=100|VOICING=X,3,2,0,1,0|FRET_SHIFT=2
TARGET|CHORD=Fmaj7|PATTERN=PATTERN_4_4|BEATS=4.00|BPM=105|AUTO_PLAY=1
```

X strings stay muted and are not filled with chord tones. A valid VOICING still sounds even if its metadata name is N.C. Without VOICING, N.C. is a silent step.

### 6.1 Receipts and playback timing

```text
# Target accepted and installed
RECEIVED|CHORD=Dm7|PATTERN=PATTERN_4_4|BEATS=4.00|BPM=100

# Performance/playback accepted
PLAYED|CHORD=Dm7|PATTERN=PATTERN_4_4|BEATS=4.00|BPM=100

# Invalid request
ERROR|CODE=MISSING_FIELD|FIELD=BEATS
ERROR|CODE=INVALID_VALUE|FIELD=PATTERN
```

All use CC framing. RECEIVED means validation/installation succeeded. PLAYED means playback was accepted, not all sound finished. Field order may vary. Reject duplicate, missing, unknown or out-of-range fields and retain the prior valid target.

Default manual flow: app sends target→RECEIVED→device assigns one of eight directions→user arms with joystick→Strum or adjacent-string sweep triggers→PLAYED→app advances. Equal symbols retain direction memory within the current song; clear memory on exit/new song. Automatic guitar uses Channel 2, drums Channel 10, and live strings Channel 1.

AUTO_PLAY=1 does not allocate an arm direction. A newly triggered target can stop old accompaniment and begin immediately while leaving live notes alone. Each active accompaniment needs an immutable target snapshot so later targets cannot change it mid-playback.

During follow, local transpose/CAPO are temporarily zero and local adjustment is locked; saved settings can be restored on exit. Loop is suspended/cleared and follow events are not recorded into it. Third parties can design different practice behavior, but full compatibility should follow these rules.

## 7. Melody Follow: CM, optional

Enter with CC85=2. FRET1–8 maps to the eight directions and STR1–6 uses guitar string numbering:

```text
F0 7D 43 4D TARGET|FRET=3|STR=2 F7
F0 7D 43 4D RECEIVED|FRET=3|STR=2 F7
F0 7D 43 4D PLAYED|FRET=3|STR=2 F7
F0 7D 43 4D CLEAR F7
F0 7D 43 4D RECEIVED|CLEAR F7
```

PLAYED reports actual fret/string, not only correct events. The app compares actual and target values. FRET0 describes open-string state; use 1–8 for ordinary targets and CLEAR to remove a prompt. CC85=0 exits follow and restores normal local mode.

## 8. Remote entry to the Wi-Fi uploader: ST, optional

```text
App: FSGS1|ENTER_BURN|request_id=123
Device: FSGS1|ACK|ENTER_BURN|ACCEPTED|request_id=123|target=APP1|reboot_in_ms=1000
Error: FSGS1|ACK|ENTER_BURN|ERROR|request_id=123|code=APP1_NOT_FOUND
```

request_id is 1–65535. Retry one operation with the same ID; a different ID while restart is pending may receive BUSY. ACCEPTED means a restart to existing App1 is scheduled, not that its hotspot is already ready. Stop sound/save settings, restart, disconnect BLE, then join Fingersonic Wi-Fi.

```text
SSID: Fingersonic
Password: 88888888
HTTP: POST http://192.168.4.1/upload
Content-Type: multipart/form-data
Field: file
Targets: .bin -> App0 (<=2MiB); .sf2 -> sound resources
```

If ENTER_BURN is omitted, encoder cold-start entry to the preinstalled App1 remains the recovery route. Explain the actual supported entry in release notes.

## 9. Implementation and compatibility notes

Validate reception and enqueue control work. Save files, switch sequencing and select App1 in ordinary tasks. Preserve protocol versions, case and escaping. Do not advance a song on an error. Discard incomplete snapshots on disconnect and resynchronize after reconnect.

Declare USB/BLE MIDI support, ST settings/catalog support, chord-quality/voicing versions, CH/CC/CM support and ENTER_BURN support. Do not advertise absent functions or reuse a protocol version for incompatible meanings.
