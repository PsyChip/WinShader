@echo off
REM Generate shaders.rc from all .glsl files in current directory
echo // Auto-generated shader resources > shaders.rc
set /a COUNT=0
for %%f in (*.glsl) do (
    set /a COUNT+=1
    echo SHADER_%%~nf RCDATA "%%f" >> shaders.rc
)
echo // Total shaders embedded: %COUNT% >> shaders.rc
echo Generated shaders.rc with resources
