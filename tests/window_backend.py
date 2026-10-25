from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'gcc/backend.c').read_text()
selection = source[source.index('#ifdef WM_BORROWED_WINDOW'):source.index('static int legacy_direct_readback;')]
lifecycle = source[source.index('static void setup('):source.index('void wm_exit(void)')]
loader = '\n'.join(line for line in (root / 'sdk/v27/client/minigl_open.c').read_text().splitlines()
                   if not line.startswith('#include') and not line.startswith('extern '))
header = r'''
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#define WM_BORROWED_WINDOW 1
#define WM_DIRECT_PRESENT 1
#define TRUE 1
#define FALSE 0
#define GL_TRUE 1
#define GL_PROJECTION 1
#define GL_MODELVIEW 2
#define GL_PACK_ALIGNMENT 3
#define GL_UNPACK_ALIGNMENT 4
#define MINIGLNAME "minigl.library"
#define MINIGL_VERSION 14
#define MINIGL_DISPATCH_ABI_VERSION 3
#define MINIGL_BACKEND_FLAG_STUB 1
#define MINIGL_BACKEND_FLAG_CLASSIC 2
typedef unsigned long ULONG;
typedef int BOOL;
typedef void *GLcontext;
struct Task {int id;};
struct MsgPort {struct Task *mp_SigTask;};
struct RastPort {int unused;};
struct Screen;
struct Window {
 struct Window *NextWindow;struct MsgPort *UserPort;struct RastPort *RPort;
 struct Screen *WScreen;
 int Width,Height,BorderLeft,BorderRight,BorderTop,BorderBottom;
 unsigned IDCMPFlags;
};
struct Screen {struct Screen *NextScreen;struct Window *FirstWindow;};
static struct {struct Screen *FirstScreen;struct Window *ActiveWindow;} intuition,*IntuitionBase=&intuition;
static struct Task task={1},foreign={2};
static int locked,creates,deletes,setup_calls,releases,fail_create,wrong_window,closes,open_ok=1;
static struct Window *borrowed,*wrong;
static GLcontext backend,current;
static int wm_dirty;
/* Diagnostics and the Texture.c registry are not under test here. */
static inline void wm_trace(const char *n,unsigned long a,unsigned long b,unsigned long c){(void)n;(void)a;(void)b;(void)c;}
static struct Task *FindTask(void *name){assert(!name);return &task;}
static ULONG LockIBase(unsigned x){assert(!x&&!locked);locked=1;return 17;}
static void UnlockIBase(ULONG key){assert(locked&&key==17);locked=0;}
static void qt_debug(const char *text){(void)text;}
static void wm_release_readback(void){++releases;}
static void *create(struct Window *w){assert(!locked);++creates;borrowed=w;return fail_create?NULL:(void *)23;}
static void destroy(GLcontext c){assert(c==(void *)23);++deletes;}
static void *get_window(GLcontext c){assert(c==(void *)23);return wrong_window?wrong:borrowed;}
static void ortho(GLcontext c,double l,double r,double b,double t,double n,double f){
 assert(c==current&&l==0&&r==320&&b==240&&t==0&&n==0&&f==-1);
}
/* No create-new-window, resize, activation, CloseWindow or pixel-read APIs:
 * borrowed mode must not need or call any of them. */
typedef struct {
 ULONG abiVersion,structSize,backendFlags;
 GLcontext *currentContext;
 void (*GLOrtho)(GLcontext,double,double,double,double,double,double);
 void *(*MGLCreateContextFromWindow)(struct Window *);
 void (*MGLDeleteContext)(GLcontext);
 void *(*MGLGetWindowHandle)(GLcontext);
} MGLDispatchTable;
static MGLDispatchTable dispatch={3,sizeof(MGLDispatchTable),4,&current,ortho,create,destroy,get_window};
static const MGLDispatchTable *MiniGLDispatch;
static struct {int unused;} library,*MiniGLBase;
static void *OpenLibrary(const char *name,unsigned version){(void)name;assert(version==14);return open_ok?&library:NULL;}
static void CloseLibrary(void *lib){assert(lib==&library);++closes;}
static const MGLDispatchTable *MiniGLGetDispatchTableLVO(void){return &dispatch;}
#define MC (*MiniGLDispatch->currentContext)
#define _glViewport(x,y,w,h) do {assert(x==0&&y==0&&w==320&&h==240);++setup_calls;}while(0)
#define _glMatrixMode(mode) ((void)(mode))
#define _glLoadIdentity() ((void)0)
#define _glPixelStorei(p,v) do {assert((p==GL_PACK_ALIGNMENT||p==GL_UNPACK_ALIGNMENT)&&v==1);}while(0)
'''
main = r'''
int main(void){
 struct MsgPort port={&task},otherport={&foreign};struct RastPort rp={0};
 struct Screen s={0},other_screen={0};
 struct Window a={NULL,&port,&rp,&s,328,264,4,4,20,4,0};
 struct Window b=a,outsider=a;
 wrong=&outsider;outsider.UserPort=&otherport;
 s.FirstWindow=&a;s.NextScreen=&other_screen;other_screen.FirstWindow=&outsider;outsider.WScreen=&other_screen;
 intuition.FirstScreen=&s;intuition.ActiveWindow=&outsider;
 /* Size/ABI guards must run before attempting any borrowed constructor. */
 open_ok=0;assert(!MiniGLOpen()&&!MiniGLBase&&!MiniGLDispatch);open_ok=1;
 dispatch.structSize=sizeof(dispatch)-sizeof(void *);
 assert(!MiniGLOpen()&&!MiniGLBase&&!MiniGLDispatch&&closes==1);
 dispatch.structSize=sizeof(dispatch);dispatch.abiVersion=2;
 assert(!MiniGLOpen());dispatch.abiVersion=3;
 dispatch.MGLCreateContextFromWindow=NULL;assert(!MiniGLOpen());dispatch.MGLCreateContextFromWindow=create;
 dispatch.backendFlags=MINIGL_BACKEND_FLAG_CLASSIC;assert(!MiniGLOpen());
 dispatch.backendFlags=MINIGL_BACKEND_FLAG_STUB;assert(!MiniGLOpen());dispatch.backendFlags=4;
 dispatch.currentContext=NULL;assert(!MiniGLOpen());dispatch.currentContext=&current;
 assert(MiniGLOpen()&&MiniGLDispatch==&dispatch);
 assert(wm_find_window(320,240)==&a&&!locked);
 a.NextWindow=&b;assert(!wm_find_window(320,240));
 assert(!wm_create(320,240)&&!creates);a.NextWindow=NULL;
 a.UserPort=NULL;assert(!wm_create(320,240)&&!creates);a.UserPort=&port;
 a.Width=327;assert(!wm_create(320,240)&&!creates);a.Width=328;
 fail_create=1;assert(!wm_create(320,240)&&!backend&&!current);fail_create=0;
 wrong_window=1;assert(!wm_create(320,240)&&!backend&&!current&&deletes==1);wrong_window=0;
 current=(void *)99;assert(!wm_create(320,240));current=NULL;
 assert(wm_create(320,240)&&backend==current&&borrowed==&a&&setup_calls==1);
 assert(intuition.ActiveWindow==&outsider&&s.FirstWindow==&a&&a.UserPort==&port);
 assert(!wm_create(320,240));
 assert(!wm_resize(800,600)&&setup_calls==1&&current==backend);
 assert(wm_resize(320,240)&&setup_calls==2);
 wm_destroy();assert(!backend&&!current&&deletes==2&&s.FirstWindow==&a&&a.UserPort==&port);
 wm_destroy();assert(deletes==2);
 assert(wm_create(320,240));wm_destroy();assert(deletes==3);
 /* Hexen/Classic startup: OpenWindow without IDCMP, then W3D context,
  * and only afterwards ModifyIDCMP. Match without changing the game's port. */
 a.UserPort=NULL;intuition.ActiveWindow=&a;
 assert(!wm_find_window(320,240)); /* A second matching window is ambiguous. */
 outsider.Width=640;
 a.IDCMPFlags=1;assert(!wm_find_window(320,240));a.IDCMPFlags=0;
 intuition.ActiveWindow=&outsider;assert(!wm_find_window(320,240));
 intuition.ActiveWindow=&a;assert(wm_find_window(320,240)==&a);
 a.UserPort=&otherport;assert(!wm_find_window(320,240));a.UserPort=NULL;
 a.NextWindow=&b;b.UserPort=NULL;assert(!wm_find_window(320,240));a.NextWindow=NULL;
 assert(wm_create(320,240)&&borrowed==&a&&!a.UserPort&&!a.IDCMPFlags);
 /* Simulate the game's later ModifyIDCMP, then ensure cleanup leaves it. */
 a.UserPort=&port;a.IDCMPFlags=1;
 wm_destroy();assert(deletes==4&&a.UserPort==&port&&a.IDCMPFlags==1);
 MiniGLClose();assert(!MiniGLBase&&!MiniGLDispatch);
 puts("PASS: V27 ABI/slot guards, task-owned and pre-IDCMP startup selection, no helper/readback/focus changes, failure cleanup, borrowed lifecycle and resize refusal");
}
'''
with tempfile.TemporaryDirectory() as directory:
    path = Path(directory)
    (path / 'test.c').write_text('\n'.join([header, loader, selection, lifecycle, main]))
    subprocess.run(['cc', '-std=gnu99', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', '-fno-pie', '-no-pie',
                    str(path / 'test.c'), '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
