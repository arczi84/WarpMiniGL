#include <stddef.h>
#include <proto/exec.h>
#include <libraries/minigl.h>
#include <libraries/minigl_dispatch.h>

/* Implemented by the tiny compiler-specific import stub. */
extern const MGLDispatchTable *MiniGLGetDispatchTableLVO(void);

/*
 * MiniGLOpen() deliberately accepts minigl.library V14 and newer.
 *
 * The V18 dispatch table only extends the old table by appending new
 * functions.  Requiring sizeof(MGLDispatchTable) here would therefore
 * reject an otherwise compatible V14-V17 library simply because it
 * does not contain the V18 tail.
 *
 * GLClientActiveTextureARB is the first entry appended after the
 * V14-compatible dispatch prefix, so its offset is the minimum table
 * size required by the original API.
 */
#define MINIGL_MIN_DISPATCH_SIZE \
    offsetof(MGLDispatchTable, GLClientActiveTextureARB)

BOOL MiniGLOpen(void)
{
    const MGLDispatchTable *dispatch;

    if (MiniGLBase && MiniGLDispatch)
        return TRUE;

    if (!MiniGLBase) {
        MiniGLBase = OpenLibrary(MINIGLNAME, MINIGL_MIN_VERSION);
        if (!MiniGLBase)
            return FALSE;
    }

    dispatch = MiniGLGetDispatchTableLVO();
    if (!dispatch) {
        CloseLibrary(MiniGLBase);
        MiniGLBase = NULL;
        return FALSE;
    }

    if (dispatch->abiVersion < MINIGL_DISPATCH_ABI_VERSION ||
        dispatch->structSize < MINIGL_MIN_DISPATCH_SIZE)
    {
        CloseLibrary(MiniGLBase);
        MiniGLBase = NULL;
        MiniGLDispatch = NULL;
        return FALSE;
    }

    MiniGLDispatch = dispatch;
    return TRUE;
}

void MiniGLClose(void)
{
    MiniGLDispatch = NULL;

    if (MiniGLBase) {
        CloseLibrary(MiniGLBase);
        MiniGLBase = NULL;
    }
}
