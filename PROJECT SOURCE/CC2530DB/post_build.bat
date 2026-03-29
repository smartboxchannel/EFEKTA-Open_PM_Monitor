@echo off
echo Starting post-build...

REM OTA Converter
"%~dp0..\..\..\..\tools\OTA\OtaConverter\Release\OtaConverter.exe" "%~dp0Router\Exe\OpenPM_Router.sim" -o"%~dp0Router\Exe" -t0x1000 -m0x5678 -v10063202 -pCC2530DB

REM Use PowerShell for HEX merge
powershell -Command "& { $boot = Get-Content '%~dp0Boot.hex'; $app = Get-Content '%~dp0Router\Exe\OpenPM_Router.hex'; $boot[0..($boot.Length-3)] + $app[1..($app.Length-1)] | Out-File '%~dp0..\firmware\Merged_Firmware.hex' -Encoding ASCII }"

echo Completed