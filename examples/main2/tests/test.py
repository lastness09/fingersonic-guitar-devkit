"""Run the firmware DSP and UI in a host shared library; write artifacts to build/."""
from pathlib import Path
import ctypes as c
import wave
import time
import struct
import hashlib

ROOT = Path(__file__).resolve().parents[1]

def build_library():
    """Use an existing host library, or build with a configured native compiler."""
    import argparse
    import os
    import shutil
    import subprocess
    import sys
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--library", type=Path, help="Existing test DLL/shared library")
    parser.add_argument("--compiler", default=os.environ.get("CXX"), help="Native C++ compiler executable")
    parser.add_argument("--gfx", default=os.environ.get("ADAFRUIT_GFX_DIR"), help="Installed Adafruit GFX directory")
    args = parser.parse_args()
    if args.library:
        library = args.library.resolve()
        if not library.is_file():
            parser.error("The specified host library does not exist")
        return library
    if not args.gfx or not (Path(args.gfx)/"glcdfont.c").is_file():
        parser.error("Pass --gfx or set ADAFRUIT_GFX_DIR to the installed Adafruit GFX directory")
    compiler = args.compiler or shutil.which("c++") or shutil.which("g++") or shutil.which("clang++")
    if not compiler:
        parser.error("A native C++ compiler is required; Windows ESP LLVM users can use tests/test.ps1")
    build = ROOT/"build"
    build.mkdir(exist_ok=True)
    suffix = ".dll" if sys.platform == "win32" else ".dylib" if sys.platform == "darwin" else ".so"
    library = build/("host_tests"+suffix)
    sources = [ROOT/"tests/HostCore.cpp", ROOT/"GrooveCore.cpp", ROOT/"GrooveDisplay.cpp",
        ROOT/"src/DaisySP/Synthesis/fm2.cpp", ROOT/"src/DaisySP/Synthesis/oscillator.cpp",
        ROOT/"src/DaisySP/Control/adsr.cpp"]
    flags = ["-dynamiclib"] if sys.platform == "darwin" else ["-shared"]
    if sys.platform != "win32":
        flags.append("-fPIC")
    command = [str(compiler), "-std=c++17", "-O2", "-DGROOVE_HOST_TEST", *flags,
        "-I", str(Path(args.gfx)), *(str(p) for p in sources), "-o", str(library)]
    subprocess.run(command, check=True)
    return library

lib = c.CDLL(str(build_library()))
lib.fm_tests.restype = c.c_int
result = lib.fm_tests()
assert result == 0, f"DSP regression failed, case {result}"
print("PASS: presets, sine LUT, saturation, voice release, tempo, swing, bank switching, recording")
lib.fm_demo.argtypes = [c.c_int]
lib.fm_render.argtypes = [c.POINTER(c.c_int16), c.c_int]
pcm = (c.c_int16 * (512 * 2))()
lib.fm_demo(0)
start = time.perf_counter()
with wave.open(str(ROOT / "build" / "groove_demo.wav"), "wb") as out:
    out.setparams((2, 2, 32000, 0, "NONE", "not compressed"))
    for _ in range(500):
        lib.fm_render(pcm, 512)
        out.writeframes(bytes(pcm))
print(f"Rendered 8s demo in {time.perf_counter() - start:.3f}s (host only, not ESP benchmark)")

from PIL import Image, ImageDraw
lib.fm_preview.argtypes = [c.c_int, c.c_int, c.c_int]
lib.fm_preview.restype = c.POINTER(c.c_uint16)
montage = Image.new("RGB", (160 * 3 * 3, 105 * 3 * 3), (24, 24, 24))
names = ["LIVE", "STEPS", "SOUND", "MIXER", "SETTINGS", "STANDBY", "BOOT"]
draw = ImageDraw.Draw(montage)
for page in range(7):
    pointer = lib.fm_preview(page, 6, 1)
    pixels = [pointer[i] for i in range(160 * 80)]
    assert sum(bool(p) for p in pixels) > 250, f"blank UI {page}"
    rgb = [(int((p >> 11) * 255 / 31), int(((p >> 5) & 63) * 255 / 63), int((p & 31) * 255 / 31)) for p in pixels]
    frame = Image.new("RGB", (160, 80))
    frame.putdata(rgb)
    frame.save(ROOT / "build" / f"ui_{names[page].lower()}.png")
    x, y = page % 3 * 480, page // 3 * 315
    montage.paste(frame.resize((480, 240), Image.Resampling.NEAREST), (x, y + 35))
    draw.text((x + 10, y + 10), names[page], fill=(230, 230, 230))
montage.save(ROOT / "build" / "ui_preview.png")
print("PASS: seven actual firmware UI views rendered; build/ui_preview.png")
startup_frames = []
for stage in (7, 8, 9):
    pointer = lib.fm_preview(stage, -1, 0)
    pixels = tuple(pointer[i] for i in range(160 * 80))
    assert sum(bool(pixel) for pixel in pixels) > 250
    startup_frames.append(pixels)
assert len(set(startup_frames)) == 3, "startup diagnostics must be distinguishable"
print("PASS: TOUCH INIT, AUDIO INIT and INPUT WAIT startup indicators")

firmware = ROOT / "build" / "main2.ino.bin"
if firmware.exists():
    binary = firmware.read_bytes()
    assert 0 < len(binary) <= 0x200000, "App0 exceeds 2MiB"
    table = (ROOT / "build" / "main2.ino.partitions.bin").read_bytes()
    apps = {}
    for offset in range(0, len(table) - 31, 32):
        magic, kind, subtype, address, size, label, flags = struct.unpack_from("<HBBII16sI", table, offset)
        if magic == 0x50AA and kind == 0:
            apps[label.split(b"\0")[0].decode()] = (address, size, flags)
    assert apps["app0"] == (0x10000, 0x200000, 1)
    assert apps["app1"] == (0x210000, 0x200000, 1)
    print(f"PASS: firmware {len(binary)}B fits encrypted App0, App1 layout unchanged")
    print(f"SHA256: {hashlib.sha256(binary).hexdigest()}")
