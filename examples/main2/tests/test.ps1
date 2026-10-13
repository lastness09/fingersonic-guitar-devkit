param(
  [string]$LlvmBin = $env:ESP_LLVM_BIN,
  [string]$Gfx = $env:ADAFRUIT_GFX_DIR,
  [string]$Python = 'python'
)
$ErrorActionPreference = 'Stop'
if (-not $LlvmBin) { throw 'Set ESP_LLVM_BIN or pass -LlvmBin with the ESP LLVM bin directory.' }
if (-not $Gfx -or -not (Test-Path -LiteralPath (Join-Path $Gfx 'glcdfont.c'))) {
  throw 'Set ADAFRUIT_GFX_DIR or pass -Gfx with the installed Adafruit GFX library directory.'
}
$root = Split-Path $PSScriptRoot -Parent
$buildDir = Join-Path $root 'build'
New-Item -ItemType Directory -Force -Path $buildDir | Out-Null
& (Join-Path $LlvmBin 'llvm-dlltool.exe') -m i386:x86-64 -d (Join-Path $PSScriptRoot 'host\ucrt.def') -l (Join-Path $buildDir 'ucrt.lib')
if ($LASTEXITCODE -ne 0) { throw 'CRT import library generation failed.' }
$sources = @("$PSScriptRoot\HostCore.cpp", "$root\GrooveCore.cpp", "$root\GrooveDisplay.cpp",
  "$root\src\DaisySP\Synthesis\fm2.cpp", "$root\src\DaisySP\Synthesis\oscillator.cpp", "$root\src\DaisySP\Control\adsr.cpp")
$library = Join-Path $buildDir 'host_tests.dll'
& (Join-Path $LlvmBin 'clang++.exe') --target=x86_64-pc-windows-msvc -fuse-ld=lld -ffreestanding -fno-builtin -fno-exceptions -fno-rtti -shared -nostdlib -O2 -DGROOVE_HOST_TEST -DGROOVE_MINIMAL_WINDOWS_CRT -I "$PSScriptRoot\host" -I $Gfx @sources (Join-Path $buildDir 'ucrt.lib') '-Wl,/noentry' -o $library
if ($LASTEXITCODE -ne 0) { throw 'Host test compilation failed.' }
& $Python (Join-Path $PSScriptRoot 'test.py') --library $library
if ($LASTEXITCODE -ne 0) { throw 'Host regression failed.' }
