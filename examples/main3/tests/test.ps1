param([string]$LLVM=$env:ESP_LLVM_BIN,
      [string]$Gfx=$env:ADAFRUIT_GFX_DIR,
      [string]$Python='python')
if (-not $LLVM) { throw 'Pass -LLVM or set ESP_LLVM_BIN to your ESP LLVM bin directory.' }
if (-not $Gfx) { throw 'Pass -Gfx or set ADAFRUIT_GFX_DIR to the installed Adafruit GFX directory.' }
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$build=Join-Path $root 'build'
New-Item -ItemType Directory -Force -Path $build | Out-Null
& (Join-Path $LLVM 'llvm-dlltool.exe') -m i386:x86-64 -d "$PSScriptRoot\host\ucrt.def" -l "$build\ucrt.lib"
if ($LASTEXITCODE -ne 0) { throw 'CRT import failed.' }
$sources=@("$PSScriptRoot\HostTests.cpp","$root\GameCore.cpp","$root\GameDisplay.cpp","$root\GameSound.cpp")
& (Join-Path $LLVM 'clang++.exe') --target=x86_64-pc-windows-msvc -fuse-ld=lld -ffreestanding -fno-builtin -fno-exceptions -fno-rtti -shared -nostdlib -O2 -DPEA_HOST_TEST -I "$PSScriptRoot\host" -I $Gfx @sources "$build\ucrt.lib" '-Wl,/noentry' -o "$build\host_tests.dll"
if ($LASTEXITCODE -ne 0) { throw 'Host build failed.' }
& $Python "$PSScriptRoot\test.py"
if ($LASTEXITCODE -ne 0) { throw 'Game regression failed.' }
