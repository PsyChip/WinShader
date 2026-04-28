@echo off
REM Remove shader screensaver from both desktop and lock screen
REM Requires: Run as Administrator

REM Remove desktop screensaver
reg delete "HKCU\Control Panel\Desktop" /v SCRNSAVE.EXE /f >nul 2>nul
reg add "HKCU\Control Panel\Desktop" /v ScreenSaveActive /t REG_SZ /d "0" /f >nul

REM Remove lock screen screensaver
reg delete "HKU\.DEFAULT\Control Panel\Desktop" /v SCRNSAVE.EXE /f >nul 2>nul
reg add "HKU\.DEFAULT\Control Panel\Desktop" /v ScreenSaveActive /t REG_SZ /d "0" /f >nul

REM Delete from System32
del "%SystemRoot%\System32\shader.scr" >nul 2>nul

echo Screensaver removed.
