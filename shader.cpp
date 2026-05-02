#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <gl/GL.h>
#include <stdarg.h>

// ---------------------------------------------------------------------------
// Optional debug logging. Define AVS_LOG (here or via /DAVS_LOG on the cl
// command line) to write a timestamped trace to %TEMP%\avs_shader.log.
// Compiles away to nothing when AVS_LOG is not defined.
// ---------------------------------------------------------------------------
//#define AVS_LOG

#ifdef AVS_LOG
static HANDLE g_logHandle = INVALID_HANDLE_VALUE;
static void LogF(const char *fmt, ...)
{
    if (g_logHandle == INVALID_HANDLE_VALUE)
    {
        char path[MAX_PATH];
        const char *fname = "avs_shader.log";
        int pathLen = lstrlenA(path);
        int nameLen = lstrlenA(fname);
        if (pathLen + nameLen + 1 >= MAX_PATH)
            return;
        for (int i = 0; i <= nameLen; ++i)
            path[pathLen + i] = fname[i];
        g_logHandle = CreateFileA(path, FILE_APPEND_DATA, FILE_SHARE_READ,
                                  nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (g_logHandle == INVALID_HANDLE_VALUE)
            return;
    }
    char buf[1024];
    SYSTEMTIME st;
    GetLocalTime(&st);
    int n = wsprintfA(buf, "[%04u-%02u-%02u %02u:%02u:%02u.%03u pid=%lu] ",
                      st.wYear, st.wMonth, st.wDay,
                      st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
                      GetCurrentProcessId());
    va_list ap;
    va_start(ap, fmt);
    int m = wvsprintfA(buf + n, fmt, ap);
    va_end(ap);
    if (m > 0)
        n += m;
    if (n < (int)sizeof(buf) - 1)
        buf[n++] = '\n';
    DWORD written;
    WriteFile(g_logHandle, buf, (DWORD)n, &written, nullptr);
}
#define LOG(...) LogF(__VA_ARGS__)
#else
#define LOG(...) ((void)0)
#endif

// ---- WGL extension prototypes (loaded at runtime) -------------------------
typedef HGLRC(WINAPI *PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC, HGLRC, const int *);
typedef BOOL(WINAPI *PFNWGLSWAPINTERVALEXTPROC)(int);
typedef GLuint(WINAPI *PFNGLCREATESHADERPROC)(GLenum);
typedef void(WINAPI *PFNGLSHADERSOURCEPROC)(GLuint, GLsizei, const char **, const GLint *);
typedef void(WINAPI *PFNGLCOMPILESHADERPROC)(GLuint);
typedef GLuint(WINAPI *PFNGLCREATEPROGRAMPROC)(void);
typedef void(WINAPI *PFNGLATTACHSHADERPROC)(GLuint, GLuint);
typedef void(WINAPI *PFNGLLINKPROGRAMPROC)(GLuint);
typedef void(WINAPI *PFNGLUSEPROGRAMPROC)(GLuint);
typedef void(WINAPI *PFNGLDELETESHADERPROC)(GLuint);
typedef void(WINAPI *PFNGLDELETEPROGRAMPROC)(GLuint);
typedef GLint(WINAPI *PFNGLGETUNIFORMLOCATIONPROC)(GLuint, const char *);
typedef void(WINAPI *PFNGLUNIFORM1FPROC)(GLint, float);
typedef void(WINAPI *PFNGLUNIFORM1IPROC)(GLint, int);
typedef void(WINAPI *PFNGLUNIFORM2FPROC)(GLint, float, float);
typedef void(WINAPI *PFNGLUNIFORM3FPROC)(GLint, float, float, float);
typedef void(WINAPI *PFNGLUNIFORM4FPROC)(GLint, float, float, float, float);
typedef void(WINAPI *PFNGLUNIFORM4FVPROC)(GLint, GLsizei, const float *);
typedef void(WINAPI *PFNGLGETSHADERIVPROC)(GLuint, GLenum, GLint *);
typedef void(WINAPI *PFNGLGETSHADERINFOLOGPROC)(GLuint, GLsizei, GLsizei *, char *);
typedef void(WINAPI *PFNGLGETPROGRAMIVPROC)(GLuint, GLenum, GLint *);
typedef void(WINAPI *PFNGLGETPROGRAMINFOLOGPROC)(GLuint, GLsizei, GLsizei *, char *);
typedef void(WINAPI *PFNGLACTIVETEXTUREPROC)(GLenum);

#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define GL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define GL_BGRA 0x80E1
#define GL_TEXTURE0 0x84C0
#define GL_TEXTURE1 0x84C1
#define GL_CLAMP_TO_EDGE 0x812F

static PFNGLCREATESHADERPROC glCreateShader;
static PFNGLSHADERSOURCEPROC glShaderSource;
static PFNGLCOMPILESHADERPROC glCompileShader;
static PFNGLCREATEPROGRAMPROC glCreateProgram;
static PFNGLATTACHSHADERPROC glAttachShader;
static PFNGLLINKPROGRAMPROC glLinkProgram;
static PFNGLUSEPROGRAMPROC glUseProgram;
static PFNGLDELETESHADERPROC glDeleteShader;
static PFNGLDELETEPROGRAMPROC glDeleteProgram;
static PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
static PFNGLUNIFORM1FPROC glUniform1f;
static PFNGLUNIFORM1IPROC glUniform1i;
static PFNGLUNIFORM2FPROC glUniform2f;
static PFNGLUNIFORM3FPROC glUniform3f;
static PFNGLUNIFORM4FPROC glUniform4f;
static PFNGLUNIFORM4FVPROC glUniform4fv;
static PFNGLGETSHADERIVPROC glGetShaderiv;
static PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
static PFNGLGETPROGRAMIVPROC glGetProgramiv;
static PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
static PFNGLACTIVETEXTUREPROC glActiveTexture;
static PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;

#define LOAD_GL(T, name) name = (T)wglGetProcAddress(#name)

// ---------------------------------------------------------------------------

static HDC g_hdc = nullptr;
static HGLRC g_hglrc = nullptr;
static GLuint g_prog = 0;
static int g_w = 1, g_h = 1;
static DWORD g_startMs = 0;
static float g_mx = 0.5f, g_my = 0.5f;
static bool g_shaderHasClock = false; // true if shader uses iDate (has its own clock)
static bool g_lockOnExit = false;     // lock workstation when shader is dismissed
static HANDLE g_hMutex = nullptr;     // single-instance mutex

// ---------------------------------------------------------------------------
// Monitor dead-zone culling
// ---------------------------------------------------------------------------
#define MAX_MONITORS 8
static RECT g_monRects[MAX_MONITORS]; // Windows coords (top-left origin)
static int g_monCount = 0;
static bool g_needsCulling = false; // true if monitors differ in size/position
static bool g_primaryOnly = false;  // true if shader uses iDate → confine to primary monitor
static RECT g_primaryRect = {};     // primary monitor rect (Windows coords)
static GLint g_uMonCount = -1;
static GLint g_uMonRects = -1;
static GLint g_uPrimRect = -1;

// GL-space monitor rects: vec4(xMin, yMin, xMax, yMax) with Y=0 at bottom
static float g_monGL[MAX_MONITORS * 4];
static float g_primGL[4]; // primary monitor rect in GL coords

static BOOL CALLBACK MonitorEnumProc(HMONITOR hMon, HDC, LPRECT rc, LPARAM)
{
    if (g_monCount < MAX_MONITORS)
        g_monRects[g_monCount++] = *rc;
    MONITORINFO mi = {};
    mi.cbSize = sizeof(mi);
    if (GetMonitorInfoW(hMon, &mi) && (mi.dwFlags & MONITORINFOF_PRIMARY))
        g_primaryRect = mi.rcMonitor;
    return TRUE;
}

static void EnumerateMonitors()
{
    g_monCount = 0;
    EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, 0);

    if (g_monCount < 2)
    {
        g_needsCulling = false;
        return;
    }

    // Check if all monitors share the same height and vertical position
    // If so, no dead zones exist — skip culling
    bool allSame = true;
    int h0 = g_monRects[0].bottom - g_monRects[0].top;
    int t0 = g_monRects[0].top;
    for (int i = 1; i < g_monCount; ++i)
    {
        int hi = g_monRects[i].bottom - g_monRects[i].top;
        int ti = g_monRects[i].top;
        if (hi != h0 || ti != t0)
        {
            allSame = false;
            break;
        }
    }
    g_needsCulling = !allSame;
}

static void BuildMonitorGLRects(int vx, int vy, int vw, int vh)
{
    // Convert Windows rects to GL coords:
    // GL x = winX - vx
    // GL y = (vh - 1) - (winY - vy)  → flip Y
    for (int i = 0; i < g_monCount; ++i)
    {
        float xMin = (float)(g_monRects[i].left - vx);
        float xMax = (float)(g_monRects[i].right - vx);
        // Flip Y: Windows top→GL bottom
        float yMin = (float)(vh - (g_monRects[i].bottom - vy));
        float yMax = (float)(vh - (g_monRects[i].top - vy));
        g_monGL[i * 4 + 0] = xMin;
        g_monGL[i * 4 + 1] = yMin;
        g_monGL[i * 4 + 2] = xMax;
        g_monGL[i * 4 + 3] = yMax;
    }

    g_primGL[0] = (float)(g_primaryRect.left - vx);
    g_primGL[2] = (float)(g_primaryRect.right - vx);
    g_primGL[1] = (float)(vh - (g_primaryRect.bottom - vy));
    g_primGL[3] = (float)(vh - (g_primaryRect.top - vy));
}

// GLSL prefix: declares uniforms and early-returns if pixel is outside all monitors
static const char *kCullPrefix = R"(
uniform int   _monCount;
uniform vec4  _monRects[8];
)";

// GLSL prefix for primary-only mode (shaders that use iDate, e.g. clocks).
// Remaps gl_FragCoord into primary-monitor-local space; pixels outside are blacked out at main entry.
static const char *kPrimPrefix = R"(
uniform vec4 _primRect;
#define gl_FragCoord (gl_FragCoord - vec4(_primRect.xy, 0.0, 0.0))
)";

// Cached uniform locations (set once after link, -1 = not present)
static GLint g_uTime = -1;
static GLint g_uResolution = -1;
static GLint g_uMouse = -1;
static GLint g_uITime = -1;
static GLint g_uIResolution = -1;
static GLint g_uIMouse = -1;
static GLint g_uITimeDelta = -1;
static GLint g_uIFrame = -1;
static GLint g_uIDate = -1;

// Minimal vertex shader — just passes through a fullscreen quad
static const char *kVertSrc = R"(
void main() {
    float x = float((gl_VertexID & 1) << 1) - 1.0;
    float y = float((gl_VertexID & 2)) - 1.0;
    gl_Position = vec4(x, y, 0.0, 1.0);
}
)";

static char *ReadFile(const char *path)
{
    HANDLE h = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ,
                           nullptr, OPEN_EXISTING, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE)
        return nullptr;
    DWORD size = GetFileSize(h, nullptr);
    char *buf = (char *)HeapAlloc(GetProcessHeap(), 0, size + 1);
    DWORD rd = 0;
    ReadFile(h, buf, size, &rd, nullptr);
    buf[rd] = '\0';
    CloseHandle(h);
    return buf;
}

// ---------------------------------------------------------------------------
// Embedded shader resources
// ---------------------------------------------------------------------------

struct ResEntry
{
    const char *name;
    const char *data;
    DWORD size;
};
static ResEntry g_mainShaders[256];
static int g_mainCount = 0;
static ResEntry g_exitShaders[64];
static int g_exitCount = 0;

// EnumResourceNamesA reuses a single internal buffer for the LPSTR name passed
// to the callback, so the pointer is only valid for the duration of that one
// callback invocation. Copy the name onto the heap before storing it.
static const char* DupResName(LPCSTR src)
{
    int n = lstrlenA(src);
    char* p = (char*)HeapAlloc(GetProcessHeap(), 0, (SIZE_T)n + 1);
    if (!p) return nullptr;
    for (int i = 0; i <= n; ++i) p[i] = src[i];
    return p;
}

// Resource names starting with "exit-" (case-insensitive) are exit/fade-out
// shaders; everything else is a main screensaver shader.
static bool IsExitShaderName(LPCSTR name)
{
    if (IS_INTRESOURCE(name))
        return false;
    char a = name[0];
    if (a >= 'A' && a <= 'Z')
        a = (char)(a + 32);
    char b = name[1];
    if (b >= 'A' && b <= 'Z')
        b = (char)(b + 32);
    char c = name[2];
    if (c >= 'A' && c <= 'Z')
        c = (char)(c + 32);
    char d = name[3];
    if (d >= 'A' && d <= 'Z')
        d = (char)(d + 32);
    char e = name[4];
    return a == 'e' && b == 'x' && c == 'i' && d == 't' && e == '-';
}

static BOOL CALLBACK EnumResNameProc(HMODULE hMod, LPCSTR type, LPSTR name, LONG_PTR lParam)
{
    if (IS_INTRESOURCE(name))
        return TRUE;
    HRSRC hr = FindResourceA(hMod, name, type);
    if (!hr)
        return TRUE;
    HGLOBAL hg = LoadResource(hMod, hr);
    if (!hg)
        return TRUE;
    DWORD sz = SizeofResource(hMod, hr);
    const char *ptr = (const char *)LockResource(hg);
    if (!ptr || sz == 0)
        return TRUE;
    if (IsExitShaderName(name))
    {
        if (g_exitCount < 64)
        {
            g_exitShaders[g_exitCount].name = DupResName(name);
            g_exitShaders[g_exitCount].data = ptr;
            g_exitShaders[g_exitCount].size = sz;
            g_exitCount++;
        }
    }
    else
    {
        if (g_mainCount < 256)
        {
            g_mainShaders[g_mainCount].name = DupResName(name);
            g_mainShaders[g_mainCount].data = ptr;
            g_mainShaders[g_mainCount].size = sz;
            g_mainCount++;
        }
    }
    return TRUE;
}

static void EnumerateEmbeddedShaders()
{
    HMODULE hMod = GetModuleHandleA(nullptr);
    EnumResourceNamesA(hMod, RT_RCDATA, EnumResNameProc, 0);
}

// xorshift32 PRNG, seeded from QPC + tick + PID for genuine entropy
static DWORD g_rngState = 0;
static DWORD XorshiftNext()
{
    DWORD x = g_rngState;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    g_rngState = x ? x : 0x9E3779B9u;
    return g_rngState;
}
static void SeedRng()
{
    LARGE_INTEGER qpc;
    QueryPerformanceCounter(&qpc);
    DWORD seed = (DWORD)qpc.QuadPart ^ (DWORD)(qpc.QuadPart >> 32) ^ GetTickCount() ^ GetCurrentProcessId();
    g_rngState = seed ? seed : 0xDEADBEEFu;
    for (int i = 0; i < 8; ++i)
        XorshiftNext();
}

// Returns heap-allocated null-terminated copy of a random embedded main shader.
static char *PickEmbeddedShader()
{
    if (g_mainCount == 0)
        return nullptr;
    SeedRng();
    int pick = (int)(XorshiftNext() % (DWORD)g_mainCount);
    LOG("choosing main shader: %s (idx %d/%d)",
        g_mainShaders[pick].name, pick, g_mainCount);
    DWORD sz = g_mainShaders[pick].size;
    char *buf = (char *)HeapAlloc(GetProcessHeap(), 0, sz + 1);
    for (DWORD i = 0; i < sz; ++i)
        buf[i] = g_mainShaders[pick].data[i];
    buf[sz] = '\0';
    return buf;
}

// Returns the chosen exit-shader resource. Index in g_exitShaders. The buffer
// returned is borrowed (points into the .exe's resource section) and is
// not heap-allocated — do not free.
static int g_exitPickedIdx = -1;
static const char *g_exitPickedSrc = nullptr;
static DWORD g_exitPickedSize = 0;
static void PickEmbeddedExitShader(int idx)
{
    if (g_exitCount == 0)
        return;
    if (idx < 0)
        idx = 0;
    idx %= g_exitCount;
    g_exitPickedIdx = idx;
    g_exitPickedSrc = g_exitShaders[idx].data;
    g_exitPickedSize = g_exitShaders[idx].size;
    LOG("choosing exit shader: %s (idx %d/%d)",
        g_exitShaders[idx].name, idx, g_exitCount);
}

static GLuint CompileShader(GLenum type, const char *src)
{
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &src, nullptr);
    glCompileShader(sh);
    GLint ok = 0;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        char log[2048];
        glGetShaderInfoLog(sh, sizeof(log), nullptr, log);
        MessageBoxA(nullptr, log, "Shader compile error", MB_OK | MB_ICONERROR);
    }
    return sh;
}

// Inject monitor culling code into fragment shader source
// Inserts uniform declarations after any #version/precision/extension lines,
// and a dead-zone check at the start of main()
static char *InjectCulling(const char *fragSrc)
{
    // Find insertion point for uniforms: after last #/precision line
    const char *uniformInsert = fragSrc;
    const char *scan = fragSrc;
    while (*scan)
    {
        // Skip lines starting with #, or containing "precision"
        if (*scan == '#' || (scan == fragSrc || *(scan - 1) == '\n'))
        {
            const char *lineStart = scan;
            // Check if line starts with # or "precision"
            bool isDirective = (*scan == '#');
            bool isPrec = (strncmp(scan, "precision", 9) == 0);
            if (isDirective || isPrec)
            {
                // Skip to end of line
                while (*scan && *scan != '\n')
                    scan++;
                if (*scan == '\n')
                    scan++;
                uniformInsert = scan;
                continue;
            }
        }
        // Skip to next line
        while (*scan && *scan != '\n')
            scan++;
        if (*scan == '\n')
            scan++;
        break;
    }

    // Find "void main()" to insert the check
    const char *mainPos = strstr(fragSrc, "void main()");
    if (!mainPos)
        mainPos = strstr(fragSrc, "void main ()");
    if (!mainPos)
        mainPos = strstr(fragSrc, "void main(");
    if (!mainPos)
        return nullptr;

    // Find the opening '{' after main
    const char *brace = strchr(mainPos, '{');
    if (!brace)
        return nullptr;
    brace++; // past the '{'

    const char *cullCheck = "\n"
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
    int checkLen = lstrlenA(cullCheck);
    int srcLen = lstrlenA(fragSrc);
    int outLen = srcLen + prefixLen + checkLen + 16;

    char *out = (char *)HeapAlloc(GetProcessHeap(), 0, outLen);
    char *w = out;

    // Copy up to uniform insertion point
    int uOff = (int)(uniformInsert - fragSrc);
    for (int i = 0; i < uOff; ++i)
        *w++ = fragSrc[i];

    // Insert uniform declarations
    const char *s = kCullPrefix;
    while (*s)
        *w++ = *s++;

    // Copy from uniform insertion point up to after main's '{'
    int bOff = (int)(brace - fragSrc);
    for (int i = uOff; i < bOff; ++i)
        *w++ = fragSrc[i];

    // Insert cull check
    s = cullCheck;
    while (*s)
        *w++ = *s++;

    // Copy rest of source
    const char *rest = brace;
    while (*rest)
        *w++ = *rest++;
    *w = '\0';

    return out;
}

// Inject primary-monitor-only confinement: remap gl_FragCoord and black out pixels outside primary
static char *InjectPrimaryOnly(const char *fragSrc)
{
    const char *uniformInsert = fragSrc;
    const char *scan = fragSrc;
    while (*scan)
    {
        if (*scan == '#' || (scan == fragSrc || *(scan - 1) == '\n'))
        {
            bool isDirective = (*scan == '#');
            bool isPrec = (strncmp(scan, "precision", 9) == 0);
            if (isDirective || isPrec)
            {
                while (*scan && *scan != '\n')
                    scan++;
                if (*scan == '\n')
                    scan++;
                uniformInsert = scan;
                continue;
            }
        }
        while (*scan && *scan != '\n')
            scan++;
        if (*scan == '\n')
            scan++;
        break;
    }

    const char *mainPos = strstr(fragSrc, "void main()");
    if (!mainPos)
        mainPos = strstr(fragSrc, "void main ()");
    if (!mainPos)
        mainPos = strstr(fragSrc, "void main(");
    if (!mainPos)
        return nullptr;

    const char *brace = strchr(mainPos, '{');
    if (!brace)
        return nullptr;
    brace++;

    // gl_FragCoord here is already remapped to primary-local by the macro.
    // Out-of-primary if local coord < 0 or >= (primMax - primMin).
    const char *primCheck = "\n"
                            "    {\n"
                            "      vec2 _sz = _primRect.zw - _primRect.xy;\n"
                            "      if (gl_FragCoord.x < 0.0 || gl_FragCoord.x >= _sz.x ||\n"
                            "          gl_FragCoord.y < 0.0 || gl_FragCoord.y >= _sz.y)\n"
                            "        { gl_FragColor = vec4(0.0,0.0,0.0,1.0); return; }\n"
                            "    }\n";

    int prefixLen = lstrlenA(kPrimPrefix);
    int checkLen = lstrlenA(primCheck);
    int srcLen = lstrlenA(fragSrc);
    int outLen = srcLen + prefixLen + checkLen + 16;

    char *out = (char *)HeapAlloc(GetProcessHeap(), 0, outLen);
    char *w = out;

    int uOff = (int)(uniformInsert - fragSrc);
    for (int i = 0; i < uOff; ++i)
        *w++ = fragSrc[i];

    const char *s = kPrimPrefix;
    while (*s)
        *w++ = *s++;

    int bOff = (int)(brace - fragSrc);
    for (int i = uOff; i < bOff; ++i)
        *w++ = fragSrc[i];

    s = primCheck;
    while (*s)
        *w++ = *s++;

    const char *rest = brace;
    while (*rest)
        *w++ = *rest++;
    *w = '\0';

    return out;
}

static bool BuildProgram(const char *fragSrc)
{
    char *cullSrc = nullptr;
    if (g_primaryOnly)
    {
        cullSrc = InjectPrimaryOnly(fragSrc);
        if (cullSrc)
            fragSrc = cullSrc;
    }
    else if (g_needsCulling)
    {
        cullSrc = InjectCulling(fragSrc);
        if (cullSrc)
            fragSrc = cullSrc;
    }

    GLuint vs = CompileShader(GL_VERTEX_SHADER, kVertSrc);
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fragSrc);

    g_prog = glCreateProgram();
    glAttachShader(g_prog, vs);
    glAttachShader(g_prog, fs);
    glLinkProgram(g_prog);

    GLint ok = 0;
    glGetProgramiv(g_prog, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        char log[2048];
        glGetProgramInfoLog(g_prog, sizeof(log), nullptr, log);
        MessageBoxA(nullptr, log, "Program link error", MB_OK | MB_ICONERROR);
        return false;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    if (cullSrc)
        HeapFree(GetProcessHeap(), 0, cullSrc);

    // Cache uniform locations
    g_uTime = glGetUniformLocation(g_prog, "time");
    g_uResolution = glGetUniformLocation(g_prog, "resolution");
    g_uMouse = glGetUniformLocation(g_prog, "mouse");
    g_uITime = glGetUniformLocation(g_prog, "iTime");
    g_uIResolution = glGetUniformLocation(g_prog, "iResolution");
    g_uIMouse = glGetUniformLocation(g_prog, "iMouse");
    g_uITimeDelta = glGetUniformLocation(g_prog, "iTimeDelta");
    g_uIFrame = glGetUniformLocation(g_prog, "iFrame");
    g_uIDate = glGetUniformLocation(g_prog, "iDate");

    // Monitor culling uniforms
    g_uMonCount = glGetUniformLocation(g_prog, "_monCount");
    g_uMonRects = glGetUniformLocation(g_prog, "_monRects");
    g_uPrimRect = glGetUniformLocation(g_prog, "_primRect");

    return true;
}

// PickRandomShader removed — embedded resources used instead

static bool InitGL(HWND hwnd, const char *fragSrcIn)
{
    // 1. Create a dummy context to get wglCreateContextAttribsARB
    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;

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
    if (tempCtx != ctx)
        wglDeleteContext(tempCtx);

    g_hdc = dc;
    g_hglrc = ctx;

    if (wglSwapIntervalEXT)
        wglSwapIntervalEXT(1); // vsync

    // 3. Load extension functions
    LOAD_GL(PFNGLCREATESHADERPROC, glCreateShader);
    LOAD_GL(PFNGLSHADERSOURCEPROC, glShaderSource);
    LOAD_GL(PFNGLCOMPILESHADERPROC, glCompileShader);
    LOAD_GL(PFNGLCREATEPROGRAMPROC, glCreateProgram);
    LOAD_GL(PFNGLATTACHSHADERPROC, glAttachShader);
    LOAD_GL(PFNGLLINKPROGRAMPROC, glLinkProgram);
    LOAD_GL(PFNGLUSEPROGRAMPROC, glUseProgram);
    LOAD_GL(PFNGLDELETESHADERPROC, glDeleteShader);
    LOAD_GL(PFNGLDELETEPROGRAMPROC, glDeleteProgram);
    LOAD_GL(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation);
    LOAD_GL(PFNGLUNIFORM1FPROC, glUniform1f);
    LOAD_GL(PFNGLUNIFORM1IPROC, glUniform1i);
    LOAD_GL(PFNGLUNIFORM2FPROC, glUniform2f);
    LOAD_GL(PFNGLUNIFORM3FPROC, glUniform3f);
    LOAD_GL(PFNGLUNIFORM4FPROC, glUniform4f);
    LOAD_GL(PFNGLUNIFORM4FVPROC, glUniform4fv);
    LOAD_GL(PFNGLGETSHADERIVPROC, glGetShaderiv);
    LOAD_GL(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog);
    LOAD_GL(PFNGLGETPROGRAMIVPROC, glGetProgramiv);
    LOAD_GL(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog);
    LOAD_GL(PFNGLACTIVETEXTUREPROC, glActiveTexture);

    // 4. Compile the shader source (already in memory)
    // Detect if shader has its own clock (uses iDate) — confine to primary monitor
    g_shaderHasClock = (strstr(fragSrcIn, "iDate") != nullptr);
    g_primaryOnly = g_shaderHasClock;
    bool ok = BuildProgram(fragSrcIn);
    return ok;
}

// Fade-out state — GPU crossfade between a captured shader frame and the
// uploaded desktop snapshot. The fade fragment shader is one of the embedded
// exit-*.glsl resources, picked at startup; standalone (uShader, uDesktop,
// uResolution, uFade only — no uMode dispatch).
static bool g_fadingOut = false;
static bool g_fadeOutRequested = false;
static bool g_fadeOutReady = false;
static HWND g_fadeOutReqMainWnd = nullptr;
static int g_fadeW = 0, g_fadeH = 0;
static GLuint g_shaderTex = 0;
static GLuint g_desktopTex = 0;
static GLuint g_fadeProg = 0;
static GLint g_uFadeUShader = -1;
static GLint g_uFadeUDesk = -1;
static GLint g_uFadeURes = -1;
static GLint g_uFadeUFade = -1;
static double g_fadeOutStartSec = 0.0;
static double g_fadeOutDurationSec = 0.500;

// Per-shader duration override. Looked up by the variant name (the part
// after the "exit-NN-" prefix in the resource name). Falls back to 0.500s.
static const struct { const char* name; double sec; } kExitDurations[] = {
    { "perlin",      0.800 },
    { "crossfade",   0.350 },
    { "crosswarp",   0.900 },
    { "ripple",      1.200 },
    { "poissonblur", 0.800 },
    { "boxblur",     0.800 },
};

static double LookupExitDuration(const char *name)
{
    if (!name) return 0.500;
    // Skip "exit-NN-" prefix to get to the variant name.
    int i = 0;
    while (name[i] && i < 32) {
        if (name[i] == '-' && i >= 6) { ++i; break; }
        ++i;
    }
    const char *tail = name + i;
    for (int k = 0; k < (int)(sizeof(kExitDurations) / sizeof(kExitDurations[0])); ++k) {
        if (lstrcmpiA(tail, kExitDurations[k].name) == 0)
            return kExitDurations[k].sec;
    }
    return 0.500;
}

// Test mode (triggered by '-' key): cycle through exit shaders without
// quitting. Each press uses the next exit shader in g_exitShaders[] order.
// Direction alternates between shader→desktop and desktop→shader.
static bool g_testActive = false;
static bool g_testRequested = false;
static int g_testNextIdx = 0;
static bool g_testToDesktop = true;
static bool g_testShowingDesk = false;
static bool g_testFrameCaptured = false;
static double g_testStartSec = 0.0;
static double g_testDurationSec = 0.500;

// Compile the given exit-shader source into g_fadeProg, replacing any
// previously linked one. Source is a borrowed pointer + size (RCDATA bytes
// are not null-terminated).
static bool BuildExitProgram(const char *src, DWORD srcLen)
{
    if (!src || srcLen == 0)
        return false;
    // Make a null-terminated copy on the stack/heap for glShaderSource.
    char *buf = (char *)HeapAlloc(GetProcessHeap(), 0, srcLen + 1);
    if (!buf)
        return false;
    for (DWORD i = 0; i < srcLen; ++i)
        buf[i] = src[i];
    buf[srcLen] = '\0';

    GLuint vs = CompileShader(GL_VERTEX_SHADER, kVertSrc);
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, buf);
    HeapFree(GetProcessHeap(), 0, buf);

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        char log[1024];
        glGetProgramInfoLog(prog, sizeof(log), nullptr, log);
        MessageBoxA(nullptr, log, "Exit-shader link error", MB_OK | MB_ICONERROR);
        glDeleteShader(vs);
        glDeleteShader(fs);
        glDeleteProgram(prog);
        return false;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);

    // Replace previous program (if any) and re-cache uniform locations.
    if (g_fadeProg)
        glDeleteProgram(g_fadeProg);
    g_fadeProg = prog;
    g_uFadeUShader = glGetUniformLocation(g_fadeProg, "uShader");
    g_uFadeUDesk = glGetUniformLocation(g_fadeProg, "uDesktop");
    g_uFadeURes = glGetUniformLocation(g_fadeProg, "uResolution");
    g_uFadeUFade = glGetUniformLocation(g_fadeProg, "uFade");
    return true;
}

// Build the fade-out crossfade program (from the chosen exit shader) and
// allocate the two textures used during fade. Called once after InitGL
// succeeds. desktopBits is a top-down BGRA buffer of size w*h.
static bool InitFadeOutGL(int w, int h, const void *desktopBits)
{
    if (g_exitCount == 0)
    {
        // No exit shaders embedded — fall back to instant exit (no fade).
        g_fadeOutReady = false;
        return false;
    }
    // Pick a random exit shader for this session.
    SeedRng();
    int pick = (int)(XorshiftNext() % (DWORD)g_exitCount);
    PickEmbeddedExitShader(pick);
    g_fadeOutDurationSec = LookupExitDuration(g_exitShaders[pick].name);
    if (!BuildExitProgram(g_exitPickedSrc, g_exitPickedSize))
        return false;

    // Shader-frame texture: allocated empty, populated each fade via
    // glCopyTexImage2D. Reserve full size up front.
    glGenTextures(1, &g_shaderTex);
    glBindTexture(GL_TEXTURE_2D, g_shaderTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Desktop-snapshot texture: uploaded once from the captured DIB. BGRA
    // top-down; exit shaders flip V to compensate.
    glGenTextures(1, &g_desktopTex);
    glBindTexture(GL_TEXTURE_2D, g_desktopTex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, desktopBits);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    glFinish();
    g_fadeOutReady = true;
    return true;
}

// Shader time pause state — when true, Render() uses g_pauseBeganAtMs as the
// clock so t is frozen. On unpause, g_startMs is advanced by the pause
// duration so t resumes seamlessly.
static bool g_shaderPaused = false;
static DWORD g_pauseBeganAtMs = 0;

static void Render()
{
    glViewport(0, 0, g_w, g_h);
    glUseProgram(g_prog);

    DWORD nowMs = g_shaderPaused ? g_pauseBeganAtMs : GetTickCount();
    float t = (float)(nowMs - g_startMs) / 1000.0f;
    float mx = g_mx * g_w, my = g_my * g_h;

    // In primary-only mode, the shader's coordinate space is the primary monitor
    // (gl_FragCoord is remapped via macro and iResolution should match).
    float resW = (float)g_w, resH = (float)g_h;
    if (g_primaryOnly)
    {
        resW = g_primGL[2] - g_primGL[0];
        resH = g_primGL[3] - g_primGL[1];
    }

    if (g_uTime != -1)
        glUniform1f(g_uTime, t);
    if (g_uResolution != -1)
        glUniform2f(g_uResolution, resW, resH);
    if (g_uMouse != -1)
        glUniform2f(g_uMouse, mx, my);
    if (g_uITime != -1)
        glUniform1f(g_uITime, t);
    if (g_uIResolution != -1)
        glUniform3f(g_uIResolution, resW, resH, 1.0f);
    if (g_uIMouse != -1)
        glUniform4f(g_uIMouse, mx, my, 0.0f, 0.0f);
    if (g_uITimeDelta != -1)
        glUniform1f(g_uITimeDelta, 0.016f);
    if (g_uIFrame != -1)
        glUniform1i(g_uIFrame, (int)(t * 60.0f));
    if (g_uIDate != -1)
    {
        SYSTEMTIME st;
        GetLocalTime(&st);
        float secsSinceMidnight = (float)st.wHour * 3600.0f + (float)st.wMinute * 60.0f + (float)st.wSecond;
        glUniform4f(g_uIDate, (float)st.wYear, (float)st.wMonth, (float)st.wDay, secsSinceMidnight);
    }

    // Monitor culling uniforms (only set if injection is active)
    if (g_uMonCount != -1)
        glUniform1i(g_uMonCount, g_monCount);
    if (g_uMonRects != -1)
        glUniform4fv(g_uMonRects, g_monCount, g_monGL);
    if (g_uPrimRect != -1)
        glUniform4f(g_uPrimRect, g_primGL[0], g_primGL[1], g_primGL[2], g_primGL[3]);

    // Draw fullscreen quad (4 vertices, no VBO needed with gl_VertexID trick)
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // If a fade-out was just requested, copy this frame's back buffer into
    // g_shaderTex before SwapBuffers — that's the same pixels the user is
    // about to see. glCopyTexImage2D stays GPU-side (no readback to CPU).
    if (g_fadeOutRequested && g_shaderTex)
    {
        glBindTexture(GL_TEXTURE_2D, g_shaderTex);
        glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, g_w, g_h, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    SwapBuffers(g_hdc);
}

static double NowSeconds(); // forward-declared (defined with the other timer code below)

// Render one crossfade frame. Returns the elapsed fade progress in [0,1]; the
// caller decides when to terminate. Runs entirely on the GPU using uShader and
// uDesktop textures + a uFade uniform.
static float RenderFadeOut()
{
    glViewport(0, 0, g_w, g_h);
    glUseProgram(g_fadeProg);

    double elapsed = NowSeconds() - g_fadeOutStartSec;
    float p = (float)(elapsed / g_fadeOutDurationSec);
    if (p < 0.0f)
        p = 0.0f;
    if (p > 1.0f)
        p = 1.0f;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_shaderTex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, g_desktopTex);
    glActiveTexture(GL_TEXTURE0);

    if (g_uFadeUShader != -1)
        glUniform1i(g_uFadeUShader, 0);
    if (g_uFadeUDesk != -1)
        glUniform1i(g_uFadeUDesk, 1);
    if (g_uFadeURes != -1)
        glUniform2f(g_uFadeURes, (float)g_w, (float)g_h);
    if (g_uFadeUFade != -1)
        glUniform1f(g_uFadeUFade, p);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    SwapBuffers(g_hdc);
    return p;
}

// Render one frame of the test-mode transition. Same as RenderFadeOut but uses
// g_testStartSec/g_testDurationSec, and may swap the texture bindings so the
// transition runs desktop → shader on alternating presses.
static float RenderTestTransition()
{
    glViewport(0, 0, g_w, g_h);
    glUseProgram(g_fadeProg);

    double elapsed = NowSeconds() - g_testStartSec;
    float p = (float)(elapsed / g_testDurationSec);
    if (p < 0.0f)
        p = 0.0f;
    if (p > 1.0f)
        p = 1.0f;

    // The shader is hardcoded to go uShader → uDesktop as uFade rises. To run
    // the reverse direction, bind shaderTex to sampler 1 and desktopTex to 0.
    GLuint texA = g_testToDesktop ? g_shaderTex : g_desktopTex;
    GLuint texB = g_testToDesktop ? g_desktopTex : g_shaderTex;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texA);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texB);
    glActiveTexture(GL_TEXTURE0);

    if (g_uFadeUShader != -1)
        glUniform1i(g_uFadeUShader, 0);
    if (g_uFadeUDesk != -1)
        glUniform1i(g_uFadeUDesk, 1);
    if (g_uFadeURes != -1)
        glUniform2f(g_uFadeURes, (float)g_w, (float)g_h);
    if (g_uFadeUFade != -1)
        glUniform1f(g_uFadeUFade, p);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    SwapBuffers(g_hdc);
    return p;
}

// Render a single static frame in test mode (between transitions): just
// composites the appropriate texture full-screen using mode 14 (plain mix)
// at uFade=0 or 1 as needed. Cheap.
static void RenderTestStatic()
{
    glViewport(0, 0, g_w, g_h);
    glUseProgram(g_fadeProg);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_shaderTex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, g_desktopTex);
    glActiveTexture(GL_TEXTURE0);

    if (g_uFadeUShader != -1)
        glUniform1i(g_uFadeUShader, 0);
    if (g_uFadeUDesk != -1)
        glUniform1i(g_uFadeUDesk, 1);
    if (g_uFadeURes != -1)
        glUniform2f(g_uFadeURes, (float)g_w, (float)g_h);
    if (g_uFadeUFade != -1)
        glUniform1f(g_uFadeUFade, g_testShowingDesk ? 1.0f : 0.0f);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    SwapBuffers(g_hdc);
}

// ---------------------------------------------------------------------------
// Clock overlay — layered window drawn with GDI+
// ---------------------------------------------------------------------------

static HWND g_clockWnd = nullptr;

// Set by WndProc when '-' is pressed; consumed by the render loop.
static void RequestTestTransition()
{
    if (!g_fadeOutReady)
        return; // texture still uploading
    if (g_fadingOut || g_fadeOutRequested)
        return; // real exit in progress
    if (g_testActive || g_testRequested)
        return; // already running one
    g_testRequested = true;
}

// Lightweight fade-out request — only sets a flag so input handlers return
// instantly. The render loop picks it up after the current frame, copies the
// just-drawn frame into g_shaderTex, then transitions into fade-out mode where
// RenderFadeOut() drives the GPU crossfade.
//
// If the fade-out machinery isn't ready yet (texture upload still in flight),
// just quit immediately — better an instant exit than a stutter.
static void RequestFadeOut(HWND mainWnd)
{
    if (g_fadingOut || g_fadeOutRequested)
        return;
    if (!g_fadeOutReady)
    {
        if (mainWnd)
            DestroyWindow(mainWnd);
        PostQuitMessage(0);
        return;
    }
    g_fadeOutRequested = true;
    g_fadeOutReqMainWnd = mainWnd;
}

// High-resolution timer — WM_TIMER and GetTickCount have ~15ms granularity
// which makes short fades (a few hundred ms) look stepped. QPC gives sub-ms
// resolution so the alpha curve advances smoothly each rendered frame.
static LARGE_INTEGER g_qpcFreq = {};
static double NowSeconds()
{
    if (!g_qpcFreq.QuadPart)
        QueryPerformanceFrequency(&g_qpcFreq);
    LARGE_INTEGER c;
    QueryPerformanceCounter(&c);
    return (double)c.QuadPart / (double)g_qpcFreq.QuadPart;
}

// Clock fade is driven from the main render loop using QPC time, not WM_TIMER,
// so the alpha ramp updates every frame rather than every ~15ms timer tick.
static double g_clockFadeStart = 0.0;
static bool g_clockFadeDone = false;

// Persistent clock GDI resources — created once, reused every tick
static const int CW = 600, CH = 170;
static HDC g_clockScreenDC = nullptr;
static HDC g_clockMemDC = nullptr;
static HDC g_clockTmpDC = nullptr;
static HBITMAP g_clockBmp = nullptr;
static HBITMAP g_clockTmpBmp = nullptr;
static HBITMAP g_clockOldBmp = nullptr;
static HBITMAP g_clockTmpOld = nullptr;
static DWORD *g_clockBits = nullptr;
static DWORD *g_clockTmpBits = nullptr;
static HFONT g_clockTimeFont = nullptr;
static HFONT g_clockDateFont = nullptr;

static void InitClockGDI()
{
    g_clockScreenDC = GetDC(nullptr);
    g_clockMemDC = CreateCompatibleDC(g_clockScreenDC);
    g_clockTmpDC = CreateCompatibleDC(g_clockScreenDC);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = CW;
    bmi.bmiHeader.biHeight = -CH;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    g_clockBmp = CreateDIBSection(g_clockMemDC, &bmi, DIB_RGB_COLORS, (void **)&g_clockBits, nullptr, 0);
    g_clockTmpBmp = CreateDIBSection(g_clockTmpDC, &bmi, DIB_RGB_COLORS, (void **)&g_clockTmpBits, nullptr, 0);
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
    if (g_clockTimeFont)
    {
        DeleteObject(g_clockTimeFont);
        g_clockTimeFont = nullptr;
    }
    if (g_clockDateFont)
    {
        DeleteObject(g_clockDateFont);
        g_clockDateFont = nullptr;
    }
    if (g_clockTmpDC)
    {
        SelectObject(g_clockTmpDC, g_clockTmpOld);
        DeleteDC(g_clockTmpDC);
        g_clockTmpDC = nullptr;
    }
    if (g_clockTmpBmp)
    {
        DeleteObject(g_clockTmpBmp);
        g_clockTmpBmp = nullptr;
    }
    if (g_clockMemDC)
    {
        SelectObject(g_clockMemDC, g_clockOldBmp);
        DeleteDC(g_clockMemDC);
        g_clockMemDC = nullptr;
    }
    if (g_clockBmp)
    {
        DeleteObject(g_clockBmp);
        g_clockBmp = nullptr;
    }
    if (g_clockScreenDC)
    {
        ReleaseDC(nullptr, g_clockScreenDC);
        g_clockScreenDC = nullptr;
    }
}

static void ClockDrawText(const wchar_t *text, HFONT font, int x, int y, DWORD argb)
{
    for (int i = 0; i < CW * CH; ++i)
        g_clockTmpBits[i] = 0;
    SelectObject(g_clockTmpDC, font);
    RECT rc = {0, 0, CW, CH};
    ExtTextOutW(g_clockTmpDC, 0, 0, 0, &rc, text, lstrlenW(text), nullptr);

    BYTE ar = (argb >> 24) & 0xFF, rr = (argb >> 16) & 0xFF;
    BYTE gr = (argb >> 8) & 0xFF, br = argb & 0xFF;

    for (int py = 0; py < CH; ++py)
    {
        for (int px = 0; px < CW; ++px)
        {
            BYTE cov = (BYTE)(g_clockTmpBits[py * CW + px] & 0xFF);
            if (cov == 0)
                continue;
            int dx = x + px, dy = y + py;
            if (dx < 0 || dx >= CW || dy < 0 || dy >= CH)
                continue;
            BYTE a = (BYTE)((ar * cov) / 255);
            DWORD &dst = g_clockBits[dy * CW + dx];
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
    if (!g_clockMemDC)
        InitClockGDI();

    MONITORINFO mi = {};
    mi.cbSize = sizeof(mi);
    POINT origin = {0, 0};
    GetMonitorInfoW(MonitorFromPoint(origin, MONITOR_DEFAULTTOPRIMARY), &mi);
    RECT mon = mi.rcMonitor;

    const int PAD = 36;
    int winX = mon.left + PAD;
    int winY = mon.bottom - CH - PAD;

    // Clear
    for (int i = 0; i < CW * CH; ++i)
        g_clockBits[i] = 0;

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

    // Hold invisible for 5s after shader start, then fade in over 1s.
    double elapsed = NowSeconds() - g_clockFadeStart;
    BYTE constAlpha;
    if (elapsed < 5.0)
        constAlpha = 0;
    else if (elapsed < 6.0)
        constAlpha = (BYTE)((elapsed - 5.0) * 255.0);
    else
        constAlpha = 255;

    POINT ptSrc = {0, 0};
    SIZE szWnd = {CW, CH};
    POINT ptDst = {winX, winY};
    BLENDFUNCTION bf = {};
    bf.BlendOp = AC_SRC_OVER;
    bf.SourceConstantAlpha = constAlpha;
    bf.AlphaFormat = AC_SRC_ALPHA;

    // No SetWindowPos — UpdateLayeredWindow with a non-null ptDst already moves
    // the window, and the size was set at creation time. Removing the redundant
    // SetWindowPos avoids per-frame WM_WINDOWPOSCHANGING/CHANGED traffic.
    UpdateLayeredWindow(hwnd, g_clockScreenDC, &ptDst, &szWnd, g_clockMemDC, &ptSrc, 0, &bf, ULW_ALPHA);
}

// Called from the main render loop while the fade-in is animating, so alpha
// advances on a per-frame basis using QPC time. Once the fade completes we
// stop driving from the loop and let the WM_TIMER handle minute updates.
static void TickClock()
{
    if (g_clockFadeDone || !g_clockWnd)
        return;
    DrawClock(g_clockWnd);
    if (NowSeconds() - g_clockFadeStart >= 6.0)
        g_clockFadeDone = true;
}

LRESULT CALLBACK ClockWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_TIMER:
        // Minute-tick redraw only; the fade-in is driven from the render loop.
        DrawClock(hwnd);
        return 0;
    // Forward quit keys to main window
    case WM_KEYDOWN:
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
        if (g_clockWnd)
            RequestFadeOut(GetParent(hwnd));
        return 0;
    case WM_DESTROY:
        KillTimer(hwnd, 1);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

static void CreateClockWindow(HINSTANCE hInst, HWND parent)
{
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = ClockWndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"ClockOverlay";
    wc.hbrBackground = nullptr;
    RegisterClassExW(&wc);

    // Created at the final clock-bitmap size so DrawClock never has to resize;
    // UpdateLayeredWindow's ptDst handles repositioning per frame on its own.
    g_clockWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT,
        L"ClockOverlay", L"",
        WS_POPUP | WS_VISIBLE,
        0, 0, CW, CH,
        parent, nullptr, hInst, nullptr);

    g_clockFadeStart = NowSeconds();
    g_clockFadeDone = false;
    DrawClock(g_clockWnd);
    // Minute-tick updates only — the 5s hold + 1s fade-in is driven per-frame
    // from the render loop via TickClock() using QPC time.
    SetTimer(g_clockWnd, 1, 1000, nullptr);
}

// ---------------------------------------------------------------------------
// Desktop fade overlay — captures the screen at startup and fades out over
// the shader. Layered + transparent so it doesn't steal input.
// On dismiss, the same DIB is reused to fade BACK to the desktop snapshot.
// ---------------------------------------------------------------------------
static HWND g_fadeWnd = nullptr;
static HBITMAP g_fadeBmp = nullptr;
static HDC g_fadeMemDC = nullptr;
static void *g_fadeBmpBits = nullptr; // raw bits of g_fadeBmp (DIB section)
// g_fadeW, g_fadeH declared above (forward-declared for Render())
static int g_fadeVx = 0, g_fadeVy = 0;
static HINSTANCE g_fadeHInst = nullptr;
static double g_fadeStartSec = 0.0;
static const double kFadeDurationSec = 0.350;
static int g_fadeHoldFrames = 0;

// Smoothstep easing: x in [0,1] → eased value in [0,1] with zero derivative
// at the endpoints. Used by the startup fade-in alpha animation.
static float SmoothstepF(float x)
{
    if (x <= 0.0f)
        return 0.0f;
    if (x >= 1.0f)
        return 1.0f;
    return x * x * (3.0f - 2.0f * x);
}

static LRESULT CALLBACK FadeWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);
        if (g_fadeMemDC)
            BitBlt(dc, 0, 0, g_fadeW, g_fadeH, g_fadeMemDC, 0, 0, SRCCOPY);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

static void CaptureDesktopAndCreateOverlay(BYTE startAlpha)
{
    g_fadeWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT,
        L"ShaderFadeOverlay", L"",
        WS_POPUP,
        g_fadeVx, g_fadeVy, g_fadeW, g_fadeH,
        nullptr, nullptr, g_fadeHInst, nullptr);
    if (!g_fadeWnd)
        return;
    SetLayeredWindowAttributes(g_fadeWnd, 0, startAlpha, LWA_ALPHA);
    ShowWindow(g_fadeWnd, SW_SHOW);
}

static void CreateFadeOverlay(HINSTANCE hInst, int vx, int vy, int vw, int vh)
{
    g_fadeHInst = hInst;
    g_fadeVx = vx;
    g_fadeVy = vy;
    g_fadeW = vw;
    g_fadeH = vh;

    // Capture the virtual desktop into a top-down 32-bit DIB section (kept
    // alive so we can a) BitBlt onto the fade overlay during startup and b)
    // upload the bits into a GL texture for the dismissal crossfade).
    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = vw;
    bi.bmiHeader.biHeight = -vh; // top-down
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    g_fadeBmp = CreateDIBSection(nullptr, &bi, DIB_RGB_COLORS, &g_fadeBmpBits, nullptr, 0);
    HDC screenDC = GetDC(nullptr);
    g_fadeMemDC = CreateCompatibleDC(screenDC);
    SelectObject(g_fadeMemDC, g_fadeBmp);
    BitBlt(g_fadeMemDC, 0, 0, vw, vh, screenDC, vx, vy, SRCCOPY);
    ReleaseDC(nullptr, screenDC);

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = FadeWndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"ShaderFadeOverlay";
    wc.hbrBackground = nullptr;
    RegisterClassExW(&wc);

    CaptureDesktopAndCreateOverlay(255);
    UpdateWindow(g_fadeWnd); // force the desktop snapshot to paint before the shader appears
    g_fadeStartSec = NowSeconds();
}

static void FreeFadeBitmap()
{
    if (g_fadeMemDC)
    {
        DeleteDC(g_fadeMemDC);
        g_fadeMemDC = nullptr;
    }
    if (g_fadeBmp)
    {
        DeleteObject(g_fadeBmp);
        g_fadeBmp = nullptr;
    }
}

// Startup fade-in tick: animates the desktop-snapshot overlay alpha 255 → 0
// over kFadeDurationSec. Dismissal fade-out is handled by RenderFadeOut() in
// the GL context — not here. Returns true while the startup fade is running.
static bool TickFadeOverlay()
{
    if (!g_fadeWnd)
        return false;
    double elapsed = NowSeconds() - g_fadeStartSec;
    if (elapsed >= kFadeDurationSec)
    {
        SetLayeredWindowAttributes(g_fadeWnd, 0, 0, LWA_ALPHA);
        if (++g_fadeHoldFrames >= 4)
        {
            DestroyWindow(g_fadeWnd);
            g_fadeWnd = nullptr;
            g_fadeHoldFrames = 0;
            return false;
        }
        return true;
    }
    float p = (float)(elapsed / kFadeDurationSec);
    BYTE alpha = (BYTE)((1.0f - SmoothstepF(p)) * 255.0f + 0.5f);
    SetLayeredWindowAttributes(g_fadeWnd, 0, alpha, LWA_ALPHA);
    return true;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_SIZE:
    {
        RECT rc;
        GetClientRect(hwnd, &rc);
        g_w = rc.right - rc.left;
        g_h = rc.bottom - rc.top;
        return 0;
    }

    case WM_MOUSEMOVE:
        if (g_w > 0 && g_h > 0)
        {
            g_mx = (float)LOWORD(lp) / g_w;
            g_my = 1.0f - (float)HIWORD(lp) / g_h; // flip Y for GL
        }
        return 0;

    case WM_SETCURSOR:
        SetCursor(LoadCursor(nullptr, IDC_ARROW));
        return TRUE;

    case WM_ERASEBKGND:
        return 1;

    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
        RequestFadeOut(hwnd);
        return 0;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        // '-' key cycles through transitions in test mode without quitting.
        if (wp == VK_OEM_MINUS || wp == VK_SUBTRACT)
        {
            RequestTestTransition();
            return 0;
        }
        RequestFadeOut(hwnd);
        return 0;

    case WM_DESTROY:
        if (g_hglrc)
        {
            wglMakeCurrent(nullptr, nullptr);
            wglDeleteContext(g_hglrc);
        }
        if (g_hMutex)
        {
            ReleaseMutex(g_hMutex);
            CloseHandle(g_hMutex);
            g_hMutex = nullptr;
        }
        if (g_lockOnExit)
            LockWorkStation();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    // Parse command line arguments manually
    // Collect up to 4 tokens: exe [arg1] [arg2]
    const char *argv[4] = {};
    int argc = 0;
    {
        const char *cl = GetCommandLineA();
        static char argbuf[MAX_PATH * 4];
        char *w = argbuf;
        while (*cl && argc < 4)
        {
            while (*cl == ' ')
                cl++;
            if (!*cl)
                break;
            const char *start;
            if (*cl == '"')
            {
                cl++;
                start = cl;
                while (*cl && *cl != '"')
                    cl++;
            }
            else
            {
                start = cl;
                while (*cl && *cl != ' ')
                    cl++;
            }
            int len = (int)(cl - start);
            argv[argc++] = w;
            for (int i = 0; i < len; ++i)
                *w++ = start[i];
            *w++ = '\0';
            if (*cl)
                cl++;
        }
    }
    // argv[0] = exe, argv[1..] = user args

    LOG("==== shader started ==== argc=%d cmdline='%s'", argc, GetCommandLineA());

    // ---------------------------------------------------------------------------
    // Screensaver mode handling
    // Windows spawns many /p (preview) and /c (config) instances — exit immediately
    // for any flag except /s (run) and /lock (our custom flag).
    // ---------------------------------------------------------------------------
    for (int i = 1; i < argc; ++i)
    {
        const char *a = argv[i];
        // If the argument is an existing file, treat it as a shader path (not a flag)
        if (GetFileAttributesA(a) != INVALID_FILE_ATTRIBUTES)
            continue;
        if (*a == '/' || *a == '-')
            a++;
        char ch = a[0] | 0x20; // lowercase
        // /lock — lock workstation when shader is dismissed
        if (ch == 'l' && (a[1] | 0x20) == 'o')
        {
            g_lockOnExit = true;
            continue;
        }
        // /s — run the screensaver (standard Windows flag): allow
        if (ch == 's' && a[1] == '\0')
            continue;
        // Everything else (/p, /c, /a, unknown flags): exit immediately
        return 0;
    }

    // ---------------------------------------------------------------------------
    // Single instance mutex — wait up to 3 seconds, then bail
    // ---------------------------------------------------------------------------
    g_hMutex = CreateMutexA(nullptr, FALSE, "Global\\AVS_Shader_Screensaver_Mutex");
    if (g_hMutex)
    {
        DWORD waitResult = WaitForSingleObject(g_hMutex, 3000);
        if (waitResult == WAIT_TIMEOUT || waitResult == WAIT_FAILED)
        {
            CloseHandle(g_hMutex);
            g_hMutex = nullptr;
            return 0; // another instance is running
        }
    }

    // ---------------------------------------------------------------------------
    // Resolve shader and launch
    // ---------------------------------------------------------------------------
    // Find shader file path from args (skip /s and other flags)
    const char *diskShaderPath = nullptr;
    for (int i = 1; i < argc; ++i)
    {
        const char *a = argv[i];
        // An existing file is always treated as a shader path
        if (GetFileAttributesA(a) != INVALID_FILE_ATTRIBUTES)
        {
            diskShaderPath = a;
            break;
        }
        if (*a != '/' && *a != '-')
        {
            diskShaderPath = a;
            break;
        }
    }

    // Resolve shader source: CLI file > embedded resources
    char *fragSrc = nullptr;
    if (diskShaderPath)
    {
        fragSrc = ReadFile(diskShaderPath);
        if (!fragSrc)
        {
            char msg[MAX_PATH + 64];
            wsprintfA(msg, "Could not open: %s", diskShaderPath);
            MessageBoxA(nullptr, msg, "Error", MB_OK | MB_ICONERROR);
            return 1;
        }
    }
    else
    {
        EnumerateEmbeddedShaders();
        LOG("enumerated embedded resources: %d main, %d exit",
            g_mainCount, g_exitCount);
        fragSrc = PickEmbeddedShader();
    }

    if (!fragSrc)
    {
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
    BuildMonitorGLRects(vx, vy, vw, vh);

    HINSTANCE hInst = GetModuleHandleW(nullptr);

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"GreenOverlay";
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wc.hIcon = LoadIconA(hInst, "IDI_ICON1");
    wc.hIconSm = wc.hIcon;
    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        L"GreenOverlay", L"",
        WS_POPUP,
        vx, vy, vw, vh,
        nullptr, nullptr, hInst, nullptr);

    g_w = vw;
    g_h = vh;

    if (!InitGL(hwnd, fragSrc))
        return 1;
    HeapFree(GetProcessHeap(), 0, fragSrc);

    // Move mouse to bottom of screen
    SetCursorPos(vx + vw / 2, vy + vh - 1);

    // Set t=0 reference here, just before the first frame is drawn —
    // doing this earlier counts shader compile + window setup time as t > 0,
    // making motion look like it skipped ahead by the time the user sees it.
    g_startMs = GetTickCount();
    // Render first frame offscreen so the window appears with shader content
    Render();

    // Create fade overlay BEFORE showing shader window, so the desktop snapshot
    // is captured cleanly. Overlay is created after shader window so it sits on top.
    CreateFadeOverlay(hInst, vx, vy, vw, vh);

    // Build the fade-out crossfade GL program + textures (shader frame +
    // desktop snapshot). Uploads the DIB bits captured just above. This
    // sets g_fadeOutReady = true on success.
    InitFadeOutGL(vw, vh, g_fadeBmpBits);

    // Show the clock only after the dismissal machinery is ready. The clock
    // appearing is the visual signal that the user can press a key to exit
    // smoothly. Before this point, any keypress quits instantly (no fade).
    if (!g_shaderHasClock && g_fadeOutReady)
        CreateClockWindow(hInst, hwnd);

    ShowWindow(hwnd, SW_SHOW);
    SetForegroundWindow(hwnd);
    SetFocus(hwnd);
    // Re-assert overlay z-order in case ShowWindow on shader bumped it.
    if (g_fadeWnd)
        SetWindowPos(g_fadeWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    // Active render loop — vsync via wglSwapIntervalEXT(1) paces SwapBuffers,
    // so the loop is naturally throttled. Don't insert Sleep() here: Windows'
    // default timer resolution rounds Sleep(1) up to ~15.6ms, which desyncs
    // from vsync and produces visible stutter.
    MSG msg = {};
    for (;;)
    {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                goto done;
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        if (g_fadingOut)
        {
            // GPU crossfade using the chosen exit-shader program.
            float p = RenderFadeOut();
            if (p >= 1.0f)
            {
                if (g_fadeOutReqMainWnd)
                {
                    DestroyWindow(g_fadeOutReqMainWnd);
                    g_fadeOutReqMainWnd = nullptr;
                }
                goto done;
            }
        }
        else if (g_testActive)
        {
            // Test-mode transition in progress. A real exit request takes
            // priority — abort test, run dismissal with the current program.
            if (g_fadeOutRequested)
            {
                g_testActive = false;
                g_testFrameCaptured = false;
                g_fadingOut = true;
                LOG("fade-out begin (test->exit): %s",
                    g_exitPickedIdx >= 0 ? g_exitShaders[g_exitPickedIdx].name : "(none)");
                g_fadeOutStartSec = NowSeconds();
                g_fadeOutRequested = false;
            }
            else
            {
                float p = RenderTestTransition();
                if (p >= 1.0f)
                {
                    g_testActive = false;
                    g_testShowingDesk = g_testToDesktop;
                    g_testNextIdx = (g_testNextIdx + 1) % g_exitCount;
                    g_testToDesktop = !g_testToDesktop;
                    // Compile next exit shader for the upcoming press.
                    PickEmbeddedExitShader(g_testNextIdx);
                    g_testDurationSec = LookupExitDuration(g_exitShaders[g_testNextIdx].name);
                    g_fadeOutDurationSec = g_testDurationSec;
                    BuildExitProgram(g_exitPickedSrc, g_exitPickedSize);
                }
            }
        }
        else if (g_testShowingDesk || g_testFrameCaptured)
        {
            // Idle between test transitions.
            if (g_fadeOutRequested)
            {
                g_testFrameCaptured = false;
                g_testShowingDesk = false;
                g_fadingOut = true;
                LOG("fade-out begin (idle->exit): %s",
                    g_exitPickedIdx >= 0 ? g_exitShaders[g_exitPickedIdx].name : "(none)");
                g_fadeOutStartSec = NowSeconds();
                g_fadeOutRequested = false;
            }
            else if (g_testRequested)
            {
                g_testActive = true;
                g_testRequested = false;
                g_testStartSec = NowSeconds();
            }
            else
            {
                RenderTestStatic();
            }
        }
        else
        {
            // Normal frame. If a fade-out or test was just requested, the
            // back-buffer copy in Render fires for fadeOutRequested, but for
            // a test request we capture explicitly below.
            Render();
            if (g_fadeOutRequested)
            {
                g_fadingOut = true;
                LOG("fade-out begin (normal->exit): %s",
                    g_exitPickedIdx >= 0 ? g_exitShaders[g_exitPickedIdx].name : "(none)");
                g_fadeOutStartSec = NowSeconds();
                if (!g_shaderPaused)
                {
                    g_shaderPaused = true;
                    g_pauseBeganAtMs = GetTickCount();
                }
                if (g_fadeWnd)
                {
                    DestroyWindow(g_fadeWnd);
                    g_fadeWnd = nullptr;
                }
                if (g_clockWnd)
                {
                    ShowWindow(g_clockWnd, SW_HIDE);
                }
                g_fadeOutRequested = false;
            }
            else if (g_testRequested)
            {
                // First test press: capture current shader frame, then flip
                // into test-active mode using the already-built g_fadeProg.
                glBindTexture(GL_TEXTURE_2D, g_shaderTex);
                glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, g_w, g_h, 0);
                glBindTexture(GL_TEXTURE_2D, 0);
                g_testFrameCaptured = true;
                if (!g_shaderPaused)
                {
                    g_shaderPaused = true;
                    g_pauseBeganAtMs = GetTickCount();
                }
                if (g_clockWnd)
                    ShowWindow(g_clockWnd, SW_HIDE);
                g_testActive = true;
                g_testRequested = false;
                g_testToDesktop = true;
                g_testStartSec = NowSeconds();
                // First press uses g_exitPickedIdx (the startup-chosen one).
                g_testNextIdx = g_exitPickedIdx;
                g_testDurationSec = g_fadeOutDurationSec;
            }
        }
        TickFadeOverlay();
        TickClock();
    }
done:
    if (g_fadeWnd)
    {
        DestroyWindow(g_fadeWnd);
        g_fadeWnd = nullptr;
    }
    FreeFadeBitmap();
    FreeClockGDI();
    if (g_clockWnd)
    {
        DestroyWindow(g_clockWnd);
        g_clockWnd = nullptr;
    }
    if (g_hMutex)
    {
        ReleaseMutex(g_hMutex);
        CloseHandle(g_hMutex);
        g_hMutex = nullptr;
    }
    return (int)msg.wParam;
}
