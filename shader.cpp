#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <gl/GL.h>


// ---- WGL extension prototypes (loaded at runtime) -------------------------
typedef HGLRC (WINAPI *PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC, HGLRC, const int*);
typedef BOOL  (WINAPI *PFNWGLSWAPINTERVALEXTPROC)(int);
typedef GLuint(WINAPI *PFNGLCREATESHADERPROC)(GLenum);
typedef void  (WINAPI *PFNGLSHADERSOURCEPROC)(GLuint, GLsizei, const char**, const GLint*);
typedef void  (WINAPI *PFNGLCOMPILESHADERPROC)(GLuint);
typedef GLuint(WINAPI *PFNGLCREATEPROGRAMPROC)(void);
typedef void  (WINAPI *PFNGLATTACHSHADERPROC)(GLuint, GLuint);
typedef void  (WINAPI *PFNGLLINKPROGRAMPROC)(GLuint);
typedef void  (WINAPI *PFNGLUSEPROGRAMPROC)(GLuint);
typedef void  (WINAPI *PFNGLDELETESHADERPROC)(GLuint);
typedef GLint (WINAPI *PFNGLGETUNIFORMLOCATIONPROC)(GLuint, const char*);
typedef void  (WINAPI *PFNGLUNIFORM1FPROC)(GLint, float);
typedef void  (WINAPI *PFNGLUNIFORM1IPROC)(GLint, int);
typedef void  (WINAPI *PFNGLUNIFORM2FPROC)(GLint, float, float);
typedef void  (WINAPI *PFNGLUNIFORM3FPROC)(GLint, float, float, float);
typedef void  (WINAPI *PFNGLUNIFORM4FPROC)(GLint, float, float, float, float);
typedef void  (WINAPI *PFNGLUNIFORM4FVPROC)(GLint, GLsizei, const float*);
typedef void  (WINAPI *PFNGLGETSHADERIVPROC)(GLuint, GLenum, GLint*);
typedef void  (WINAPI *PFNGLGETSHADERINFOLOGPROC)(GLuint, GLsizei, GLsizei*, char*);
typedef void  (WINAPI *PFNGLGETPROGRAMIVPROC)(GLuint, GLenum, GLint*);
typedef void  (WINAPI *PFNGLGETPROGRAMINFOLOGPROC)(GLuint, GLsizei, GLsizei*, char*);

#define GL_FRAGMENT_SHADER        0x8B30
#define GL_VERTEX_SHADER          0x8B31
#define GL_COMPILE_STATUS         0x8B81
#define GL_LINK_STATUS            0x8B82
#define GL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define GL_CONTEXT_MINOR_VERSION_ARB 0x2092

static PFNGLCREATESHADERPROC        glCreateShader;
static PFNGLSHADERSOURCEPROC        glShaderSource;
static PFNGLCOMPILESHADERPROC       glCompileShader;
static PFNGLCREATEPROGRAMPROC       glCreateProgram;
static PFNGLATTACHSHADERPROC        glAttachShader;
static PFNGLLINKPROGRAMPROC         glLinkProgram;
static PFNGLUSEPROGRAMPROC          glUseProgram;
static PFNGLDELETESHADERPROC        glDeleteShader;
static PFNGLGETUNIFORMLOCATIONPROC  glGetUniformLocation;
static PFNGLUNIFORM1FPROC           glUniform1f;
static PFNGLUNIFORM1IPROC           glUniform1i;
static PFNGLUNIFORM2FPROC           glUniform2f;
static PFNGLUNIFORM3FPROC           glUniform3f;
static PFNGLUNIFORM4FPROC           glUniform4f;
static PFNGLUNIFORM4FVPROC          glUniform4fv;
static PFNGLGETSHADERIVPROC         glGetShaderiv;
static PFNGLGETSHADERINFOLOGPROC    glGetShaderInfoLog;
static PFNGLGETPROGRAMIVPROC        glGetProgramiv;
static PFNGLGETPROGRAMINFOLOGPROC   glGetProgramInfoLog;
static PFNWGLSWAPINTERVALEXTPROC    wglSwapIntervalEXT;

#define LOAD_GL(T,name) name = (T)wglGetProcAddress(#name)

// ---------------------------------------------------------------------------

static HDC   g_hdc     = nullptr;
static HGLRC g_hglrc   = nullptr;
static GLuint g_prog   = 0;
static int   g_w = 1, g_h = 1;
static DWORD g_startMs = 0;
static float g_mx = 0.5f, g_my = 0.5f;
static bool  g_shaderHasClock = false; // true if shader uses iDate (has its own clock)
static bool  g_lockOnExit = false;     // lock workstation when shader is dismissed
static HANDLE g_hMutex = nullptr;      // single-instance mutex

// ---------------------------------------------------------------------------
// Monitor dead-zone culling
// ---------------------------------------------------------------------------
#define MAX_MONITORS 8
static RECT  g_monRects[MAX_MONITORS];  // Windows coords (top-left origin)
static int   g_monCount = 0;
static bool  g_needsCulling = false;     // true if monitors differ in size/position
static GLint g_uMonCount = -1;
static GLint g_uMonRects = -1;

// GL-space monitor rects: vec4(xMin, yMin, xMax, yMax) with Y=0 at bottom
static float g_monGL[MAX_MONITORS * 4];

static BOOL CALLBACK MonitorEnumProc(HMONITOR, HDC, LPRECT rc, LPARAM)
{
    if (g_monCount < MAX_MONITORS)
        g_monRects[g_monCount++] = *rc;
    return TRUE;
}

static void EnumerateMonitors()
{
    g_monCount = 0;
    EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, 0);

    if (g_monCount < 2) { g_needsCulling = false; return; }

    // Check if all monitors share the same height and vertical position
    // If so, no dead zones exist — skip culling
    bool allSame = true;
    int h0 = g_monRects[0].bottom - g_monRects[0].top;
    int t0 = g_monRects[0].top;
    for (int i = 1; i < g_monCount; ++i) {
        int hi = g_monRects[i].bottom - g_monRects[i].top;
        int ti = g_monRects[i].top;
        if (hi != h0 || ti != t0) { allSame = false; break; }
    }
    g_needsCulling = !allSame;
}

static void BuildMonitorGLRects(int vx, int vy, int vw, int vh)
{
    // Convert Windows rects to GL coords:
    // GL x = winX - vx
    // GL y = (vh - 1) - (winY - vy)  → flip Y
    for (int i = 0; i < g_monCount; ++i) {
        float xMin = (float)(g_monRects[i].left   - vx);
        float xMax = (float)(g_monRects[i].right  - vx);
        // Flip Y: Windows top→GL bottom
        float yMin = (float)(vh - (g_monRects[i].bottom - vy));
        float yMax = (float)(vh - (g_monRects[i].top    - vy));
        g_monGL[i * 4 + 0] = xMin;
        g_monGL[i * 4 + 1] = yMin;
        g_monGL[i * 4 + 2] = xMax;
        g_monGL[i * 4 + 3] = yMax;
    }
}

// GLSL prefix: declares uniforms and early-returns if pixel is outside all monitors
static const char* kCullPrefix = R"(
uniform int   _monCount;
uniform vec4  _monRects[8];
)";

// Cached uniform locations (set once after link, -1 = not present)
static GLint g_uTime       = -1;
static GLint g_uResolution = -1;
static GLint g_uMouse      = -1;
static GLint g_uITime      = -1;
static GLint g_uIResolution= -1;
static GLint g_uIMouse     = -1;
static GLint g_uITimeDelta = -1;
static GLint g_uIFrame     = -1;
static GLint g_uIDate      = -1;

// Minimal vertex shader — just passes through a fullscreen quad
static const char* kVertSrc = R"(
void main() {
    float x = float((gl_VertexID & 1) << 1) - 1.0;
    float y = float((gl_VertexID & 2)) - 1.0;
    gl_Position = vec4(x, y, 0.0, 1.0);
}
)";



static char* ReadFile(const char* path)
{
    HANDLE h = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ,
                           nullptr, OPEN_EXISTING, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE) return nullptr;
    DWORD size = GetFileSize(h, nullptr);
    char* buf = (char*)HeapAlloc(GetProcessHeap(), 0, size + 1);
    DWORD rd = 0;
    ReadFile(h, buf, size, &rd, nullptr);
    buf[rd] = '\0';
    CloseHandle(h);
    return buf;
}

// ---------------------------------------------------------------------------
// Embedded shader resources
// ---------------------------------------------------------------------------

struct ResEntry { const char* name; const char* data; DWORD size; };
static ResEntry g_resShaders[256];
static int      g_resCount = 0;

static BOOL CALLBACK EnumResNameProc(HMODULE hMod, LPCSTR type, LPSTR name, LONG_PTR lParam)
{
    if (g_resCount >= 256) return FALSE;
    HRSRC hr = FindResourceA(hMod, name, type);
    if (!hr) return TRUE;
    HGLOBAL hg = LoadResource(hMod, hr);
    if (!hg) return TRUE;
    DWORD sz = SizeofResource(hMod, hr);
    const char* ptr = (const char*)LockResource(hg);
    if (ptr && sz > 0) {
        g_resShaders[g_resCount].name = name;
        g_resShaders[g_resCount].data = ptr;
        g_resShaders[g_resCount].size = sz;
        g_resCount++;
    }
    return TRUE;
}

static void EnumerateEmbeddedShaders()
{
    HMODULE hMod = GetModuleHandleA(nullptr);
    // RCDATA type = RT_RCDATA = 10
    EnumResourceNamesA(hMod, RT_RCDATA, EnumResNameProc, 0);
}

// Returns heap-allocated null-terminated copy of a random embedded shader, or nullptr
static char* PickEmbeddedShader()
{
    if (g_resCount == 0) return nullptr;
    int pick = (int)(GetTickCount() % (DWORD)g_resCount);
    DWORD sz = g_resShaders[pick].size;
    char* buf = (char*)HeapAlloc(GetProcessHeap(), 0, sz + 1);
    for (DWORD i = 0; i < sz; ++i) buf[i] = g_resShaders[pick].data[i];
    buf[sz] = '\0';
    return buf;
}

static GLuint CompileShader(GLenum type, const char* src)
{
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &src, nullptr);
    glCompileShader(sh);
    GLint ok = 0;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetShaderInfoLog(sh, sizeof(log), nullptr, log);
        MessageBoxA(nullptr, log, "Shader compile error", MB_OK | MB_ICONERROR);
    }
    return sh;
}

// Inject monitor culling code into fragment shader source
// Inserts uniform declarations after any #version/precision/extension lines,
// and a dead-zone check at the start of main()
static char* InjectCulling(const char* fragSrc)
{
    // Find insertion point for uniforms: after last #/precision line
    const char* uniformInsert = fragSrc;
    const char* scan = fragSrc;
    while (*scan) {
        // Skip lines starting with #, or containing "precision"
        if (*scan == '#' || (scan == fragSrc || *(scan-1) == '\n')) {
            const char* lineStart = scan;
            // Check if line starts with # or "precision"
            bool isDirective = (*scan == '#');
            bool isPrec = (strncmp(scan, "precision", 9) == 0);
            if (isDirective || isPrec) {
                // Skip to end of line
                while (*scan && *scan != '\n') scan++;
                if (*scan == '\n') scan++;
                uniformInsert = scan;
                continue;
            }
        }
        // Skip to next line
        while (*scan && *scan != '\n') scan++;
        if (*scan == '\n') scan++;
        break;
    }

    // Find "void main()" to insert the check
    const char* mainPos = strstr(fragSrc, "void main()");
    if (!mainPos) mainPos = strstr(fragSrc, "void main ()");
    if (!mainPos) mainPos = strstr(fragSrc, "void main(");
    if (!mainPos) return nullptr;

    // Find the opening '{' after main
    const char* brace = strchr(mainPos, '{');
    if (!brace) return nullptr;
    brace++; // past the '{'

    const char* cullCheck = "\n"
        "    // Dead-zone culling: skip pixels outside all monitors\n"
        "    { bool _vis = false;\n"
        "      for (int _i = 0; _i < _monCount; _i++) {\n"
        "        vec4 _mr = _monRects[_i];\n"
        "        if (gl_FragCoord.x >= _mr.x && gl_FragCoord.x < _mr.z &&\n"
        "            gl_FragCoord.y >= _mr.y && gl_FragCoord.y < _mr.w)\n"
        "          { _vis = true; break; }\n"
        "      }\n"
        "      if (!_vis) { gl_FragColor = vec4(0,0,0,1); return; }\n"
        "    }\n";

    int prefixLen = lstrlenA(kCullPrefix);
    int checkLen  = lstrlenA(cullCheck);
    int srcLen    = lstrlenA(fragSrc);
    int outLen    = srcLen + prefixLen + checkLen + 16;

    char* out = (char*)HeapAlloc(GetProcessHeap(), 0, outLen);
    char* w = out;

    // Copy up to uniform insertion point
    int uOff = (int)(uniformInsert - fragSrc);
    for (int i = 0; i < uOff; ++i) *w++ = fragSrc[i];

    // Insert uniform declarations
    const char* s = kCullPrefix;
    while (*s) *w++ = *s++;

    // Copy from uniform insertion point up to after main's '{'
    int bOff = (int)(brace - fragSrc);
    for (int i = uOff; i < bOff; ++i) *w++ = fragSrc[i];

    // Insert cull check
    s = cullCheck;
    while (*s) *w++ = *s++;

    // Copy rest of source
    const char* rest = brace;
    while (*rest) *w++ = *rest++;
    *w = '\0';

    return out;
}

static bool BuildProgram(const char* fragSrc)
{
    char* cullSrc = nullptr;
    if (g_needsCulling) {
        cullSrc = InjectCulling(fragSrc);
        if (cullSrc) fragSrc = cullSrc;
    }

    GLuint vs = CompileShader(GL_VERTEX_SHADER, kVertSrc);
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fragSrc);

    g_prog = glCreateProgram();
    glAttachShader(g_prog, vs);
    glAttachShader(g_prog, fs);
    glLinkProgram(g_prog);

    GLint ok = 0;
    glGetProgramiv(g_prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetProgramInfoLog(g_prog, sizeof(log), nullptr, log);
        MessageBoxA(nullptr, log, "Program link error", MB_OK | MB_ICONERROR);
        return false;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    if (cullSrc) HeapFree(GetProcessHeap(), 0, cullSrc);

    // Cache uniform locations
    g_uTime        = glGetUniformLocation(g_prog, "time");
    g_uResolution  = glGetUniformLocation(g_prog, "resolution");
    g_uMouse       = glGetUniformLocation(g_prog, "mouse");
    g_uITime       = glGetUniformLocation(g_prog, "iTime");
    g_uIResolution = glGetUniformLocation(g_prog, "iResolution");
    g_uIMouse      = glGetUniformLocation(g_prog, "iMouse");
    g_uITimeDelta  = glGetUniformLocation(g_prog, "iTimeDelta");
    g_uIFrame      = glGetUniformLocation(g_prog, "iFrame");
    g_uIDate       = glGetUniformLocation(g_prog, "iDate");

    // Monitor culling uniforms
    g_uMonCount    = glGetUniformLocation(g_prog, "_monCount");
    g_uMonRects    = glGetUniformLocation(g_prog, "_monRects");

    return true;
}


// PickRandomShader removed — embedded resources used instead

static bool InitGL(HWND hwnd, const char* fragSrcIn)
{
    // 1. Create a dummy context to get wglCreateContextAttribsARB
    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize        = sizeof(pfd);
    pfd.nVersion     = 1;
    pfd.dwFlags      = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType   = PFD_TYPE_RGBA;
    pfd.cColorBits   = 32;

    HDC dc = GetDC(hwnd);
    int fmt = ChoosePixelFormat(dc, &pfd);
    SetPixelFormat(dc, fmt, &pfd);

    HGLRC tempCtx = wglCreateContext(dc);
    wglMakeCurrent(dc, tempCtx);

    auto wglCreateContextAttribsARB =
        (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");

    wglSwapIntervalEXT =
        (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");

    // 2. Real context (OpenGL 2.1 is enough for GLSL 1.10)
    HGLRC ctx = wglCreateContextAttribsARB
        ? wglCreateContextAttribsARB(dc, nullptr, nullptr)
        : tempCtx;

    wglMakeCurrent(dc, ctx);
    if (tempCtx != ctx) wglDeleteContext(tempCtx);

    g_hdc   = dc;
    g_hglrc = ctx;

    if (wglSwapIntervalEXT) wglSwapIntervalEXT(1); // vsync

    // 3. Load extension functions
    LOAD_GL(PFNGLCREATESHADERPROC,       glCreateShader);
    LOAD_GL(PFNGLSHADERSOURCEPROC,       glShaderSource);
    LOAD_GL(PFNGLCOMPILESHADERPROC,      glCompileShader);
    LOAD_GL(PFNGLCREATEPROGRAMPROC,      glCreateProgram);
    LOAD_GL(PFNGLATTACHSHADERPROC,       glAttachShader);
    LOAD_GL(PFNGLLINKPROGRAMPROC,        glLinkProgram);
    LOAD_GL(PFNGLUSEPROGRAMPROC,         glUseProgram);
    LOAD_GL(PFNGLDELETESHADERPROC,       glDeleteShader);
    LOAD_GL(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation);
    LOAD_GL(PFNGLUNIFORM1FPROC,          glUniform1f);
    LOAD_GL(PFNGLUNIFORM1IPROC,          glUniform1i);
    LOAD_GL(PFNGLUNIFORM2FPROC,          glUniform2f);
    LOAD_GL(PFNGLUNIFORM3FPROC,          glUniform3f);
    LOAD_GL(PFNGLUNIFORM4FPROC,          glUniform4f);
    LOAD_GL(PFNGLUNIFORM4FVPROC,         glUniform4fv);
    LOAD_GL(PFNGLGETSHADERIVPROC,        glGetShaderiv);
    LOAD_GL(PFNGLGETSHADERINFOLOGPROC,   glGetShaderInfoLog);
    LOAD_GL(PFNGLGETPROGRAMIVPROC,       glGetProgramiv);
    LOAD_GL(PFNGLGETPROGRAMINFOLOGPROC,  glGetProgramInfoLog);

    // 4. Compile the shader source (already in memory)
    // Detect if shader has its own clock (uses iDate)
    g_shaderHasClock = (strstr(fragSrcIn, "iDate") != nullptr);
    bool ok = BuildProgram(fragSrcIn);
    return ok;
}

static void Render()
{
    glViewport(0, 0, g_w, g_h);
    glUseProgram(g_prog);

    float t  = (float)(GetTickCount() - g_startMs) / 1000.0f;
    float mx = g_mx * g_w, my = g_my * g_h;

    if (g_uTime       != -1) glUniform1f(g_uTime,        t);
    if (g_uResolution != -1) glUniform2f(g_uResolution,  (float)g_w, (float)g_h);
    if (g_uMouse      != -1) glUniform2f(g_uMouse,       mx, my);
    if (g_uITime      != -1) glUniform1f(g_uITime,       t);
    if (g_uIResolution!= -1) glUniform3f(g_uIResolution, (float)g_w, (float)g_h, 1.0f);
    if (g_uIMouse     != -1) glUniform4f(g_uIMouse,      mx, my, 0.0f, 0.0f);
    if (g_uITimeDelta != -1) glUniform1f(g_uITimeDelta,  0.016f);
    if (g_uIFrame     != -1) glUniform1i(g_uIFrame,      (int)(t * 60.0f));
    if (g_uIDate      != -1) {
        SYSTEMTIME st;
        GetLocalTime(&st);
        float secsSinceMidnight = (float)st.wHour * 3600.0f + (float)st.wMinute * 60.0f + (float)st.wSecond;
        glUniform4f(g_uIDate, (float)st.wYear, (float)st.wMonth, (float)st.wDay, secsSinceMidnight);
    }

    // Monitor culling uniforms (only set if injection is active)
    if (g_uMonCount   != -1) glUniform1i(g_uMonCount,    g_monCount);
    if (g_uMonRects   != -1) glUniform4fv(g_uMonRects,   g_monCount, g_monGL);

    // Draw fullscreen quad (4 vertices, no VBO needed with gl_VertexID trick)
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    SwapBuffers(g_hdc);
}

// ---------------------------------------------------------------------------
// Clock overlay — layered window drawn with GDI+
// ---------------------------------------------------------------------------

static HWND g_clockWnd = nullptr;

// Persistent clock GDI resources — created once, reused every tick
static const int CW = 600, CH = 170;
static HDC   g_clockScreenDC = nullptr;
static HDC   g_clockMemDC    = nullptr;
static HDC   g_clockTmpDC    = nullptr;
static HBITMAP g_clockBmp    = nullptr;
static HBITMAP g_clockTmpBmp = nullptr;
static HBITMAP g_clockOldBmp = nullptr;
static HBITMAP g_clockTmpOld = nullptr;
static DWORD*  g_clockBits   = nullptr;
static DWORD*  g_clockTmpBits= nullptr;
static HFONT   g_clockTimeFont = nullptr;
static HFONT   g_clockDateFont = nullptr;

static void InitClockGDI()
{
    g_clockScreenDC = GetDC(nullptr);
    g_clockMemDC    = CreateCompatibleDC(g_clockScreenDC);
    g_clockTmpDC    = CreateCompatibleDC(g_clockScreenDC);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = CW;
    bmi.bmiHeader.biHeight      = -CH;
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    g_clockBmp    = CreateDIBSection(g_clockMemDC, &bmi, DIB_RGB_COLORS, (void**)&g_clockBits, nullptr, 0);
    g_clockTmpBmp = CreateDIBSection(g_clockTmpDC, &bmi, DIB_RGB_COLORS, (void**)&g_clockTmpBits, nullptr, 0);
    g_clockOldBmp = (HBITMAP)SelectObject(g_clockMemDC, g_clockBmp);
    g_clockTmpOld = (HBITMAP)SelectObject(g_clockTmpDC, g_clockTmpBmp);

    SetBkMode(g_clockTmpDC, TRANSPARENT);
    SetTextColor(g_clockTmpDC, RGB(255, 255, 255));

    g_clockTimeFont = CreateFontW(100, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    g_clockDateFont = CreateFontW(36, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
}

static void FreeClockGDI()
{
    if (g_clockTimeFont) { DeleteObject(g_clockTimeFont); g_clockTimeFont = nullptr; }
    if (g_clockDateFont) { DeleteObject(g_clockDateFont); g_clockDateFont = nullptr; }
    if (g_clockTmpDC)    { SelectObject(g_clockTmpDC, g_clockTmpOld); DeleteDC(g_clockTmpDC); g_clockTmpDC = nullptr; }
    if (g_clockTmpBmp)   { DeleteObject(g_clockTmpBmp); g_clockTmpBmp = nullptr; }
    if (g_clockMemDC)    { SelectObject(g_clockMemDC, g_clockOldBmp); DeleteDC(g_clockMemDC); g_clockMemDC = nullptr; }
    if (g_clockBmp)      { DeleteObject(g_clockBmp); g_clockBmp = nullptr; }
    if (g_clockScreenDC) { ReleaseDC(nullptr, g_clockScreenDC); g_clockScreenDC = nullptr; }
}

static void ClockDrawText(const wchar_t* text, HFONT font, int x, int y, DWORD argb)
{
    for (int i = 0; i < CW * CH; ++i) g_clockTmpBits[i] = 0;
    SelectObject(g_clockTmpDC, font);
    RECT rc = {0, 0, CW, CH};
    ExtTextOutW(g_clockTmpDC, 0, 0, 0, &rc, text, lstrlenW(text), nullptr);

    BYTE ar = (argb >> 24) & 0xFF, rr = (argb >> 16) & 0xFF;
    BYTE gr = (argb >> 8) & 0xFF, br = argb & 0xFF;

    for (int py = 0; py < CH; ++py) {
        for (int px = 0; px < CW; ++px) {
            BYTE cov = (BYTE)(g_clockTmpBits[py * CW + px] & 0xFF);
            if (cov == 0) continue;
            int dx = x + px, dy = y + py;
            if (dx < 0 || dx >= CW || dy < 0 || dy >= CH) continue;
            BYTE a = (BYTE)((ar * cov) / 255);
            DWORD& dst = g_clockBits[dy * CW + dx];
            BYTE inv = 255 - a;
            BYTE da = (dst >> 24) & 0xFF;
            dst = (((DWORD)(a + (da * inv) / 255)) << 24) |
                  (((DWORD)((rr * a + ((dst >> 16 & 0xFF) * inv)) / 255)) << 16) |
                  (((DWORD)((gr * a + ((dst >> 8 & 0xFF) * inv)) / 255)) << 8) |
                  ((DWORD)((br * a + ((dst & 0xFF) * inv)) / 255));
        }
    }
}

static void DrawClock(HWND hwnd)
{
    if (!g_clockMemDC) InitClockGDI();

    MONITORINFO mi = {};
    mi.cbSize = sizeof(mi);
    POINT origin = {0, 0};
    GetMonitorInfoW(MonitorFromPoint(origin, MONITOR_DEFAULTTOPRIMARY), &mi);
    RECT mon = mi.rcMonitor;

    const int PAD = 36;
    int winX = mon.left + PAD;
    int winY = mon.bottom - CH - PAD;

    // Clear
    for (int i = 0; i < CW * CH; ++i) g_clockBits[i] = 0;

    SYSTEMTIME st;
    GetLocalTime(&st);
    wchar_t timeBuf[16];
    wsprintfW(timeBuf, L"%02d:%02d", st.wHour, st.wMinute);
    wchar_t dayName[64], monthDay[64], dateBuf[128];
    GetDateFormatW(LOCALE_USER_DEFAULT, 0, &st, L"dddd", dayName, 64);
    GetDateFormatW(LOCALE_USER_DEFAULT, 0, &st, L"MMM d", monthDay, 64);
    wsprintfW(dateBuf, L"%s, %s", monthDay, dayName);

    ClockDrawText(timeBuf, g_clockTimeFont, 4, 4, 0x99000000);
    ClockDrawText(timeBuf, g_clockTimeFont, 0, 0, 0xFFFFFFFF);
    ClockDrawText(dateBuf, g_clockDateFont, 4, 98, 0x99000000);
    ClockDrawText(dateBuf, g_clockDateFont, 2, 96, 0xFFFFFFFF);

    POINT ptSrc = {0, 0};
    SIZE  szWnd = {CW, CH};
    POINT ptDst = {winX, winY};
    BLENDFUNCTION bf = {};
    bf.BlendOp             = AC_SRC_OVER;
    bf.SourceConstantAlpha = 255;
    bf.AlphaFormat         = AC_SRC_ALPHA;

    SetWindowPos(hwnd, nullptr, winX, winY, CW, CH, SWP_NOZORDER | SWP_NOACTIVATE);
    UpdateLayeredWindow(hwnd, g_clockScreenDC, &ptDst, &szWnd, g_clockMemDC, &ptSrc, 0, &bf, ULW_ALPHA);
}

LRESULT CALLBACK ClockWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_TIMER:
        DrawClock(hwnd);
        return 0;
    // Forward quit keys to main window
    case WM_KEYDOWN:
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
        if (g_clockWnd) DestroyWindow(GetParent(hwnd));
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

static void CreateClockWindow(HINSTANCE hInst, HWND parent)
{
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = ClockWndProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = L"ClockOverlay";
    wc.hbrBackground = nullptr;
    RegisterClassExW(&wc);

    g_clockWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT,
        L"ClockOverlay", L"",
        WS_POPUP | WS_VISIBLE,
        0, 0, 1, 1,
        parent, nullptr, hInst, nullptr
    );

    DrawClock(g_clockWnd);
    SetTimer(g_clockWnd, 1, 1000, nullptr);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_SIZE:
    {
        RECT rc;
        GetClientRect(hwnd, &rc);
        g_w = rc.right  - rc.left;
        g_h = rc.bottom - rc.top;
        return 0;
    }

    case WM_MOUSEMOVE:
        if (g_w > 0 && g_h > 0) {
            g_mx = (float)LOWORD(lp) / g_w;
            g_my = 1.0f - (float)HIWORD(lp) / g_h; // flip Y for GL
        }
        return 0;

    case WM_SETCURSOR:
        SetCursor(LoadCursor(nullptr, IDC_ARROW));
        return TRUE;

    case WM_ERASEBKGND: return 1;

    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
        DestroyWindow(hwnd);
        return 0;
    case WM_KEYDOWN:
        if (wp == VK_ESCAPE) DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        if (g_hglrc) { wglMakeCurrent(nullptr, nullptr); wglDeleteContext(g_hglrc); }
        if (g_hMutex) { ReleaseMutex(g_hMutex); CloseHandle(g_hMutex); g_hMutex = nullptr; }
        if (g_lockOnExit) LockWorkStation();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    // Parse command line arguments manually
    // Collect up to 4 tokens: exe [arg1] [arg2]
    const char* argv[4] = {};
    int argc = 0;
    {
        const char* cl = GetCommandLineA();
        static char argbuf[MAX_PATH * 4];
        char* w = argbuf;
        while (*cl && argc < 4) {
            while (*cl == ' ') cl++;
            if (!*cl) break;
            const char* start;
            if (*cl == '"') {
                cl++;
                start = cl;
                while (*cl && *cl != '"') cl++;
            } else {
                start = cl;
                while (*cl && *cl != ' ') cl++;
            }
            int len = (int)(cl - start);
            argv[argc++] = w;
            for (int i = 0; i < len; ++i) *w++ = start[i];
            *w++ = '\0';
            if (*cl) cl++;
        }
    }
    // argv[0] = exe, argv[1..] = user args

    // ---------------------------------------------------------------------------
    // Screensaver mode handling
    // ---------------------------------------------------------------------------
    for (int i = 1; i < argc; ++i) {
        const char* a = argv[i];
        if (*a == '/' || *a == '-') a++;
        char ch = a[0] | 0x20; // lowercase
        // /c — config, /a — password (obsolete): exit immediately
        if ((ch == 'c' || ch == 'a') && a[1] == '\0') return 0;
        // /lock — lock workstation when shader is dismissed
        if (ch == 'l' && (a[1] | 0x20) == 'o') g_lockOnExit = true;
        // /p HWND — preview: create a child window that stays alive (black)
        if (ch == 'p' && a[1] == '\0' && i + 1 < argc) {
            LONG_PTR hwndVal = 0;
            for (const char* c = argv[i + 1]; *c >= '0' && *c <= '9'; c++)
                hwndVal = hwndVal * 10 + (*c - '0');
            HWND parent = (HWND)hwndVal;
            if (parent) {
                // Idle message loop — keeps preview alive until parent closes
                MSG msg;
                while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
                    TranslateMessage(&msg);
                    DispatchMessageW(&msg);
                }
            }
            return 0;
        }
    }

    // ---------------------------------------------------------------------------
    // Single instance mutex — wait up to 3 seconds, then bail
    // ---------------------------------------------------------------------------
    g_hMutex = CreateMutexA(nullptr, FALSE, "Global\\AVS_Shader_Screensaver_Mutex");
    if (g_hMutex) {
        DWORD waitResult = WaitForSingleObject(g_hMutex, 3000);
        if (waitResult == WAIT_TIMEOUT || waitResult == WAIT_FAILED) {
            CloseHandle(g_hMutex);
            g_hMutex = nullptr;
            return 0; // another instance is running
        }
    }

    // ---------------------------------------------------------------------------
    // Resolve shader and launch
    // ---------------------------------------------------------------------------
    // Find shader file path from args (skip /s and other flags)
    const char* diskShaderPath = nullptr;
    for (int i = 1; i < argc; ++i) {
        const char* a = argv[i];
        if (*a != '/' && *a != '-') { diskShaderPath = a; break; }
    }

    // Resolve shader source: CLI file > embedded resources
    char* fragSrc = nullptr;
    if (diskShaderPath) {
        fragSrc = ReadFile(diskShaderPath);
        if (!fragSrc) {
            char msg[MAX_PATH + 64];
            wsprintfA(msg, "Could not open: %s", diskShaderPath);
            MessageBoxA(nullptr, msg, "Error", MB_OK | MB_ICONERROR);
            return 1;
        }
    } else {
        EnumerateEmbeddedShaders();
        fragSrc = PickEmbeddedShader();
    }

    if (!fragSrc) {
        MessageBoxA(nullptr, "No shaders embedded and none specified on command line.",
                    "Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // --------------- Fullscreen mode ---------------
    int vx = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int vy = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int vw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int vh = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    EnumerateMonitors();
    if (g_needsCulling) BuildMonitorGLRects(vx, vy, vw, vh);

    HINSTANCE hInst = GetModuleHandleW(nullptr);

    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = L"GreenOverlay";
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wc.hIcon         = LoadIconA(hInst, "IDI_ICON1");
    wc.hIconSm       = wc.hIcon;
    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        L"GreenOverlay", L"",
        WS_POPUP | WS_VISIBLE,
        vx, vy, vw, vh,
        nullptr, nullptr, hInst, nullptr
    );

    g_w = vw;
    g_h = vh;
    g_startMs = GetTickCount();

    if (!InitGL(hwnd, fragSrc)) return 1;
    HeapFree(GetProcessHeap(), 0, fragSrc);

    if (!g_shaderHasClock) CreateClockWindow(hInst, hwnd);

    // Move mouse to bottom of screen
    SetCursorPos(vx + vw / 2, vy + vh - 1);

    // Active render loop — render every frame, dispatch queued messages
    MSG msg = {};
    for (;;) {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) goto done;
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        Render();
    }
done:
    FreeClockGDI();
    if (g_clockWnd) { DestroyWindow(g_clockWnd); g_clockWnd = nullptr; }
    if (g_hMutex) { ReleaseMutex(g_hMutex); CloseHandle(g_hMutex); g_hMutex = nullptr; }
    return (int)msg.wParam;
}
