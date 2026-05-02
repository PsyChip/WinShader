![Screenshot](Screenshot%202026-04-21.png)

# <div align="center">AVS Shader Screensaver</div>

<div align="center">
<h3>A portable OpenGL screensaver for Windows that randomly picks from <b>23 curated GLSL fragment shaders</b> embedded directly in the executable. No external files needed — a single <code>.exe</code> (or <code>.scr</code>) contains everything.</h3>
</div>

---

## Features

- **23 embedded GLSL shaders** — fractals, terrain, oceans, fire, clocks, alien water, tunnels, galaxies, ripples and more. Additional heavyweight shaders (aurora, mountain, zion, frostedforest, waveform, flower, star) are stashed in `heavy/` and can be embedded by moving them up one level before building
- **Exit transitions** — when the user dismisses the screensaver, the host snapshots the current desktop, uploads it as a texture, and runs a GPU crossfade between the live shader frame and the desktop using one of the embedded `exit-*.glsl` shaders (e.g. `exit-05-perlin`, `exit-12-crosswarp`). Each variant has its own duration (perlin 0.8s, crosswarp 0.9s, crossfade 0.35s, default 0.5s) looked up from the resource name. Direction alternates between shader→desktop and desktop→shader. With no `exit-*` resources embedded, the host falls back to instant exit
- **`/lock` flag** — when launched with `shader.exe /lock`, the workstation is locked the moment the user dismisses the screensaver (mouse/keyboard). The mutex is released before `LockWorkStation()` so the lock screen can immediately spawn its own instance
- **Install / uninstall scripts** — `install.bat` copies `shader.scr` to `System32`, registers it as the active screensaver for the current user *and* for the lock screen desktop (`HKU\.DEFAULT`). `uninstall.bat` reverses both
- **Fully portable** — all shaders are bundled as PE resources inside the executable. Copy it anywhere and run
- **Multi-monitor support** — renders across all monitors using the virtual screen dimensions
- **Dead-zone culling** — automatically detects mismatched monitor layouts (different resolutions/orientations) and injects per-pixel monitor bounds checking into every shader at runtime. Pixels in the gaps between monitors are skipped via early-return, saving GPU cycles on non-rectangular multi-monitor setups
- **Real-time clock** — shaders that use `iDate` receive actual system time via `GetLocalTime()`. When a shader has its own clock display, the built-in GDI overlay clock is automatically hidden
- **GDI clock overlay** — Segoe UI time/date display with drop shadow, anchored to primary monitor bottom-left. Disabled when the active shader provides its own clock
- **Screensaver compatible** — handles `/s`, `/p`, `/c`, `/a` command-line flags. Rename to `.scr` and install via right-click or copy to `System32`
- **Single instance** — mutex with 3-second timeout prevents multiple copies from running. Mutex is released before `LockWorkStation()` to avoid stalling the next instance on exit
- **GPU-friendly optimizations** — many shaders include scanline skip (50% pixel reduction), reduced iteration counts, and early-exit patterns
- **Shadertoy uniform compatibility** — supports `time`/`resolution`/`mouse` and `iTime`/`iResolution`/`iMouse`/`iTimeDelta`/`iFrame`/`iDate` uniforms
- **VSync enabled** via `wglSwapIntervalEXT`
- **GLSL injection** — monitor culling code is transparently injected into shader source at load time by renaming `main()` and wrapping it. No per-shader modification needed

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

This will:
1. Auto-generate `shaders.rc` from all `*.glsl` files in the directory + icon + version info
2. Compile the resource file with `rc`
3. Compile and link `shader.cpp` with embedded resources
4. Copy `shader.exe` to `shader.scr`
5. Launch `shader.exe`

### Code signing (optional)

```bat
sign.bat
```

Creates a self-signed certificate (first run) and signs both `shader.exe` and `shader.scr` using Windows SDK tools (`makecert`, `pvk2pfx`, `signtool`).

## Architecture

```
shader.cpp          Main host — window, OpenGL context, render loop, clock overlay
*.glsl              Fragment shaders (embedded as RCDATA PE resources at build time)
build.bat           Build script — generates .rc, compiles, links
sign.bat            Self-sign script
commodorevic20.ico  Application icon
```

### Shader loading pipeline

1. `EnumerateEmbeddedShaders()` scans PE resources via `EnumResourceNamesA(RT_RCDATA)`
2. `PickEmbeddedShader()` selects one at random using `GetTickCount()`
3. Source is checked for `iDate` to decide whether to show the GDI clock overlay
4. If multi-monitor dead zones are detected, `InjectCulling()` transparently wraps the shader's `main()` with monitor bounds checking
5. Source is compiled and linked as a fragment shader, paired with a minimal `gl_VertexID`-based fullscreen quad vertex shader

### Multi-monitor culling

On systems with monitors of different sizes or orientations, the virtual screen bounding box contains rectangular gaps where no monitor exists. The host detects this at startup via `EnumDisplayMonitors`, converts monitor rects to GL coordinates, and injects uniform declarations + a bounds check into each shader's `main()`. Pixels outside all monitor rects early-return as black — the GPU skips the expensive fragment computation entirely.

This injection is **automatic and transparent** — no shader modification needed. On uniform monitor setups, no injection occurs (zero overhead).

## Tested on

- NVIDIA RTX 3090 + 4 monitors (3 horizontal + 1 vertical)
- Windows 10/11, MSVC 19.44, x64

## Shader Credits

| Shader | Author | License |
|--------|--------|---------|
| matrix.glsl | FabriceNeyret2, otaviogood | — |
| sea.glsl | Alexander Alekseev (TDM) 2014 | CC BY-NC-SA 3.0 |
| seascape.glsl | Alexander Alekseev (TDM) 2014 | CC BY-NC-SA 3.0 |
| sinus.glsl | Green120 | — |
| terrain.glsl | Inigo Quilez 2014 | — |
| warp.glsl | Inigo Quilez 2013 | — |
| 7seg.glsl | Based on cmarangu | — |

Shaders in `heavy/` (not embedded by default):

| Shader | Author | License |
|--------|--------|---------|
| aurora.glsl | nimitz 2017 (@stormoid) | CC BY-NC-SA 3.0 |
| frostedforest.glsl | eiffie | — |
| mountain.glsl | Alexander Alekseev (TDM) 2014 | CC BY-NC-SA 3.0 |
| waveform.glsl | @XorDev | — |
| zion.glsl | dean_the_coder (@deanthecoder) | CC BY-NC-SA 3.0 |

Remaining shaders are twigl.app conversions, Shadertoy ports, or original compositions without embedded attribution.

## License

The host code (`shader.cpp`, `build.bat`, `sign.bat`) is released under MIT license.  
Individual GLSL shaders retain their original licenses as noted above. Most are CC BY-NC-SA 3.0.  
The icon (`commodorevic20.ico`) is included for personal/non-commercial use.

---

**psychip.net**
May 2026  

