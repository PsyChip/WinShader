@echo off

copy shader.scr "%SystemRoot%\System32\shader.scr" /y
REM Set as active screensaver for current user
reg add "HKCU\Control Panel\Desktop" /v SCRNSAVE.EXE /t REG_SZ /d "%SystemRoot%\System32\shader.scr" /f >nul
reg add "HKCU\Control Panel\Desktop" /v ScreenSaveActive /t REG_SZ /d "1" /f >nul
reg add "HKCU\Control Panel\Desktop" /v ScreenSaveTimeOut /t REG_SZ /d "300" /f >nul
reg add "HKCU\Control Panel\Desktop" /v ScreenSaverIsSecure /t REG_SZ /d "1" /f >nul

REM Set as lock screen screensaver (runs on login/lock screen desktop)
reg add "HKU\.DEFAULT\Control Panel\Desktop" /v SCRNSAVE.EXE /t REG_SZ /d "%SystemRoot%\System32\shader.scr" /f >nul
reg add "HKU\.DEFAULT\Control Panel\Desktop" /v ScreenSaveActive /t REG_SZ /d "1" /f >nul
reg add "HKU\.DEFAULT\Control Panel\Desktop" /v ScreenSaveTimeOut /t REG_SZ /d "60" /f >nul

echo Installed: %SystemRoot%\System32\shader.scr
echo Desktop screensaver: 5 minutes timeout
echo Lock screen screensaver: 1 minute timeout
echo.
echo To test now: shader.scr /s
echo To remove:   screensaver_remove.bat
pause