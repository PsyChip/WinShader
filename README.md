# AVS Shader Screensaver

A portable OpenGL screensaver for Windows that randomly picks from **43 curated GLSL fragment shaders** embedded directly in the executable. No external files needed — a single `.exe` (or `.scr`) contains everything.

---

## Features

- **43 embedded GLSL shaders** — fractals, terrain, aurora, oceans, fire, clocks, cyberpunk cityscapes, glass refraction, fireworks and more
- **Fully portable** — all shaders are bundled as PE resources inside the executable. Copy it anywhere and run
- **Multi-monitor support** — renders across all monitors using the virtual screen dimensions
- **Dead-zone culling** — automatically detects mismatched monitor layouts (different resolutions/orientations) and injects per-pixel monitor bounds checking into every shader at runtime. Pixels in the gaps between monitors are skipped via early-return, saving GPU cycles on non-rectangular multi-monitor setups
- **Real-time clock** — shaders that use `iDate` receive actual system time via `GetLocalTime()`. When a shader has its own clock display, the built-in GDI overlay clock is automatically hidden
- **GDI clock overlay** — Segoe UI time/date display with drop shadow, anchored to primary monitor bottom-left. Disabled when the active shader provides its own clock
- **Screensaver compatible** — handles `/s`, `/p`, `/c`, `/a` command-line flags. Rename to `.scr` and install via right-click or copy to `System32`
- **Single instance** — mutex with 3-second timeout prevents multiple copies from running
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

1. Build the project (see below)
2. Rename `shader.exe` to `shader.scr`, or use the auto-generated copy from `build.bat`
3. Right-click `shader.scr` → **Install**, or copy to `C:\Windows\System32`
4. Select in **Screen Saver Settings**

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
| aurora.glsl | nimitz 2017 (@stormoid) | CC BY-NC-SA 3.0 |
| cob.glsl | Kabuto, based on @ahnqqq | — |
| frostedforest.glsl | eiffie | — |
| gamma.glsl | @XorDev | — |
| glasscube.glsl | Danil (github.com/danilw) | CC BY-NC-SA 3.0 |
| heaven.glsl | @XorDev | — |
| matrix.glsl | FabriceNeyret2, otaviogood | — |
| mountain.glsl | Alexander Alekseev (TDM) 2014 | CC BY-NC-SA 3.0 |
| sea.glsl | Alexander Alekseev (TDM) 2014 | CC BY-NC-SA 3.0 |
| sinus.glsl | Green120 | — |
| sunset.glsl | srvstr 2025 | MIT |
| terrain.glsl | Inigo Quilez 2014 | — |
| tracks.glsl | Cole Peterson (Plento) | — |
| tron.glsl | — | — |
| universe.glsl | Martijn Steinrucken (BigWings) 2018 | CC BY-NC-SA 3.0 |
| vorofire.glsl | — | — |
| warp.glsl | Inigo Quilez 2013 | — |
| wasteland.glsl | Dave Hoskins, nimitz (@stormoid) | CC BY-NC-SA 3.0 |
| waveform.glsl | @XorDev | — |
| woods.glsl | dr2 2018 | CC BY-NC-SA 3.0 |
| zion.glsl | dean_the_coder (@deanthecoder) | CC BY-NC-SA 3.0 |
| 7seg.glsl | Based on cmarangu | — |

Remaining shaders are twigl.app conversions, Shadertoy ports, or original compositions without embedded attribution.

## License

The host code (`shader.cpp`, `build.bat`, `sign.bat`) is released under MIT license.  
Individual GLSL shaders retain their original licenses as noted above. Most are CC BY-NC-SA 3.0.  
The icon (`commodorevic20.ico`) is included for personal/non-commercial use.

---

Curated by **PsyChip**
root@psychip.net
https://psychip.net
April 2026  

