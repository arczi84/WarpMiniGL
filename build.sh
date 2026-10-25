#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "$0")"
variant=${1:-v18}
case "$variant" in v18|v23|v27|v27_2|v29_1) ;; *) echo 'Usage: bash build.sh v18|v23|v27|v27_2|v29_1' >&2;exit 1;; esac
mode=${2:-bitmap}
case "$mode" in bitmap|direct|readpixels|window|target|targetcopy) ;; *) echo 'Usage: bash build.sh v18|v23|v27|v27_2 [bitmap|direct|readpixels|window|target|targetcopy]' >&2;exit 1;; esac
if [[ ( "$mode" == target || "$mode" == targetcopy ) && "$variant" != v27_2 && "$variant" != v29_1 ]];then echo 'TARGET requires V27_2 or V29.1 SDK' >&2;exit 1;fi
if [[ "$mode" == window && "$variant" != v27 && "$variant" != v29_1 ]];then echo 'WINDOW requires V27 or V29.1 SDK' >&2;exit 1;fi
prefix=${WARPMINIGL_GCC:-/opt/amiga-gcc16-rc10/bin/m68k-amigaos}
sdkroot="sdk/$variant"
if [[ "$variant" == v29_1 ]];then
 prefix=${WARPMINIGL_GCC:-/opt/amiga-gcc16-rc14/bin/m68k-amigaos}
 sdkroot=${WARPMINIGL_SDK:-sdk/v29_1/libraryroot}
fi
out="build/$variant"
modeflags=()
if [[ "$mode" == direct ]];then out="${out}-direct";modeflags=(-DWM_DIRECT_PRESENT);fi
if [[ "$mode" == readpixels ]];then out="${out}-readpixels";modeflags=(-DWM_READPIXELS_ONLY);fi
if [[ "$mode" == window ]];then out="${out}-window";modeflags=(-DWM_DIRECT_PRESENT -DWM_BORROWED_WINDOW);fi
if [[ "$mode" == target ]];then out="${out}-target";modeflags=(-DWM_DIRECT_PRESENT -DWM_BORROWED_BITMAP);fi
if [[ "$mode" == targetcopy ]];then out="${out}-targetcopy";modeflags=(-DWM_DIRECT_PRESENT -DWM_BORROWED_BITMAP -DWM_TARGET_COPY);fi
if [[ "${WARPMINIGL_TRACE:-0}" == 1 ]];then
 if [[ "$mode" != targetcopy ]];then echo 'TRACE requires targetcopy' >&2;exit 1;fi
 out="${out}-trace";modeflags+=(-DWM_FRAME_TRACE)
fi
mkdir -p "$out"
sdkflags=(-I"$sdkroot/include")
if [[ "$variant" == v27 || "$variant" == v27_2 ]];then
 python3 gcc/generate_v27_client.py "$out/client" "$variant"
 sdkflags=(-I"$out/client" "${sdkflags[@]}")
fi
perl gcc/generate.pl vectors > "$out/vectors.inc"
flags=(-std=gnu99 -fgnu89-inline -mcpu=68040 -mhard-float -mcrt=clib2 -O2 -fno-fast-math -fno-strict-aliasing -fno-builtin -fcommon "${sdkflags[@]}" -I. -Igcc -I"$out" -include gcc/compat.h -D__M68K__ -DMINIGL_LIBRARY_BUILD -DUSE_MGLAPI)
flags+=("${modeflags[@]}")
if [[ "$variant" == v29_1 ]];then flags+=(-DWM_V291);fi
for src in Warp3D.library/*.c gcc/library.c gcc/runtime.c gcc/backend.c gcc/frame_trace.c "$sdkroot/client/minigl_base.c" "$sdkroot/client/minigl_open.c";do
 "$prefix-gcc" "${flags[@]}" -c "$src" -o "$out/$(basename "${src%.c}").o"
done
"$prefix-gcc" -mcpu=68040 -c "$sdkroot/client/minigl_dispatch_stub_gcc.s" -o "$out/import.o"
"$prefix-gcc" -nostdlib -nostartfiles -mcpu=68040 -mhard-float -mcrt=clib2 -o "$out/Warp3D.library" "$out"/*.o -lgcc
"$prefix-gcc" -mcpu=68040 -mhard-float -mcrt=clib2 -O2 "${modeflags[@]}" examples/smoke.c examples/import.s -o "$out/smoke"
