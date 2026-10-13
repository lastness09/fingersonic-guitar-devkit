param([string]$Cli = 'arduino-cli')
$ErrorActionPreference = 'Stop'
$board = 'esp32:esp32:esp32s3:USBMode=default,CDCOnBoot=cdc,CPUFreq=240,FlashMode=qio,FlashSize=16M,PartitionScheme=custom,DebugLevel=none,PSRAM=opi,LoopCore=1,EventsCore=1'
$buildDir = Join-Path $PSScriptRoot 'build'
& $Cli compile --fqbn $board --build-path $buildDir $PSScriptRoot
if ($LASTEXITCODE -ne 0) { throw 'Firmware compilation failed.' }
$bin = Get-Item -LiteralPath (Join-Path $buildDir 'main2.ino.bin')
if ($bin.Length -le 0 -or $bin.Length -gt 0x200000) { throw 'Application must fit the 2 MiB App0 partition.' }
Write-Host ('App0 application: {0}, {1} bytes' -f $bin.FullName, $bin.Length)
Get-FileHash -LiteralPath $bin.FullName -Algorithm SHA256
Write-Host 'Upload main2.ino.bin through the preinstalled App1 Wi-Fi uploader.'
