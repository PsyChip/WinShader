@echo off

REM Generate shaders.rc from all .glsl files + icon + version info
(
echo #include ^<winver.h^>
echo.
echo IDI_ICON1 ICON "commodorevic20.ico"
echo.
echo VS_VERSION_INFO VERSIONINFO
echo FILEVERSION 1,0,0,0
echo PRODUCTVERSION 1,0,0,0
echo FILEFLAGSMASK 0x3fL
echo FILEFLAGS 0x0L
echo FILEOS VOS_NT_WINDOWS32
echo FILETYPE VFT_APP
echo FILESUBTYPE 0x0L
echo BEGIN
echo   BLOCK "StringFileInfo"
echo   BEGIN
echo     BLOCK "040904b0"
echo     BEGIN
echo       VALUE "CompanyName", "PsyChip"
echo       VALUE "FileDescription", "GLSL shader screensaver"
echo       VALUE "FileVersion", "1.2.0.0"
echo       VALUE "InternalName", "shader"
echo       VALUE "LegalCopyright", "psychip.net"
echo       VALUE "OriginalFilename", "shader.scr"
echo       VALUE "ProductName", "GLSL shader screensaver"
echo       VALUE "ProductVersion", "1.2.0.0"
echo     END
echo   END
echo   BLOCK "VarFileInfo"
echo   BEGIN
echo     VALUE "Translation", 0x0409, 1200
echo   END
echo END
echo.
) > shaders.rc
for %%f in (*.glsl) do (
    echo %%~nf RCDATA "%%f" >> shaders.rc
)

REM Compile resource file
rc /nologo shaders.rc

REM Compile and link with embedded resources
cl /O1 /GS- /GF shader.cpp shaders.res /link /OPT:REF /OPT:ICF user32.lib gdi32.lib opengl32.lib kernel32.lib ole32.lib /subsystem:windows /out:shader.exe

del *.obj 2>nul
del shaders.res 2>nul
del shaders.rc 2>nul

REM Copy as screensaver
copy /y shader.exe shader.scr >nul 2>nul

start shader.exe
