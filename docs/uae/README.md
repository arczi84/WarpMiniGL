# WinUAE test bed for the V29.1 build

The wrapper needs a MiniGL 29 that renders without Warp3D (on the PiStorm:
the V3D MiniGL). In WinUAE the available MiniGL 29.1 is MiniGL Classic, which
draws through Warp3D.library, the very library this wrapper replaces. The test
bed therefore moves the lower Warp3D under another name:

```
FreeSpace -> Warp3D.library (WarpMiniGL 4.20)
          -> minigl.library (MiniGL Classic 29.1, patched to open "Warp3Q.library")
          -> Warp3Q.library (QuarkTex 0.53 test copy) -> host OpenGL (QuarkTex.alib)
```

Steps (on a test system only; nothing here belongs in a normal install):

1. MiniGL Classic 29.1: replace the three `Warp3D.library` strings by
   `Warp3Q.library` (same length):
   `LC_ALL=C sed 's/Warp3D\.library/Warp3Q.library/g' minigl.library > LIBS/minigl.library`
2. QuarkTex (Robert Konrad, LGPL; GCC tree): apply `quarktex-clear.patch` and
   build with `bash gcc/build.sh`; install `build-gcc/Warp3D.library` as
   `LIBS:Warp3Q.library`. The patch renames the library and fixes
   `W3D_ClearDrawRegion` in windowed mode, which drew a rectangle with the
   current texture/blend state. Without the fix the space background is never
   cleared and alpha HUD textures turn into solid blocks.
3. Install this wrapper as `LIBS:Warp3D.library`.
4. `startup` and `poller` are the AmigaDOS scripts used for automated runs:
   the poller executes `OGLTEST:cmd` when it appears, which lets the host
   inject keys and clicks with `uaeinput` (`uaeinput.c`, input.device events),
   without taking the host's keyboard focus.

QuarkTex needs WinUAE's `QuarkTex.alib` (in the WinUAE `plugins/alib` folder).
