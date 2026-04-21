@echo off
REM Self-sign shader.exe and shader.scr — no PowerShell
REM Requires: Windows SDK (makecert, pvk2pfx, signtool)

set EXE=shader.exe
set SCR=shader.scr
set PFX=psychip.pfx
set PASS=psychip2026
set PVK=psychip.pvk
set CER=psychip.cer

if exist %PFX% goto :sign

echo Creating self-signed certificate...
makecert -r -pe -n "CN=PsyChip,E=root@psychip.net" -ss My -sr CurrentUser -a sha256 -len 2048 -cy end -sky signature -sv %PVK% %CER%
if errorlevel 1 (
    echo makecert failed. Make sure Windows SDK is in PATH.
    pause
    exit /b 1
)

echo Converting to PFX...
pvk2pfx -pvk %PVK% -spc %CER% -pfx %PFX% -po %PASS% -f
del %PVK% 2>nul
del %CER% 2>nul

:sign
if not exist %PFX% (
    echo ERROR: No certificate found.
    pause
    exit /b 1
)

echo Signing %EXE%...
signtool sign /f %PFX% /p %PASS% /fd SHA256 /tr http://timestamp.digicert.com /td SHA256 /d "AVS Shader Screensaver" /du "https://www.psychip.net" %EXE%

if exist %SCR% (
    echo Signing %SCR%...
    signtool sign /f %PFX% /p %PASS% /fd SHA256 /tr http://timestamp.digicert.com /td SHA256 /d "AVS Shader Screensaver" /du "https://www.psychip.net" %SCR%
)

echo Done.
