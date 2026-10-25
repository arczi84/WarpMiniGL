#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/resident.h>
#include <exec/libraries.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/dos.h>

struct ExecBase *SysBase;
struct DosLibrary *DOSBase;
/* ENV:WarpMiniGL/LogFile: every message is also appended to that file and the
 * file closed again, so the log survives a crash or reset (FFS updates the
 * file only on Close). Set by wm_log_open(). */
static char wm_logpath[128];
void qt_debug(const char *text)
{
    ULONG n = 0;
    BPTR output;
    /* Expunge can run outside a DOS process. Output() requires a Process. */
    if (!DOSBase || FindTask(NULL)->tc_Node.ln_Type != NT_PROCESS) return;
    while (text[n]) ++n;
    if (wm_logpath[0]) {
        BPTR f = Open((STRPTR)wm_logpath, MODE_READWRITE);
        if (f) { Seek(f, 0, OFFSET_END); Write(f, (APTR)text, n); Close(f); }
    }
    output = Output();
    if (!output) return;
    Write(output, (APTR)text, n);
}
/* ENV:WarpMiniGL/Trace=1: the first Warp3D state calls with their arguments. */
static int wm_trace_left;
static void wm_hex(ULONG v)
{
    char b[10]; int i;
    b[0] = ' ';
    for (i = 0; i < 8; ++i) b[8 - i] = "0123456789abcdef"[(v >> (i * 4)) & 15];
    b[9] = 0;
    qt_debug(b);
}
void wm_trace(const char *name, ULONG a, ULONG b, ULONG c)
{
    if (wm_trace_left <= 0) return;
    --wm_trace_left;
    qt_debug("[W3D] "); qt_debug(name); wm_hex(a); wm_hex(b); wm_hex(c); qt_debug("\n");
}
void wm_log_open(void)
{
    char value[8];
    wm_trace_left = DOSBase && GetVar("WarpMiniGL/Trace", value, sizeof(value), 0) > 0 && value[0] == '1' ? 1500 : 0;
    if (DOSBase && GetVar("WarpMiniGL/LogFile", wm_logpath, sizeof(wm_logpath), 0) <= 0) wm_logpath[0] = 0;
}
extern struct IntuitionBase *IntuitionBase;
extern struct GfxBase *GfxBase;
extern struct Library *P96Base;
extern void INIT_0_Warp3D(void), EXIT_0_Warp3D(void);
extern int wm_ready(void);
struct QTLibrary { struct Library lib; BPTR seglist; };
static const char qt_name[] = "Warp3D.library";
#if defined(WM_V291) && defined(WM_TARGET_COPY)
static const char qt_id[] = "$VER: Warp3D.library 4.20 (25.10.2026) WarpMiniGL TARGET COPY V29.1 ABI5\r\n";
#elif defined(WM_FRAME_TRACE)
static const char qt_id[] = "$VER: Warp3D.library 4.18 (26.9.2026) WarpMiniGL TARGET COPY TRACE V27_2\r\n";
#elif defined(WM_TARGET_COPY)
static const char qt_id[] = "$VER: Warp3D.library 4.18 (26.9.2026) WarpMiniGL TARGET COPY V27_2\r\n";
#elif defined(WM_BORROWED_BITMAP)
static const char qt_id[] = "$VER: Warp3D.library 4.15 (20.9.2026) WarpMiniGL TARGET DIRECT V27_2\r\n";
#elif defined(WM_BORROWED_WINDOW)
static const char qt_id[] = "$VER: Warp3D.library 4.15 (20.9.2026) WarpMiniGL WINDOW DIRECT V27\r\n";
#elif defined(WM_DIRECT_PRESENT)
static const char qt_id[] = "$VER: Warp3D.library 4.15 (20.9.2026) WarpMiniGL DIRECT\r\n";
#elif defined(WM_READPIXELS_ONLY)
static const char qt_id[] = "$VER: Warp3D.library 4.15 (20.9.2026) WarpMiniGL READPIXELS RGBA\r\n";
#else
static const char qt_id[] = "$VER: Warp3D.library 4.15 (20.9.2026) WarpMiniGL bitmap\r\n";
#endif
#ifdef WM_V291
#define QT_REVISION 20
#elif defined(WM_FRAME_TRACE)
#define QT_REVISION 18
#elif defined(WM_TARGET_COPY)
#define QT_REVISION 18
#else
#define QT_REVISION 15
#endif

static BPTR LibExpunge(struct QTLibrary *base __asm("a6"));
static struct QTLibrary *LibOpen(struct QTLibrary *base __asm("a6"))
{
    ++base->lib.lib_OpenCnt;
    base->lib.lib_Flags &= ~LIBF_DELEXP;
    return base;
}
static BPTR LibClose(struct QTLibrary *base __asm("a6"))
{
    qt_debug("[WarpMiniGL exit] LibClose: begin\n");
    if (base->lib.lib_OpenCnt) --base->lib.lib_OpenCnt;
    if (!base->lib.lib_OpenCnt && (base->lib.lib_Flags & LIBF_DELEXP)) return LibExpunge(base);
    qt_debug("[WarpMiniGL exit] LibClose: done (resident)\n");
    return 0;
}
static BPTR LibExpunge(struct QTLibrary *base __asm("a6"))
{
    BPTR seg;
    ULONG size;
    UBYTE *start;
    if (base->lib.lib_OpenCnt) { base->lib.lib_Flags |= LIBF_DELEXP; return 0; }
    seg = base->seglist;
    Remove((struct Node *)base);
    qt_debug("[WarpMiniGL exit] Expunge: dependencies begin\n");
    EXIT_0_Warp3D();
    qt_debug("[WarpMiniGL exit] Expunge: dependencies done; freeing library base\n");
    if (DOSBase) { CloseLibrary((struct Library *)DOSBase); DOSBase = 0; }
    size = base->lib.lib_NegSize + base->lib.lib_PosSize;
    start = (UBYTE *)base - base->lib.lib_NegSize;
    FreeMem(start, size);
    return seg;
}
static ULONG LibReserved(void) { return 0; }
static struct QTLibrary *LibInit(struct QTLibrary *base __asm("d0"),
    BPTR seglist __asm("a0"), struct ExecBase *sys __asm("a6"))
{
    SysBase = sys;
    base->lib.lib_Node.ln_Type = NT_LIBRARY;
    base->lib.lib_Node.ln_Name = (char *)qt_name;
    base->lib.lib_Flags = LIBF_SUMUSED | LIBF_CHANGED;
    base->lib.lib_Version = 4;
    base->lib.lib_Revision = QT_REVISION;
    base->lib.lib_IdString = (char *)qt_id;
    base->seglist = seglist;
    DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 37);
    { extern void wm_log_open(void); wm_log_open(); }
    qt_debug(qt_id);
    INIT_0_Warp3D();
    if (!IntuitionBase || !GfxBase || !P96Base || !wm_ready()) {
        EXIT_0_Warp3D();
        qt_debug("[WarpMiniGL] initialization failed\n");
        if (DOSBase) { CloseLibrary((struct Library *)DOSBase); DOSBase = 0; }
        FreeMem((UBYTE *)base - base->lib.lib_NegSize,
            base->lib.lib_NegSize + base->lib.lib_PosSize);
        return 0;
    }
    qt_debug("[WarpMiniGL] initialized; MiniGL backend\n");
    return base;
}
#include "vectors.inc"
static const struct { ULONG size; const APTR *vectors; APTR data; APTR init; } qt_init = {
    sizeof(struct QTLibrary), qt_vectors, 0, (APTR)LibInit
};
const struct Resident RomTag = {
    RTC_MATCHWORD, (struct Resident *)&RomTag, (APTR)(&RomTag + 1),
    RTF_AUTOINIT, 4, NT_LIBRARY, 0, (char *)qt_name, (char *)qt_id, (APTR)&qt_init
};
