![Screenshot](Screenshot%202026-04-21.png)

# <div align="center">AVS Shader Screensaver</div>

<div align="center">
<h3>A portable OpenGL screensaver for Windows that randomly picks from <b>23 curated GLSL fragment shaders</b> embedded directly in the executable. No external files needed — a single <code>.exe</code> (or <code>.scr</code>) contains everything.</h3>
</div>

---

## Usage

```
shader.exe                    # Run with a random embedded shader
shader.exe terrain.glsl       # Run a specific shader from disk
```

### As a screensaver

1. Build the project (see below) — `build.bat` produces both `shader.exe` and `shader.scr`
2. Run `install.bat` (as Administrator) to copy `shader.scr` into `System32` and register it as the active screensaver for both the desktop session and the lock screen
3. Run `uninstall.bat` to remove it

Or do it manually: right-click `shader.scr` → **Install**, or copy to `C:\Windows\System32` and pick it from **Screen Saver Settings**.

## Build

### Requirements

- **MSVC** (Visual Studio Build Tools or full Visual Studio)
- **Windows SDK** (for `rc.exe` resource compiler)
- No external libraries — uses only `user32`, `gdi32`, `opengl32`, `kernel32`

### Build command

```bat
build.bat
```

### Code signing (optional)

```bat
sign.bat
```

Creates a self-signed certificate (first run) and signs both `shader.exe` and `shader.scr` using Windows SDK tools (`makecert`, `pvk2pfx`, `signtool`).

---

**psychip.net**
May 2026  

