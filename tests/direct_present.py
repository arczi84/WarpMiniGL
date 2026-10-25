from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
backend = (root / 'gcc/backend.c').read_text()
present = '#ifdef WM_DIRECT_PRESENT\n' + backend[backend.index('int wm_present_direct(void)'):]
hardware = (root / 'Warp3D.library/Hardware.c').read_text()
state = hardware[hardware.index('static ULONG wm_copy_error;'):hardware.index('#include <proto/graphics.h>')]
start = hardware.index('void wm_sync_bitmap(W3D_Context *context) {')
sync = hardware[start:hardware.index('\nvoid W3D_WaitIdle', start)]
header = r'''
#include <assert.h>
#include <stdio.h>
#define WM_DIRECT_PRESENT 1
#define GL_NO_ERROR 0
#define W3D_SUCCESS 0
#define W3D_UNSUPPORTED 28
typedef unsigned long ULONG;
typedef struct {int unused;} W3D_Context;
static void *backend=(void *)1;
static int wm_dirty,order,submitted,finished,fail_before,fail_after;
static unsigned error(void *c){
 assert(c==backend);
 if(order==0){order=1;return fail_before;}
 assert(order==3);order=4;return fail_after;
}
static void present(void *c){assert(c==backend&&order==1);order=2;++submitted;}
static void finish(void *c){assert(c==backend&&order==2);order=3;++finished;}
/* Deliberately no GLReadPixels, MGLLockBack or bitmap/pixel allocation API. */
static struct {
 unsigned (*GLGetError)(void *);void (*MGLSwitchDisplay)(void *);void (*GLFinish)(void *);
} dispatch={error,present,finish},*MiniGLDispatch=&dispatch;
'''
main = r'''
int main(void){
 wm_reset_readback();wm_sync_bitmap(NULL);assert(!submitted&&!finished&&!order);
 wm_dirty=1;wm_sync_bitmap(NULL);
 assert(submitted==1&&finished==1&&order==4&&!wm_dirty&&wm_copy_error==0);
 order=0;wm_sync_bitmap(NULL);assert(!order&&submitted==1);
 fail_before=1;wm_dirty=1;wm_sync_bitmap(NULL);
 assert(submitted==1&&wm_dirty&&wm_copy_error==W3D_UNSUPPORTED);
 order=0;fail_before=0;fail_after=1;wm_sync_bitmap(NULL);
 assert(submitted==2&&finished==2&&wm_dirty&&wm_copy_error==W3D_UNSUPPORTED);
 order=0;fail_after=0;wm_sync_bitmap(NULL);
 assert(submitted==3&&!wm_dirty&&wm_copy_error==0);
 backend=NULL;order=0;wm_dirty=1;wm_sync_bitmap(NULL);
 assert(!order&&submitted==3&&wm_dirty&&wm_copy_error==W3D_UNSUPPORTED);
 puts("PASS: DIRECT submits dirty frames without readback, skips clean frames and propagates errors");
 return 0;
}
'''
with tempfile.TemporaryDirectory() as directory:
    path = Path(directory)
    (path / 'test.c').write_text(header + present + state + sync + main)
    subprocess.run(['cc', '-std=c99', '-Wall', '-Wextra', '-Werror',
                    str(path / 'test.c'), '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
