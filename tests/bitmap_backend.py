"""V27_2 borrowed bitmap contract: no window or CPU readback APIs available."""
from pathlib import Path
import re
import subprocess
import tempfile
import sys

root = Path(__file__).resolve().parent.parent
source = (root / 'gcc/backend.c').read_text()
lifecycle = source[source.index('static void setup('):source.index('void wm_exit(void)')]
present = '#ifdef WM_DIRECT_PRESENT\n' + source[source.index('int wm_present_direct(void)'):]
loader = '\n'.join(line for line in (root / 'sdk/v27_2/client/minigl_open.c').read_text().splitlines()
                   if not line.startswith('#include') and not line.startswith('extern '))
hardware = (root / 'Warp3D.library/Hardware.c').read_text()
state = hardware[hardware.index('static ULONG wm_copy_error;'):hardware.index('#include <proto/graphics.h>')]
start = hardware.index('void wm_sync_bitmap(W3D_Context *context) {')
sync = hardware[start:hardware.index('\nvoid W3D_WaitIdle', start)]
effect = (root / 'Warp3D.library/Effect.c').read_text()
region = effect[effect.index('extern int wm_resize'):effect.index('ULONG W3D_SetDrawRegionWBM')]
header = r'''
#include <assert.h>
#include <stdio.h>
#include <stddef.h>
#define WM_DIRECT_PRESENT 1
#define WM_BORROWED_BITMAP 1
#define TRUE 1
#define FALSE 0
#define GL_PROJECTION 1
#define GL_MODELVIEW 2
#define GL_PACK_ALIGNMENT 3
#define GL_UNPACK_ALIGNMENT 4
#define GL_NO_ERROR 0
#define MINIGLNAME "minigl.library"
#define MINIGL_VERSION 14
#define MINIGL_DISPATCH_ABI_VERSION 3
#define MINIGL_BACKEND_FLAG_STUB 1
#define MINIGL_BACKEND_FLAG_CLASSIC 2
#define P96BMA_BYTESPERPIXEL 1
#define BMA_WIDTH 1
#define BMA_HEIGHT 2
#define W3D_SUCCESS 0
#define W3D_UNSUPPORTED 28
#define W3D_ILLEGALINPUT 1
#define W3D_ILLEGALBITMAP 2
#define __REGA0(x) x
#define __REGA1(x) x
#define __REGA2(x) x
#define __REGD1(x) x
typedef unsigned long ULONG;
typedef int BOOL;
typedef void *GLcontext;
struct BitMap {int w,h,bpp,pixel;};
typedef struct {int left,top,width,height;} W3D_Scissor;
typedef struct {struct BitMap *drawregion;int yoffset,width,height;W3D_Scissor scissor;} W3D_Context;
static GLcontext backend,current;
static struct BitMap *borrowed_bitmap,*driver_bitmap;
static int target_width,target_height,width=320,height=240,wm_dirty;
static int creates,deletes,submitted,finished,fail_create,fail_finish,open_ok=1,closes,setup_calls;
static void qt_debug(const char *s){(void)s;}
static unsigned p96GetBitMapAttr(struct BitMap *b,unsigned attr){assert(attr==P96BMA_BYTESPERPIXEL);return b->bpp;}
static int GetBitMapAttr(struct BitMap *b,unsigned attr){return attr==BMA_WIDTH?b->w:b->h;}
static GLcontext create(struct BitMap *b){++creates;if(fail_create)return NULL;driver_bitmap=b;return (void *)23;}
static void destroy(GLcontext c){assert(c==current&&c==(void *)23);++deletes;driver_bitmap=NULL;}
static void ortho(GLcontext c,double l,double r,double b,double t,double n,double f){
 assert(c==current&&l==0&&r==target_width&&b==target_height&&t==0&&n==0&&f==-1);
}
static int error(GLcontext c){assert(c==current);return fail_finish&&finished?1:0;}
static void submit(GLcontext c){assert(c==current&&driver_bitmap);++submitted;}
static void finish(GLcontext c){assert(c==current&&driver_bitmap);++finished;if(!fail_finish)driver_bitmap->pixel=42;}
/* No Window, Intuition, GLReadPixels, CPU copy or bitmap-free API is declared. */
typedef struct {
 ULONG abiVersion,structSize,backendFlags;
 GLcontext *currentContext;
 void (*GLOrtho)(GLcontext,double,double,double,double,double,double);
 GLcontext (*MGLCreateContextFromBitMap)(struct BitMap *);
 void (*MGLDeleteContext)(GLcontext);
 void (*MGLSwitchDisplay)(GLcontext);
 void (*GLFinish)(GLcontext);
 int (*GLGetError)(GLcontext);
} MGLDispatchTable;
static MGLDispatchTable dispatch={3,sizeof(MGLDispatchTable),4,&current,ortho,create,destroy,submit,finish,error};
static const MGLDispatchTable *MiniGLDispatch;
static struct {int unused;} library,*MiniGLBase;
static void *OpenLibrary(const char *name,unsigned version){(void)name;assert(version==14);return open_ok?&library:NULL;}
static void CloseLibrary(void *lib){assert(lib==&library);++closes;}
static const MGLDispatchTable *MiniGLGetDispatchTableLVO(void){return &dispatch;}
#define MC (*MiniGLDispatch->currentContext)
#define _glViewport(x,y,w,h) do {assert(x==0&&y==0&&w==target_width&&h==target_height);++setup_calls;}while(0)
#define _glMatrixMode(m) ((void)(m))
#define _glLoadIdentity() ((void)0)
#define _glPixelStorei(p,v) do {assert((p==GL_PACK_ALIGNMENT||p==GL_UNPACK_ALIGNMENT)&&v==1);}while(0)
static void W3D_SetScissor(W3D_Context *c,W3D_Scissor *s){c->scissor=*s;}
'''
main = r'''
ULONG W3D_GetDriverState(W3D_Context *context){(void)context;return wm_copy_error;}
int main(void){
 struct BitMap a={320,240,4,0},b={320,240,4,0},narrow={320,240,2,0};
 W3D_Context c={&a,0,320,240,{0,0,320,240}};
 W3D_Scissor scissor={1,2,100,120};
 open_ok=0;assert(!MiniGLOpen());open_ok=1;
 dispatch.structSize=sizeof(dispatch)-sizeof(void *);assert(!MiniGLOpen()&&!MiniGLDispatch&&!MiniGLBase);
 dispatch.structSize=sizeof(dispatch);dispatch.abiVersion=2;assert(!MiniGLOpen());dispatch.abiVersion=3;
 dispatch.MGLCreateContextFromBitMap=NULL;assert(!MiniGLOpen());dispatch.MGLCreateContextFromBitMap=create;
 dispatch.MGLSwitchDisplay=NULL;assert(!MiniGLOpen());dispatch.MGLSwitchDisplay=submit;
 dispatch.GLFinish=NULL;assert(!MiniGLOpen());dispatch.GLFinish=finish;
 dispatch.MGLDeleteContext=NULL;assert(!MiniGLOpen());dispatch.MGLDeleteContext=destroy;
 dispatch.GLGetError=NULL;assert(!MiniGLOpen());dispatch.GLGetError=error;
 dispatch.currentContext=NULL;assert(!MiniGLOpen());dispatch.currentContext=&current;
 dispatch.backendFlags=1;assert(!MiniGLOpen());dispatch.backendFlags=2;assert(!MiniGLOpen());dispatch.backendFlags=4;
 assert(MiniGLOpen());
 assert(!wm_create_target(&narrow,0,320,240)&&!creates);
 assert(!wm_create_target(&a,1,320,239)&&!creates);
 current=(void *)99;assert(!wm_create_target(&a,0,320,240));current=NULL;
 fail_create=1;assert(!wm_create_target(&a,0,320,240)&&!current&&!borrowed_bitmap);fail_create=0;
 assert(wm_create_target(&a,0,320,240)&&backend==current&&driver_bitmap==&a&&borrowed_bitmap==&a);
 assert(!wm_create_target(&b,0,320,240));
 wm_reset_readback();wm_dirty=1;wm_sync_bitmap(&c);
 assert(a.pixel==42&&b.pixel==0&&!wm_dirty&&!wm_copy_error&&submitted==1&&finished==1);
 wm_sync_bitmap(&c);assert(submitted==1);
 assert(W3D_SetDrawRegion(&c,&a,0,&scissor)==W3D_SUCCESS&&c.scissor.left==1);
 scissor.left=7;wm_dirty=1;
 assert(W3D_SetDrawRegion(&c,&b,0,&scissor)==W3D_UNSUPPORTED);
 assert(c.drawregion==&a&&borrowed_bitmap==&a&&c.scissor.left==1&&wm_dirty&&submitted==1);
 assert(W3D_SetDrawRegion(&c,&a,1,&scissor)==W3D_UNSUPPORTED&&c.yoffset==0);
 a.w=640;assert(W3D_SetDrawRegion(&c,&a,0,&scissor)==W3D_UNSUPPORTED&&width==320);a.w=320;
 assert(!wm_resize(640,480)&&wm_resize(320,240)&&setup_calls==1);
 fail_finish=1;finished=0;
 assert(W3D_SetDrawRegion(&c,&a,0,&scissor)==W3D_UNSUPPORTED&&wm_dirty&&c.scissor.left==1);
 fail_finish=0;assert(W3D_SetDrawRegion(&c,&a,0,&scissor)==0&&!wm_dirty&&c.scissor.left==7);
 wm_destroy();assert(!backend&&!current&&!borrowed_bitmap&&a.pixel==42&&deletes==1);
 wm_destroy();assert(deletes==1);
 b.w=640;b.h=480;assert(wm_create_target(&b,0,640,480));wm_dirty=1;wm_sync_bitmap(&c);
 assert(b.pixel==42&&target_width==640);wm_destroy();assert(deletes==2);
 MiniGLClose();assert(!MiniGLBase&&!MiniGLDispatch);
 puts("PASS: bitmap target ABI guards, exact destination, no window/readback, format/offset rejection, finish errors, failed retarget preserves state, borrowed lifetime and recreation");
}
'''
if '--copy' in sys.argv:
    header = '#define WM_TARGET_COPY 1\n' + header
    header = header.replace('W3D_Scissor scissor;} W3D_Context;', 'W3D_Scissor scissor;ULONG state;} W3D_Context;')
    header += r'''
#include <stdlib.h>
#include <string.h>
#define W3D_DOUBLEHEIGHT 32
#define W3D_NODRIVER 3
#define W3D_NOMEMORY 4
#define W3D_UNSUPPORTEDFMT 5
#define W3D_CC_BITMAP 1
#define W3D_CC_YOFFSET 2
#define W3D_CC_DOUBLEHEIGHT 3
#define TAG_DONE 0
#define W3D_AUTOTEXMANAGEMENT 1
#define W3D_TEXMAPPING 2
#define W3D_GOURAUD 4
#define W3D_ZBUFFERUPDATE 8
struct TagItem {ULONG ti_Tag,ti_Data;};
typedef struct {float r,g,b,a;} W3D_Color;
static W3D_Color wm_current_color;
static W3D_Context *active;
static int creating;
#define _glColor4f(a,b,c,d) ((void)0)
#define _glEnable(a) ((void)0)
#define _glShadeModel(a) ((void)0)
#define _glDepthMask(a) ((void)0)
#define BMA_DEPTH 3
#define BMF_MINPLANES 1
#define BMF_CLEAR 2
#define BMF_DISPLAYABLE 4
static struct BitMap private_bitmap;
static int allocations,frees,copies,waits,fail_alloc,bad_format;
static int upper_pixel,lower_pixel;
static struct BitMap *AllocBitMap(int w,int h,int depth,int flags,struct BitMap *friend){
 (void)depth;assert(friend&&flags==(BMF_MINPLANES|BMF_DISPLAYABLE|BMF_CLEAR));
 if(fail_alloc)return NULL;
 ++allocations;private_bitmap=(struct BitMap){w,h,bad_format?2:4,0};return &private_bitmap;
}
static void FreeBitMap(struct BitMap *b){assert(b==&private_bitmap&&!driver_bitmap);++frees;}
static void BltBitMap(struct BitMap *src,int x,int y,struct BitMap *dst,int dx,int dy,
                     int w,int h,int minterm,int mask,void *temp){
 assert(src==driver_bitmap&&src==&private_bitmap&&dst!=src);
 assert(!x&&!y&&!dx&&!temp&&w==src->w&&h==src->h&&minterm==0xc0&&mask==0xff);
 assert(dy>=0&&dy+h<=dst->h);
 if(dst->h==2*h){if(dy==0)upper_pixel=src->pixel;else {assert(dy==h);lower_pixel=src->pixel;}}
 assert(finished>0);dst->pixel=src->pixel;++copies;
}
static void WaitBlit(void){++waits;}
'''
    # TARGET COPY helpers (Fake16, options, logging) live above setup().
    i = source.index('#ifdef WM_TARGET_COPY\n#include <proto/dos.h>')
    helpers = source[i:source.index('\n#endif\n', i) + 8]
    helpers = '\n'.join(l for l in helpers.splitlines() if not l.startswith('#include'))
    # The seed (CPU pixels -> renderer) needs a GL context; not under test here.
    lifecycle = lifecycle.replace('#include "seed.c"', 'static void seed_forget(void){}\nstatic void seed_free(void){}\n'
                                  'static void wm_describe_target(W3D_Context *c){(void)c;}')
    header = header.replace(
        'static unsigned p96GetBitMapAttr(struct BitMap *b,unsigned attr){assert(attr==P96BMA_BYTESPERPIXEL);return b->bpp;}',
        'static unsigned p96GetBitMapAttr(struct BitMap *b,unsigned attr){assert(attr==P96BMA_BYTESPERPIXEL||attr==P96BMA_RGBFORMAT);'
        'return attr==P96BMA_BYTESPERPIXEL?(unsigned)b->bpp:9u;}')
    header = header.replace('#define P96BMA_BYTESPERPIXEL 1', '#define P96BMA_BYTESPERPIXEL 1\n#define P96BMA_RGBFORMAT 2')
    header += r'''
#define RGBFB_A8R8G8B8 9
#define RGBFB_R5G6B5 10
#define BMF_USERPRIVATE 8
static void *DOSBase;
static long GetVar(const char *n,char *b,long s,long f){(void)n;(void)b;(void)s;(void)f;return -1;}
static void wm_log_open(void){}
static int p96allocations,p96frees;
static struct BitMap *p96AllocBitMap(int w,int h,int depth,int flags,struct BitMap *friend,int format){
 assert(depth==32&&!friend&&format==RGBFB_A8R8G8B8&&(flags&BMF_CLEAR));
 if(fail_alloc)return NULL;
 ++allocations;++p96allocations;private_bitmap=(struct BitMap){w,h,4,0};return &private_bitmap;
}
static void p96FreeBitMap(struct BitMap *b){assert(b==&private_bitmap&&!driver_bitmap);++frees;++p96frees;}
''' + helpers
    context_source = (root/'Warp3D.library/Context.c').read_text()
    creation = context_source[context_source.index('W3D_Context *W3D_CreateContext('):context_source.index('void W3D_DestroyContext(')]
    region += '\n' + creation
    main = r'''
ULONG W3D_GetDriverState(W3D_Context *context){(void)context;return wm_copy_error;}
int main(void){
 struct BitMap a={320,240,4,0},b={320,240,4,0},narrow={320,240,3,0};
 W3D_Context c={&a,0,320,240,{0,0,320,240},0};
 W3D_Scissor scissor={7,2,100,120};
 assert(MiniGLOpen());
 fail_alloc=1;assert(!wm_create_target(&a,0,320,240)&&!creates);fail_alloc=0;
 bad_format=1;assert(!wm_create_target(&a,0,320,240)&&allocations==frees);bad_format=0;
 fail_create=1;assert(!wm_create_target(&a,0,320,240)&&allocations==frees&&!current);fail_create=0;
 assert(!wm_create_target(&narrow,0,320,240));
 assert(!wm_create_target(&a,1,320,240));
 assert(wm_create_target(&a,0,320,240)&&driver_bitmap==&private_bitmap&&borrowed_bitmap!=&a);
 wm_reset_readback();wm_dirty=1;wm_sync_bitmap(&c);
 assert(a.pixel==42&&b.pixel==0&&copies==1&&waits==1&&!wm_dirty);
 wm_sync_bitmap(&c);assert(copies==1);
 /* Flush pending work into the OLD destination before switching. */
 a.pixel=0;wm_dirty=1;
 assert(W3D_SetDrawRegion(&c,&b,0,&scissor)==0&&c.drawregion==&b);
 assert(a.pixel==42&&b.pixel==0&&copies==2&&c.scissor.left==7);
 wm_dirty=1;wm_sync_bitmap(&c);assert(b.pixel==42&&a.pixel==42&&copies==3);
 for(int i=0;i<8;++i){
  struct BitMap *next=i%2?&b:&a;
  assert(W3D_SetDrawRegion(&c,next,0,&scissor)==0);
  next->pixel=0;wm_dirty=1;wm_sync_bitmap(&c);assert(next->pixel==42);
 }
 assert(creates==2&&setup_calls==1); /* failed creation + one live context */
 wm_dirty=1;scissor.left=11;
 assert(W3D_SetDrawRegion(&c,&narrow,0,&scissor)==W3D_UNSUPPORTED&&wm_dirty&&c.scissor.left==7);
 assert(W3D_SetDrawRegion(&c,&a,1,&scissor)==W3D_UNSUPPORTED);
 a.w=640;assert(W3D_SetDrawRegion(&c,&a,0,&scissor)==W3D_UNSUPPORTED);a.w=320;
 fail_finish=1;finished=0;
 int oldcopies=copies;
 assert(W3D_SetDrawRegion(&c,&a,0,&scissor)==W3D_UNSUPPORTED&&wm_dirty&&c.drawregion==&b);
 assert(copies==oldcopies);
 fail_finish=0;assert(W3D_SetDrawRegion(&c,&a,0,&scissor)==0&&!wm_dirty);
 assert(copies==waits);
 wm_destroy();assert(!backend&&!current&&!borrowed_bitmap&&allocations==frees&&deletes==1);
 wm_destroy();assert(deletes==1&&allocations==frees);
 b.w=640;b.h=480;c.drawregion=&b;
 assert(wm_create_target(&b,0,640,480));wm_dirty=1;wm_sync_bitmap(&c);
 assert(b.pixel==42&&target_width==640);wm_destroy();assert(deletes==2&&allocations==frees);
 /* Real context creation must honor DOUBLEHEIGHT, including initial offset. */
 struct BitMap tall={320,480,4,0};ULONG err;
 struct TagItem tags[]={{W3D_CC_BITMAP,(ULONG)&tall},{W3D_CC_DOUBLEHEIGHT,1},
                       {W3D_CC_YOFFSET,240},{TAG_DONE,0}};
 W3D_Context *dc=W3D_CreateContext(&err,tags);
 assert(dc&&err==0&&dc->height==240&&dc->yoffset==240&&(dc->state&W3D_DOUBLEHEIGHT));
 assert(private_bitmap.h==240&&width==320&&height==240);
 wm_dirty=1;wm_sync_bitmap(dc);assert(lower_pixel==42&&upper_pixel==0);
 int live_creates=creates;
 for(int i=0;i<8;++i){
  int offset=i%2?240:0;
  assert(W3D_SetDrawRegion(dc,&tall,offset,NULL)==0);
  assert(dc->height==240&&height==240&&dc->yoffset==offset);
  if(offset)lower_pixel=0;else upper_pixel=0;
  wm_dirty=1;wm_sync_bitmap(dc);assert((offset?lower_pixel:upper_pixel)==42);
 }
 assert(creates==live_creates&&upper_pixel==42&&lower_pixel==42);
 wm_dirty=1;int previous_copies=copies;
 assert(W3D_SetDrawRegion(dc,&tall,241,NULL)==W3D_UNSUPPORTED);
 assert(dc->yoffset==240&&wm_dirty&&copies==previous_copies);
 assert(!wm_copy_target(&tall,-1)&&!wm_copy_target(&tall,241));
 wm_destroy();free(dc);active=NULL;
 assert(allocations==frees);
 /* 15/16-bit destination: private 32-bit renderer from p96AllocBitMap. */
 {struct BitMap h16={320,240,2,0};W3D_Context hc={&h16,0,320,240,{0,0,320,240},0};
  assert(wm_create_target(&h16,0,320,240)&&p96allocations==1&&driver_bitmap==&private_bitmap);
  wm_dirty=1;wm_sync_bitmap(&hc);assert(h16.pixel==42);
  wm_destroy();assert(p96frees==1&&allocations==frees);}
 MiniGLClose();
 puts("PASS: TARGET COPY 16-bit destination, rotating bitmaps and DOUBLEHEIGHT offsets, real context creation, bounds, persistent renderer and cleanup");
}
'''
with tempfile.TemporaryDirectory() as directory:
    p = Path(directory)
    (p/'test.c').write_text('\n'.join([header,loader,lifecycle,present,state,sync,region,main]))
    subprocess.run(['cc','-std=gnu99','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
                    '-fno-pie','-no-pie',str(p/'test.c'),'-o',str(p/'test')],check=True)
    subprocess.run([str(p/'test')],check=True)
