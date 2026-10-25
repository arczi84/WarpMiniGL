# Rendering into the game target without GLReadPixels

Status: original V25 API audit, 17 September 2026.

Update V27_2: the author has now supplied MGLCreateContextFromBitMap(BitMap *).
WarpMiniGL 4.15 TARGET implements this published constructor and uses the
documented MGLSwitchDisplay behavior to finish without presenting. It accepts
whole four-byte-per-pixel targets only. Retargeting, subrectangles and 16-bit
conversion remain absent from this SDK. The three-function proposal below is
historical design guidance, not the signature implemented by V27_2.

Update 20 September: the supplied V27 SDK now publishes MGLCreateContextFromWindow.
WarpMiniGL 4.13/4.14 WINDOW implements that interface experimentally. The user
reports alternating game image and gray content with 4.14. In the available
Classic MiniGL source, MGLSwitchDisplay calls MGLUnlockDisplay (which reaches
our W3D_UnLockHardware and direct presentation) and then ClipBlit from the game's
offscreen bitmap. WINDOW never fills that bitmap. This source sequence explains
the reported overwrite; it is not an execution trace of the installed binary.

The game must remain unchanged. Disabling its blit, rebuilding its static
MiniGL, or patching its executable is outside the solution. Borrowed-bitmap
rendering below is a proposed backend extension, not an implemented or published
MiniGL API. Local V27 files are SDK headers, not the current PiStorm renderer
sources; the older partial V18 renderer cannot establish V27 behavior.

## Concrete API proposal for the MiniGL backend maintainer

Proposed names/signatures, to be agreed and published in the official SDK:

```c
GLcontext MGLCreateContextFromBitMap(struct BitMap *bitmap,
    GLint x, GLint y, GLsizei width, GLsizei height);
GLboolean MGLSetRenderTarget(GLcontext context, struct BitMap *bitmap,
    GLint x, GLint y, GLsizei width, GLsizei height);
GLboolean MGLFinishRenderTarget(GLcontext context);
```

- **Create:** borrow the bitmap and the specified rectangle, without opening or
  searching for a window/screen. Coordinates are top-left bitmap coordinates;
  GL viewport/scissor coordinates remain local to the rectangle. Validate bounds,
  pitch, format and GPU accessibility through the RTG API, not BitMap internals.
  Return NULL on failure without changing the current context or freeing the
  caller's resources. The client macro sets currentContext only on success.
- **Retarget:** first complete pending writes to the old bitmap, then bind the
  new rectangle. Retain textures and GL state. Return GL_FALSE on failure and
  leave the old target usable, without partial dimension/target changes. A size
  change may recreate internal render/depth storage; viewport and scissor remain
  under caller control. Invalid targets must not corrupt the previous binding.
- **Finish:** submit ALL accumulated drawing, wait for GPU completion and any
  destination conversion/copy, make writes visible to RTG/CPU readers, and release
  target locks before returning GL_TRUE. GLFinish must provide compatible GL
  completion semantics, but this explicit entry also reports submission/resolve
  failure to Warp3D. No window blit, screen swap, focus change or IDCMP access.
  A second finish without new drawing must not resubmit the previous frame.
- **Formats/performance:** retain a 32-bit internal render path with defined
  channel order and alpha. Render directly to compatible RTG targets. For other
  supported targets, use a backend-native resolve/copy/conversion. The reported
  Hexen mode requests 16-bit color, so RGB565 targets must be tested, not assumed
  compatible with a 32-bit GPU store. Preserve alpha for supported 32-bit targets;
  a 16-bit target has no alpha channel. Define exact supported RTG formats and
  byte order, and reject others cleanly. Do not hide a full-frame GLReadPixels +
  CPU conversion/upload behind this API and call it the fast path.
- **Destroy:** existing MGLDeleteContext completes outstanding accesses before
  releasing backend-owned resources. Never free the borrowed bitmap or close a
  caller-owned window/screen. The caller keeps its bitmap alive through context
  deletion; target replacement finishes use of the old bitmap before returning.
- **Publish:** append entries after V27's MGLCreateContextFromWindow, preserving
  every existing offset. Update the runtime table, public header and client
  macros together. Check structSize through the last required entry and non-NULL
  pointers before use. Do not assign speculative offsets in the wrapper's V27 SDK.
  Unsupported backends must expose absence/failure rather than a successful stub.

WarpMiniGL maps W3D_CreateContext to Create(bitmap, 0, yoffset, width, height),
W3D_SetDrawRegion to Retarget, and unlock/flush/wait to Finish. It must propagate
failures instead of clearing its dirty flag or committing a failed new target.
The game's original blit/screen swap then presents the completed bitmap.

Acceptance requires a test that **blits the supplied offscreen bitmap after
Warp3D unlock**, exactly as Classic MiniGL does. Checking only the visible window
misses this bug. Also test RGB565 and supported 32-bit formats, nonzero offsets,
alternating target bitmaps, preservation outside the target rectangle, failed
retarget, and context deletion while the game still owns its bitmap/window.

## Findings in available sources

- The V25 public dispatch header in
  `../PiStorm3D_V25_Classic/dev/MiniGL_Library_Source_Code/include/libraries/minigl_dispatch.h`
  exposes context creation by dimensions or display mode ID, window-handle getters,
  and resize. It exposes no constructor accepting an existing Window or BitMap,
  and no setter for a caller-owned rendering destination.
- That tree's README explicitly records removal of the experimental
  MGLCreateContextFromWindow API from the mainline-derived interface.
- The older V18 experimental dispatch has MGLCreateContextFromWindow, and the
  partial PiStorm renderer implements it by borrowing a Window and Screen,
  allocating its own 32-bit render bitmap, and presenting to the borrowed Window.
  It is not the V25 ABI. Its appended slot overlaps unrelated V23/V25 entries;
  casting a V25 table to that older tail could call the wrong function.
- These findings concern the available SDK/source snapshots. The user's installed
  V25 binary has not been inspected. The local V25 Classic backend is not the
  independent PiStorm backend and must not replace it in this configuration.

## Why a borrowed window alone is insufficient for an unchanged Warp3D game

WarpMiniGL receives W3D_CC_BITMAP and W3D_CC_YOFFSET in W3D_CreateContext.
W3D_SetDrawRegion can subsequently select another bitmap or offset. The supplied
bitmap may be the game's back buffer, not the window's currently visible bitmap.
The game continues to own presentation, input and its own window lifecycle.
Drawing only to its window can be overwritten by the game's later blit or buffer
swap. There is no standard Window pointer in the tags this wrapper consumes;
Intuition's active window is not a reliable substitute for a rendering target.

The robust integration point is therefore the caller-owned destination bitmap.
Creating another visible context or reading a helper window cannot provide this
contract. A borrowed-window API is useful for applications that explicitly hand
presentation to MiniGL, but cannot alone guarantee unchanged Warp3D semantics.

## Proposed minimal backend capability

Names and signatures must be agreed with the actual MiniGL backend maintainer;
no unassigned dispatch slot or private context offset should be used.

1. Create a MiniGL context targeting an existing RTG BitMap with an explicit
   rectangle/offset and dimensions, without opening a window or screen.
2. Retarget an existing context to another borrowed bitmap/offset, preserving
   textures and GL state. Resizing should preserve the context or report a
   defined failure without destroying the old target.
3. Submit accumulated rendering and wait for completion in that target before
   Warp3D unlock/flush returns. No implicit window blit, screen swap, or input
   message consumption. Specify whether the existing GLFlush/GLFinish pair has
   these submission/completion semantics; otherwise expose a target flush call.
4. Keep ownership with the caller: deleting the MiniGL context must not free the
   bitmap, close its window, unlock a screen it did not lock, or change IDCMP.
   Lock RTG memory while GPU access is outstanding and do not cache an unlocked
   bitmap address across retargets/presentation. Reject unsupported target formats,
   alignment or memory domains explicitly. Preserve all four channels in supported
   32-bit formats and define their byte order.
5. Publish capability/version/size checks and append agreed dispatch entries
   after the published V27 tail. Preserve the prefix and define function-pointer
   availability checks. Supporting a feature in a static archive alone does not
   expose it to clients of minigl.library.

The backend may use GPU/RTG-native resolve/conversion when necessary; actual
zero-copy feasibility depends on its memory/format constraints. The required
performance property is no per-frame CPU GLReadPixels-to-P96 transfer.

## Wrapper mapping once the backend contract is available

- Context.c / W3D_CreateContext: pass the supplied bitmap, yoffset and dimensions
  to borrowed-target creation instead of wm_create opening a helper window.
- Effect.c / W3D_SetDrawRegion: flush the old target, retarget, then commit bitmap,
  dimensions and scissor only on success. A failure leaves the previous binding.
- Hardware.c: use target submission/completion at unlock, wait and flush; remove
  the full-frame readback/copy from this presentation mode.
- Context destruction: finish outstanding GPU work, delete only renderer-owned
  resources, and leave the game's bitmap/window intact.
- Keep the hardware-confirmed 4.10 perspective/color path unchanged.

## Acceptance checks on the actual independent PiStorm backend

- Smoke renders its triangle into its own destination bitmap, with correct RGBA,
  orientation and nonzero yoffset, without creating another Window.
- Switching two same-sized back buffers updates only the selected bitmap.
- Resizing/retarget failure preserves the old context and its textures.
- A game owns one input/render window; no separate gray focus window remains.
- Window close, game exit, context recreation and library unload have no borrowed
  resource frees or stale GPU accesses.
- Compare the same scene/resolution against 4.10 DIRECT and 4.11/4.12 READPIXELS.
  No performance or crash-fix claim is established by host mocks alone.

Until that interface is available, keep 4.10 DIRECT as the known fast fallback.
Both 4.11 and 4.12 READPIXELS were reported slow by the user; 4.12's reduced
wrapper overhead did not remove the slowdown.
