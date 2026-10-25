# WarpMiniGL

Separate experimental **Warp3D.library -> minigl.library** wrapper, based on
QuarkTex's Amiga Warp3D implementation. Intended call chain:

```
unchanged game -> StormMesa agl.library -> WarpMiniGL Warp3D.library
              -> installed minigl.library -> its rendering backend
```


Current release: **v0.2 = Warp3D.library 4.20, TARGET COPY for MiniGL V29.1
(dispatch ABI 5)**. It runs Hyperion's FreeSpace (68k) through
MiniGL 29.x.

## Installing v0.2

Requirements: AmigaOS 3.x with Picasso96 (or a P96-compatible RTG system), an
installed **minigl.library 29.x** (dispatch ABI 5) whose renderer does NOT use
Warp3D itself (e.g. the PiStorm3D/V3D MiniGL), and a 15/16 or 32-bit RTG
screen/window for the game.

1. Back up the current library: `Copy LIBS:Warp3D.library LIBS:Warp3D.library.old`
2. Copy `Warp3D.library` from the release archive to `LIBS:`.
3. Optional check: `smoke` from the archive opens a window, draws a triangle
   through Warp3D -> MiniGL and prints PASS lines.
4. Reboot (or `Avail FLUSH`) so an already loaded Warp3D.library is replaced.

To go back, copy `Warp3D.library.old` over `LIBS:Warp3D.library` and reboot.

### Environment variables (all optional)

| Variable | Effect |
|---|---|
| `ENV:WarpMiniGL/Fake16` = `1` | With a 32-bit destination, give the program a 16-bit R5G6B5 copy of the bitmap as `drawmem`. **Needed for FreeSpace on a 32-bit Workbench**: the game draws its 2D (menus, HUD text, mouse pointer) with the CPU in 16 bits regardless of the reported format. Use `SetEnv` for one session or copy to `ENVARC:` to keep it. |
| `ENV:WarpMiniGL/LogFile` = path | Append every wrapper message to that file. The file is opened and closed per message, so the log survives a crash or reset. |
| `ENV:WarpMiniGL/Trace` = `1` | Log the first 1500 Warp3D state calls (SetState, blend/alpha modes, clears, texture formats, texture env) with their arguments. |

Example for FreeSpace windowed on a 32-bit Workbench:

```
MakeDir ENVARC:WarpMiniGL
Echo 1 >ENVARC:WarpMiniGL/Fake16
Copy ENVARC:WarpMiniGL ENV:WarpMiniGL ALL QUIET
```

## What changed in 4.20 (v0.2)

The FreeSpace work done on the ogles2 sibling (WarpGLES) applied to TARGET COPY:

- `W3D_Context.drawmem`, `bprow`, `format`, `depth` and `supportedfmt` describe
  the real destination (or the Fake16 copy). Games that draw with the CPU
  while the hardware is locked crashed on the NULL `drawmem` before.
- 15/16-bit destinations: MiniGL renders into a private 32-bit bitmap and
  Picasso96's `BltBitMap` converts when the frame is copied.
- CPU-drawn pixels survive 3D: before the first GPU drawing after
  `W3D_LockHardware` the destination's pixels are drawn into the renderer as
  256x256 textured quads (`gcc/seed.c`). Only public MiniGL calls are used, so
  this works with any MiniGL 29 backend. A frame without GPU drawing copies the
  Fake16 pixels straight into the destination at unlock.
- Warp3D default texture filtering is point sampling until `W3D_SetFilter`
  (FreeSpace never calls it; linear filtering smeared its fonts).
- Textures are tracked in the wrapper's own registry instead of
  `W3D_Texture.link`, and stale or foreign texture pointers are ignored
  (statically linked MiniGL copies in some games reuse that node).
- Logging to a file and a call trace (see the table above).
- The V29.1 SDK is vendored in `sdk/v29_1`, so `bash build.sh v29_1 targetcopy`
  builds from this repository alone.

Known limits: CPU drawing done after the last GPU drawing of a locked section is
overwritten by the frame copy (FreeSpace does not do that). The seed uploads up
to a full frame of texture data per locked section, which costs time on slow
MiniGL texture paths.

## Build

```
bash build.sh v29_1 targetcopy      # -> build/v29_1-targetcopy/Warp3D.library + smoke
bash tests/run.sh                   # host tests (gcc with ASan/UBSan)
```

Toolchain: Amiga GCC 16 (`/opt/amiga-gcc16-rc14` by default, override with
`WARPMINIGL_GCC=<prefix>/bin/m68k-amigaos`). The other variants
(`v18`, `v23`, `v27`, `v27_2` and modes `bitmap|direct|readpixels|window|target`)
are documented in the sections below.

## Testing in WinUAE

A MiniGL 29 whose renderer goes through Warp3D (MiniGL Classic) cannot run
under this wrapper, since it would call itself. The UAE bed used for v0.2 renames the
lower layer instead: see [docs/uae/README.md](docs/uae/README.md).
Result with FreeSpace 1.06 (68k, OS 3.2.2, uaegfx 32-bit Workbench, Fake16):
pilot selection, main hall, briefing and the first training mission render
correctly (space, stars, HUD with alpha, target view), and the game exits
cleanly to Workbench. Enforcer hits: one in the game's own `psnet_init`
(`gethostbyname` NULL), the rest inside MiniGL Classic (reads of a NULL window
for bitmap contexts). None come from this wrapper.

## Double-height destinations (4.18 TARGET COPY)

The supplied `payback.log` identifies WarpMiniGL 4.17
and MiniGL 27.6. It contains 207 rejected target-region requests, with the same
bitmap pointer in the captured requests; success and rejection alternate. The
active-window pointer is unchanged across MiniGL context creation and all 256
captured trace events. No color/depth clears appear in that captured prefix.
4.17 did not log requested offsets or geometry, so the exact rejected argument
is inferred, not proven by that log.

4.18 fixes the missing double-height/Y-offset path in TARGET COPY. Creation now
honors W3D_CC_DOUBLEHEIGHT, allocating a private renderer at half the bitmap
height and retaining W3D_DOUBLEHEIGHT in the context state. SetDrawRegion keeps
that logical height stable while the destination Y offset changes; the bitmap
copy uses that offset. Validation rejects negative/out-of-bounds offsets before
changing the current target, and pending work is flushed into the old region.
Non-double-height contexts can also use an initial offset with a fitting region.
Other modes keep their existing behavior. This does not add dynamic context
resizing or change window activation.

The ASan/UBSan host test executes real context creation with a 320x480 bitmap,
a 320x240 double-height render area and initial Y=240, then alternates Y=0/240
without recreating the renderer. It verifies both halves, copy bounds and
unchanged state on invalid offset. The Amiga smoke now checks both halves of two
double-height bitmaps and preservation of the upper half after writing the lower.
A separate 4.18 TRACE build includes requested Y offset, logical size and state
in its bounded output. Fullscreen Payback and the activation issue still require
hardware retesting.

## Payback diagnostics (4.17 TARGET COPY TRACE)

The user reports unchanged fullscreen flicker with 4.16, plus a need to click
before the window becomes active. Buffer rotation alone is not a confirmed
explanation. Build with `WARPMINIGL_TRACE=1 bash build.sh v27_2 targetcopy`.
The separate `build/v27_2-targetcopy-trace` output retains 4.16 rendering and
adds a bounded 256-event trace to DOS Output(): MiniGL identity, active window
before/after creation, target changes, sync boundaries, and cumulative draw,
color-clear and depth-clear counts (hexadecimal). Run the game from Shell with
output redirected to capture it. This is diagnostic, not a confirmed fix, and
logging affects timing. The wrapper does not change activation in this variant.

## Rotating game buffers (4.16 TARGET COPY, MiniGL V27_2)

Build with `bash build.sh v27_2 targetcopy`. This is a separate compatibility
variant for the reported Payback fullscreen alternating-image/gray-screen issue.
4.15 TARGET rejects changes to the bitmap in W3D_SetDrawRegion, leaving rendering
bound to the first bitmap. Buffer rotation is a likely explanation for the
symptom; the game's call sequence has not been captured on hardware.

TARGET COPY creates one private RTG friend bitmap and one MiniGL context.
After submitting and finishing rendering, it uses BltBitMap followed by WaitBlit
to copy into the current Warp3D destination. SetDrawRegion first synchronizes
pending work into the old destination, then selects the next compatible bitmap.
Textures and depth state stay in the same renderer. The game owns screen flipping;
the private bitmap cannot accidentally become its visible screen buffer.
No MiniGL SDK change or GLReadPixels is required. Existing build modes retain
their behavior, and the original 4.15 release remains available.

This costs an extra full-frame bitmap copy and render bitmap allocation.
Same-size four-byte-per-pixel targets with zero Y offset are supported.
It is intended for games rendering complete frames through Warp3D: CPU drawing
into game buffers is not imported into the private render bitmap and may be
overwritten by synchronization. A native MiniGL retarget API would avoid this
copy and limitation.

Host tests cover alternating destinations, flushing the old target before a
switch, persistent context, failed submission, format/size rejection, allocation
failure, cleanup ordering and recreation, including ASan/UBSan. The Amiga smoke
also renders and samples four alternating destinations using one context before
checking the visible window. Actual Payback behavior and performance need a
hardware test. This variant reports `Warp3D.library 4.16 ... TARGET COPY V27_2`.

## Game-owned bitmap (4.15 TARGET, MiniGL V27_2)

Build with `bash build.sh v27_2 target`. This uses the supplied
`include_v27_2/include` SDK without modifying its headers. The new
MGLCreateContextFromBitMap dispatch entry receives exactly the bitmap supplied
through W3D_CC_BITMAP; the wrapper explicitly sets currentContext on success.
No window lookup, helper window, GLReadPixels, or CPU pixel upload is used in
this mode. MGLSwitchDisplay followed by GLFinish completes the frame; per the
new SDK contract, bitmap contexts do not present to a window. The game retains
its original bitmap-to-window blit. Perspective/color handling is unchanged.

This first API accepts a whole, CGX-lockable bitmap with FOUR BYTES PER PIXEL.
The wrapper checks P96BMA_BYTESPERPIXEL, not BMA_DEPTH (CGX may report depth 24
for four-byte storage), and rejects incompatible targets. The backend validates
whether it can use the bitmap. No format conversion is promised by this SDK.
Test on a suitable 32-bit RTG target; a game's requested 16-bit mode alone does
not establish the actual format of its windowed friend bitmap.

V27_2 has no retarget/subrectangle entry. A repeated W3D_SetDrawRegion for the
same bitmap, zero offset and unchanged size is accepted. A different bitmap,
nonzero yoffset or changed dimensions is rejected without changing the live
target, textures or scissor. Games using fullscreen buffer rotation still need
a retarget API. Destroying and recreating a context for a new resolution is
supported; silently recreating a live context would lose its textures.

The TARGET smoke checks pixels in its supplied offscreen bitmap, performs the
game's final blit, then checks visible pixels. Expect two PASS messages and a
colored triangle on a dark background. Context deletion precedes freeing the
bitmap and closing the test window. Host tests cover ABI guards, rejected
formats/offsets, exact target selection, finish errors, retarget rejection and
context recreation. Actual PiStorm rendering/performance still require testing.

## Pre-IDCMP game startup (4.14 WINDOW)

The 4.13 game test reported no matching window, followed by "No suitable driver
found" and an illegal-instruction alert on the game's failure path. Inspection
of the available Hexen Amiga source shows ModifyIDCMP only after mglCreateContext
returns (gl_vidamiga.c around lines 499-503). Classic MiniGL vid_OpenWindow opens
and activates the window without IDCMP, allocates its bitmap, and then calls
W3D_CreateContext. Consequently UserPort is NULL during our window selection;
requiring mp_SigTask ownership excluded this legitimate startup sequence.

4.14 still prefers one matching same-task IDCMP window. If none exists, it also
accepts the active window with NULL UserPort and zero IDCMPFlags, but only when
it is the sole matching-size window in all Intuition screen lists. A foreign
port, inactive portless window, or ambiguous match is rejected. Portless ownership
cannot be proven by this heuristic; it is restricted to Classic's observed
activation-before-context startup pattern. Selection does not install or change
IDCMP. Logs identify this selection path; failures include requested dimensions,
active-window inner dimensions, and candidate counts to distinguish other causes.

The WINDOW smoke now reproduces Hexen's sequence: OpenWindow without IDCMP,
create the renderer, then ModifyIDCMP. Host tests cover that ordering, subsequent
port installation and context deletion, along with ambiguity, wrong task, inactive
windows and inconsistent port/IDCMP state. Sanitizers and the V27 target build
pass. This repairs a source-confirmed matching bug; the installed game/backend
still require testing, and the later crash is not independently diagnosed.
After the reported crash restart before testing to avoid stale runtime state.

## Existing game window (4.13 WINDOW, MiniGL V27)

The user supplied V27 headers with the published MGLCreateContextFromWindow
entry on 20 September. `bash build.sh v27 window` builds a separate
`build/v27-window/Warp3D.library` and matching smoke. This mode keeps the
hardware-confirmed 4.10 perspective/color behavior and DIRECT presentation,
but calls the new constructor with the game's existing Window. No helper window,
GLReadPixels, or per-frame CPU copy into the Warp3D bitmap is used by the wrapper.
MiniGL's documented internal render bitmap and ClipBlit presentation still apply;
this is not a claim of zero memory transfers inside the driver.

Warp3D supplies a bitmap and dimensions, not a Window pointer. This experimental
mode selects a window only when exactly one live window has both the requested
inner dimensions and an IDCMP UserPort signaled to the calling task. Selection
traverses Intuition's lists under LockIBase. It does not use an unrelated active
Shell/Workbench window, change IDCMP, or activate/depth-arrange a window. No match
or multiple matches produce a logged context-creation failure. Games using a
separate input task or no matching IDCMP port need explicit integration rather
than a guessed pointer. The window must stay open until context deletion.

The V27 client checks ABI, full dispatch size, currentContext and the published
constructor pointer, and rejects stub/Classic backends. The input SDK is preserved
unchanged; a generated public-only header view avoids the SDK's missing private
backend dependencies without declaring or accessing GLcontext internals.

Install only this Warp3D.library alongside the independent MiniGL V27. Close the
game and use Avail FLUSH before replacing the old wrapper; restart after memory
corruption or if a stale library remains open. Run the included smoke first. It
checks that context creation adds no window, samples the triangle in its original
window (not the unused offscreen Warp3D bitmap), and checks that deleting the
context leaves the window count intact. Native execution is still required.

Then test Hexen in the same resolution/scene as DIRECT. The game still owns its
normal Warp3D bitmap and may overwrite the directly presented image by blitting
or swapping it. Flicker/blank frames would identify this integration limitation;
the new constructor is a window API, not a borrowed-bitmap API. Same-size bitmap
switches retain direct window presentation. Different-size in-place resize is
explicitly refused; game-driven context recreation can select its new window.
The old exit alert has not been declared fixed.

Host tests exercise actual V27 loader and backend lifecycle code under sanitizers:
short/incompatible tables, missing entries, ambiguous/foreign/wrong-size windows,
creation failures, no focus changes, deletion, recreation and unsupported resize.
They deliberately provide no create-new-window, CloseWindow or pixel-read APIs.
Cross-build succeeds; performance and real game buffering require a PiStorm run.
Known-working 4.10 DIRECT and the previous READPIXELS ZIPs are retained.

## Borrowed game target: V25 API audit

The user reports that both READPIXELS 4.11 and optimized 4.12 remain slow.
The available V25 dispatch SDK does not expose attaching an existing window or
bitmap. The removed V18 experimental window API cannot be called through V25's
incompatible appended slots. Unchanged Warp3D games supply a destination bitmap,
so bypassing that bitmap and drawing only to an inferred window would not reliably
preserve game buffer swaps. See [the API audit and proposed backend requirements](docs/MINIGL_BORROWED_TARGET.md).
This historical audit predates V27. Version 4.13 now tests the new borrowed-window
API; a borrowed-bitmap interface remains a separate requirement for full Warp3D
destination-bitmap semantics.

## Readback transfer overhead (4.12)

The user reports a large slowdown with 4.11 READPIXELS compared with DIRECT.
Version 4.12 removes two avoidable costs on the wrapper side: the readback buffer
is retained across frames (grown only when required), and the per-scanline
p96WritePixelArray loop becomes one rectangle call. RenderInfo.BytesPerRow is
a signed WORD in the Picasso96 SDK, documented as the distance to the neighboring
row up or down. A negative RGBA pitch and a pointer to the last GL row perform
vertical reversal without a second CPU image-flip pass. Width remains limited
to 8191 pixels, so the four-byte pitch fits a signed WORD.

A failed buffer growth preserves the previous allocation and reports NOMEMORY
without reading or displaying an incomplete frame. Context deletion and library
expunge release the scratch buffer through wm_destroy, including when no MiniGL
context is active. Errors and unchanged frames retain the existing behavior.
The perspective correction, RGBA alpha, and synchronization points are unchanged.

Tests use a signed 16-bit stride and verify row reversal, y-offset, odd heights,
reuse, growth, failed growth, repeated cleanup, and reallocation after cleanup
under address/undefined-behavior sanitizers. They verify one P96 rectangle call
per successful sync. The target cross-build passes, but performance and the
installed P96 driver's negative-pitch behavior need confirmation on PiStorm.

This still transfers an entire framebuffer from the GPU through CPU memory to
the game bitmap at synchronization points. No measured FPS gain or DIRECT-level
performance is claimed. The confirmed 4.10 DIRECT archive remains the fallback
for speed. The separate window/input and exit issues remain open.

## GLReadPixels presentation (4.11, 17 September 2026)

The user confirmed the perspective correction in 4.10 on PiStorm and now
reports that PiStorm MiniGL V25 implements GLReadPixels. The new explicit
`readpixels` build retains that geometry/color code and copies GL_RGBA /
GL_UNSIGNED_BYTE output into the game's Warp3D bitmap through Picasso96.
It always uses GLReadPixels, bypassing the old MiniGL-14 ID-based back-buffer
accessor. The two-seed probe rejects silently unimplemented reads; rendering
errors and failed reads are reported without displaying uninitialized output.

Build: `bash build.sh v23 readpixels`. Output is `build/v23-readpixels/`.
The V23 dispatch function prefix matches the available V25 SDK. The wrapper
uses those existing slots, not V25's appended calls. No installed V25 backend
has been run locally, and the local Classic V25 renderer is a different backend;
it is not bundled or installed. Continue using the independent PiStorm MiniGL.
The compiler default now points to the installed GCC16 RC10 toolchain; the
WARPMINIGL_GCC override still works.

Close the game, unload the old wrapper with `Avail FLUSH`, and replace only
Warp3D.library. Use a restart after a memory error or if the old copy remains
open. Run the included `smoke` with no arguments. It requires the READPIXELS
wrapper ID and verifies the destination bitmap rather than treating driver state
zero as success. Expect a colored triangle and the bitmap sample PASS message.
Then test the game. Confirm orientation, colors, perspective and input.

The helper MiniGL window is moved behind other windows, not removed. The game
receives the copied image in its own bitmap. Full-frame CPU transfer may cost
performance compared with DIRECT. This build does not claim to repair context
resize or the previous exit alert. The known working 4.10 DIRECT ZIP is retained.
Host readback tests run in both compatibility and READPIXELS modes; the latter
asserts that the legacy back-buffer accessor is never called.

## Build and files

```
bash build.sh v18
bash build.sh v23
bash tests/run.sh
```

For the experimental mode that displays the frame directly in MiniGL's window:

```
bash build.sh v23 direct
bash build.sh v18 direct
```

These produce separate `build/v23-direct/` and `build/v18-direct/` directories,
each containing `Warp3D.library` **4.10 DIRECT** and its matching `smoke`.
The normal bitmap-copy builds remain separate.

## Direct presentation without GLReadPixels

The DIRECT variant leaves MiniGL's rendering window visible and presents dirty
frames through MGLSwitchDisplay, followed by GLFinish. It does not call
GLReadPixels or MGLLockBack and does not copy pixels to the caller's Warp3D
bitmap. This is useful for testing a game whose scene and HUD are all rendered
through Warp3D while the available MiniGL backend lacks working readback.
PiStorm3D still uses its 32-bit render target; no RGB reduction is introduced.

Use the matching DIRECT smoke: it opens the installed Warp3D.library by default
and keeps the MiniGL window visible for ten seconds. Inspect the triangle there.
It explicitly skips bitmap pixel checks because the game's bitmap is not updated;
driver state zero means the API reported no error, not that a visual test passed.
The normal smoke refuses a DIRECT driver to avoid testing the wrong output.

This mode changes presentation semantics: the original game window/screen and
the MiniGL window are separate. CPU/2D writes into the original bitmap, bitmap
readback, screenshots and effects that require those pixels are not provided.
Original window input focus is restored, but game input, fullscreen behavior,
window refresh and any game-specific presentation assumptions require a runtime
test. There is no automatic fallback that would silently change presentation mode.
The user confirmed that DIRECT 4.6 displays the game on PiStorm, with distorted
perspective. Version 4.7 then caused missing level geometry while trees and
other objects remained visible. Version 4.8 rolls that vertex-submission change
back; the original texture-perspective issue and window/input issue remain open.

## Perspective with constant-color polygons (4.10)

Warp3D disabling W3D_GOURAUD previously selected GL_FLAT in MiniGL. In the
available PiStorm3D draw.c, flat-textured paths ignore projective Q, while the
opaque smooth-textured and smooth alpha-test paths can use it. This is a
concrete source-level reason for affine texture distortion with screen-space
Warp3D input; the exact installed backend and game state still require a test.

Version 4.10 keeps GL_SMOOTH when Warp3D Gouraud is disabled. The Warp3D state
bit stays disabled and both immediate and array paths continue to ignore
per-vertex colors, so every vertex carries the same constant color. The wrapper
now saves W3D_SetCurrentColor separately and restores it after Gouraud draws,
including its alpha component. New contexts reset it to white. This does not
rescale positions: X/Y/Z and vertex W=1 remain as in 4.8, with perspective Q
still passed through GLTexCoord4f. The public API is unchanged, and there are
no private MiniGL context accesses or backend patches.

The state/color regression test executes the actual W3D_SetState,
W3D_SetCurrentColor, drawVertex and array setColor code. It checks Gouraud/flat
transitions, constant alpha, unchanged positions and a reference model of the
inspected MiniGL opaque single-texture interpolation path. It does not emulate
the GPU. Some blended shader paths in that source still ignore Q; changing the
shade model alone cannot repair every backend path. Verify walls/floors, models,
cutout sprites and lighting in the game. The original exit diagnostics remain
in this build, but exit/resize/input fixes are not claimed.

## Exit diagnostics (4.9)

The user reported that both windows close, followed by Exec's recoverable
`Memory header not located` alert, with the previous build. Closing both windows
does not identify the failing allocation or prove that context cleanup finished.
Version 4.9 retains the 4.8 geometry rollback and adds paired `[WarpMiniGL exit]`
messages around bulk texture cleanup, MGLDeleteContext, clearing the current
context, freeing the wrapper context, and library close/expunge. These messages
locate the last completed wrapper operation; they do not establish the cause of
memory corruption, which may have occurred earlier or inside MiniGL/the game.

Run from an Amiga Shell with output redirected, keeping the usual game arguments,
for example `ghexen2 >System:warp-exit.log`. After exiting, retain the log and any
alert. `MGLDeleteContext: begin` without `returned` places the failure inside that
call; `DestroyContext: done` confirms the wrapper reached the end of its context
cleanup. This diagnostic release does not claim to fix the reported exit error
or resizing/input behavior, and does not omit any frees to hide the failure.
It also ignores a NULL DestroyContext call and avoids DOS Output() when logging
from an Exec task that is not a DOS process. No installed library is modified.

## Geometry rollback (4.8)

Version 4.7 pre-scaled screen X/Y/Z by reciprocal clip W and supplied a non-unit
vertex W. Host projection tests passed, but they assumed a pipeline honoring W
for every primitive. The available PiStorm renderer only selects its real-W
shader path for a subset of render states. The user's result shows that this
change is not compatible with the actual game/backend combination.

Version 4.8 restores the 4.6 contract: unchanged raster X/Y/Z, vertex W=1, and
projective texture coordinates `(u*rhw, v*rhw, 0, rhw)`. The rollback covers both
immediate primitives and all array/indexed paths. No new perspective workaround
or window-lifecycle change is bundled into this recovery build. Host tests assert
unscaled raster positions independently of reciprocal depth, cover all vertex
and index formats, and verify normalized/strided UV handling. They do not claim
correct perspective on the GPU. The pre-rollback 4.7 sources are saved under
`backups/4.7-perspective-before-rollback/`, and the 4.6/4.7 ZIPs are retained.

Default compiler: `/opt/amiga-gcc16-rc10/bin/m68k-amigaos-gcc`, 68040,
hard-float, O2, no fast-math. `WARPMINIGL_GCC` overrides the compiler prefix.
Outputs: `build/v18/Warp3D.library`, `build/v23/Warp3D.library`, and `smoke`
in each build directory. All 84 vectors retain the original Warp3D FD order
and register ABI. The build links a MiniGL import stub, not the renderer.

Select the variant matching the installed MiniGL dispatch SDK. The local V18
and V23 tables both report ABI 3 but have incompatible appended entries.
No existing library has been replaced or installed by this project.

## Rendering integration

StormMesa supplies a destination bitmap through W3D_CC_BITMAP and switches it
using W3D_SetDrawRegion. The current common public MiniGL API creates its own
window/context and cannot attach an arbitrary Warp3D bitmap. This prototype
therefore creates a private MiniGL render window, moves it to the back, and
reads completed rendering into StormMesa's bitmap through Picasso96. Normally
this uses GLReadPixels(RGBA). For the identified PiStorm3D 14.0 backend, it
uses the public MGLLockBack buffer and converts native BGRA to RGBA instead.
Readback occurs at unlock, WaitIdle, Flush/FlushFrame and before a destination
switch. Unchanged frames skip readback. RGBA rows are flipped and the Warp3D
destination y-offset is applied. Failed MiniGL readback is reported through
W3D_GetDriverState and is not copied into the destination bitmap.

Warp3D's top-left pixel coordinates use an orthographic MiniGL projection;
depth 0..1 remains 0..1 after projection. Version 4.8 restores projective texture
coordinates with unscaled raster vertex positions. Vertex data remains in Amiga byte order.
Texture source formats (including ARGB, RGB565, ARGB4444, alpha/luminance and
palettes) are converted to byte RGBA before calling MiniGL.

The original global graphics-function patches, host-memory offsets and byte
swaps have been removed. MiniGL owns its renderer and its presentation policy;
StormMesa continues to own the game's window and presentation.

## Backend requirements and limitations

PiStorm3D 14.0 runtime investigation (11 September 2026): the smoke log
confirmed WarpMiniGL loaded and reported driver state zero, but its bitmap
contained garbage. Disassembly of the local matching PiStorm3D 14.0 binary
(`../PiStorm3D_V23_Classic/bin/minigl.library`) confirms that GLReadPixels
accepts only GL_RGB/GL_UNSIGNED_BYTE and silently returns for RGBA. Its
GLFinish only calls MGLFlushPendingRender, which waits for submitted work;
the active frame is submitted separately at MGLSwitchDisplay. The available
direct-backend source reads the visible window rather than the private render
bitmap. Changing only RGBA to RGB would therefore not repair this integration.
The earlier Classic 23.1 GLReadPixels fix does not change this PiStorm3D binary.

WarpMiniGL 4.4 probes RGBA readback with two different initial pixel values
once per context, rejects a silent no-op with W3D_UNSUPPORTED, preserves
pending GL errors, and leaves the destination untouched when validation fails.
This prevents copying uninitialized memory; it does **not** implement the
missing backend readback or make PiStorm3D 14.0 render successfully here.
WarpMiniGL 4.5 adds a compatibility path for the inspected PiStorm3D 14.0,
revision 0, dated 26.8.2026. It calls MGLSwitchDisplay to submit the current
frame, GLFinish to wait, and MGLLockBack to obtain the native drawing buffer.
That backend reports a 32-bit buffer with pixel_format 0; its renderer stores
BGRA bytes (RGBA8 output with red/blue swap). The wrapper validates the returned
pointer, dimensions, depth, format and row pitch, converts BGRA to RGBA while
preserving **all four channels**, then calls MGLUnlockDisplay. It reads the
private render buffer rather than pixels from the obscured helper window.
The library prints `PiStorm3D 14.0: 32-bit BGRA back buffer -> RGBA` when selected.

This uses the existing public dispatch API and MGLLockInfo only, with no private
context layout, binary patches or PiStorm register access. The native format
interpretation is limited to the identified backend; other versions retain the
validated GLReadPixels path. RGB-only conversion with alpha 255 is therefore
unnecessary for this compatibility path. Neither minigl.library nor system Libs
has been modified. Runtime verification on PiStorm is still required.

- The installed MiniGL must render independently of this wrapper. A call cycle
  back into W3D_CreateContext is rejected with W3D_NODRIVER. Merely providing
  the name minigl.library does not make a Classic Warp3D-based backend independent.
- Requires working MiniGL GLReadPixels or the identified 14.0 MGLLockBack path,
  a render window large enough for the
  destination bitmap, and Picasso96API.library. The helper window and CPU
  framebuffer copy are a compatibility approach, not a performance solution.
- Only one active Warp3D/MiniGL context. An already-current MiniGL context is
  not replaced. Resize succeeds only if MiniGL actually resizes the drawable;
  a no-op MGLResizeContext implementation produces W3D_UNSUPPORTED.
- Fog, stencil, logic operations, chroma tests, antialiasing, direct CPU depth
  reads/writes and several extended effects are unsupported. Capability queries
  are conservative rather than claiming complete Warp3D support.
- This is a first compatibility implementation. Rendering mixed with arbitrary
  CPU writes/2D drawing into the same bitmap between 3D batches needs further
  work: the private MiniGL framebuffer is not seeded from external bitmap edits.
- No claim is made that every StormMesa game works yet. No game or hardware
  runtime test has been completed on either PiStorm3D or WinUAE.

## Tests and first emulator/hardware run

Host tests compile actual pixel conversion, QuarkTex-derived vertex emission,
and readback code against mocks. They cover format conversion and input
preservation, MiniGL dispatch arguments, depth/projective UV, RGBA row order,
destination y-offset, dirty-frame handling, failed/silent readback, a legitimate
pixel matching the first probe seed, and preservation of rendering errors. The
legacy path tests backend identification, submit/wait/lock ordering, padded row
pitch, BGRA/RGBA conversion, alpha preservation, invalid lock metadata and failure
cleanup with address/undefined-behavior sanitizers. Both target
library variants and the smoke programs are cross-built.

The normal bitmap-copy smoke program opens a Warp3D library, creates a 320x240 destination bitmap,
draws a colored triangle through WarpMiniGL, then displays that bitmap for ten
seconds (or until closed), repainting window refresh events. Diagnostic version 3
opens the default system `Warp3D.library` when run as `smoke` without arguments.
An optional first argument selects an explicit library path. It prints the loaded
Warp3D and MiniGL library IDs and refuses a Warp3D driver whose ID does not contain
WarpMiniGL. The test also samples the destination bitmap's background and triangle
colors: driver state zero alone is not treated as proof of rendering.
Run `smoke` to test the installed wrapper, or supply an explicit test library path:

Use a matching, independent MiniGL backend. Verify that the window shows a
colored triangle on a dark background, that driver state is zero, and that the
bitmap sample check reports PASS. If the library ID check fails, stop and verify
the file and any already-loaded Warp3D driver before trying again. A context
failure with a Classic backend may be the intentional recursion guard.

## Provenance

Warp3D sources, FD, GCC resident lifecycle and allocation runtime derive from
the local QuarkTex project (Robert Konrad); its LGPL license is retained as
License.txt. Renderer-specific context and texture handling have been replaced.
`sdk/v18` and `sdk/v23` contain the matching locally available MiniGL public
headers/import clients from PiStorm3D_V18_Classic-fix and PiStorm3D_V23_Classic.
Their existing copyright notices are retained.
