from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'gcc/backend.c').read_text()
init = source[source.index('static int wm_id_contains('):source.index('int wm_ready(void)')]
read = '#ifndef WM_DIRECT_PRESENT\n' + source[source.index('int wm_read_legacy_rgba('):]
header = r'''
#include <assert.h>
#include <string.h>
#include <stdio.h>
typedef unsigned char UBYTE;
typedef unsigned long ULONG;
typedef void *GLcontext;
typedef struct {ULONG width,height,depth,pixel_format;void *base_address;ULONG pitch;} MGLLockInfo;
#define GL_NO_ERROR 0
static int legacy_direct_readback,open_ok=1;
static GLcontext backend=(void *)1;
static struct {unsigned lib_Version,lib_Revision;const char *lib_IdString;} lib,*MiniGLBase=&lib;
static int MiniGLOpen(void){return open_ok;}
void qt_debug(const char *s){(void)s;}
static int order,locks,unlocks,gpu_error,lock_ok=1;
static UBYTE native[24]={3,2,1,4, 7,6,5,8, 99,99,99,99, 13,12,11,14, 17,16,15,18, 99,99,99,99};
static MGLLockInfo supplied={2,2,32,0,native,12};
static void present(GLcontext c){assert(c==backend&&order==0);order=1;}
static void finish(GLcontext c){assert(c==backend&&order==1);order=2;}
static unsigned error(GLcontext c){assert(c==backend&&order==2);order=3;return gpu_error;}
static int lock(GLcontext c,MGLLockInfo *i){assert(c==backend&&order==3);order=4;++locks;*i=supplied;return lock_ok;}
static void unlock(GLcontext c){assert(c==backend&&order==4);order=5;++unlocks;}
static struct {
 void (*MGLSwitchDisplay)(GLcontext);void (*GLFinish)(GLcontext);
 unsigned (*GLGetError)(GLcontext);int (*MGLLockBack)(GLcontext,MGLLockInfo *);
 void (*MGLUnlockDisplay)(GLcontext);
} dispatch={present,finish,error,lock,unlock},*MiniGLDispatch=&dispatch;
'''
main = r'''
int main(void){
 UBYTE pixels[16],original[16];
 const UBYTE expected[16]={11,12,13,14,15,16,17,18, 1,2,3,4,5,6,7,8};
 lib.lib_Version=14;lib.lib_Revision=0;
 lib.lib_IdString="$VER: minigl.library 14.0 (26.8.2026) PiStorm3D, single-context direct";
 assert(wm_init()&&legacy_direct_readback);
 memset(pixels,0xaa,sizeof(pixels));
 assert(wm_read_legacy_rgba(pixels,2,2)==1&&order==5&&locks==1&&unlocks==1);
 assert(!memcmp(pixels,expected,sizeof(pixels)));
 memcpy(original,pixels,sizeof(pixels));
 /* Failed lock, GPU failure, invalid pitch/depth/format/size/address:
  * no destination changes and each acquired lock is released. */
 order=0;lock_ok=0;assert(wm_read_legacy_rgba(pixels,2,2)==-1&&unlocks==1);lock_ok=1;
 order=0;gpu_error=1;assert(wm_read_legacy_rgba(pixels,2,2)==-1&&locks==2);gpu_error=0;
 order=0;supplied.pitch=7;assert(wm_read_legacy_rgba(pixels,2,2)==-1&&unlocks==2);supplied.pitch=12;
 order=0;supplied.depth=16;assert(wm_read_legacy_rgba(pixels,2,2)==-1&&unlocks==3);supplied.depth=32;
 order=0;supplied.pixel_format=9;assert(wm_read_legacy_rgba(pixels,2,2)==-1&&unlocks==4);supplied.pixel_format=0;
 order=0;supplied.width=1;assert(wm_read_legacy_rgba(pixels,2,2)==-1&&unlocks==5);supplied.width=2;
 order=0;supplied.base_address=NULL;assert(wm_read_legacy_rgba(pixels,2,2)==-1&&unlocks==6);supplied.base_address=native;
 order=0;supplied.pitch=0x7fffffffUL;assert(wm_read_legacy_rgba(pixels,2,2)==-1&&unlocks==7);
 assert(!memcmp(pixels,original,sizeof(pixels)));
 /* Do not reinterpret another driver's unspecified native pixel format. */
 lib.lib_IdString="(26.8.2026) classic MiniGL,";assert(wm_init()&&!legacy_direct_readback);
 order=0;assert(wm_read_legacy_rgba(pixels,2,2)==0&&order==0);
 lib.lib_IdString="(26.8.2026) PiStorm3D,";lib.lib_Version=18;
 assert(wm_init()&&!legacy_direct_readback);
 lib.lib_Version=14;lib.lib_Revision=1;assert(wm_init()&&!legacy_direct_readback);
 lib.lib_Revision=0;lib.lib_IdString="(1.9.2026) PiStorm3D,";assert(wm_init()&&!legacy_direct_readback);
 puts("PASS: legacy PiStorm detection, submit/wait/lock order, padded BGRA -> RGBA, alpha, row order and failures");
 return 0;
}
'''
with tempfile.TemporaryDirectory() as directory:
    path = Path(directory)
    (path / 'test.c').write_text(header + init + read + main)
    subprocess.run(['cc', '-std=c99', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', str(path / 'test.c'),
                    '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
