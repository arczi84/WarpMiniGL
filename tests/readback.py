from pathlib import Path
import subprocess,tempfile
p=Path(__file__).resolve().parent.parent
s=(p/'Warp3D.library/Hardware.c').read_text();a=s.index('void wm_sync_bitmap(W3D_Context *context) {');b=s.index('\nvoid W3D_WaitIdle',a);body=s[s.index('static ULONG wm_copy_error;'):s.index('#include <proto/graphics.h>')]+s[a:b]
header='''#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
typedef unsigned char UBYTE;typedef unsigned long ULONG;typedef void GLvoid;
#define W3D_NOMEMORY 3
#define W3D_ILLEGALBITMAP 4
#define W3D_UNSUPPORTED 30
#define W3D_SUCCESS 0
#define GL_NO_ERROR 0
#define GL_PACK_ALIGNMENT 1
#define GL_RGBA 2
#define GL_UNSIGNED_BYTE 3
#define RGBFB_R8G8B8A8 4
#define memoffset 0
#define MC 0
static int wm_dirty=1,width=2,height=2,reads=0,writes=0,fail=0,phase=0;
static int probes=0,noop=0,seed_match=0,prior_error=0;
void qt_debug(const char *s){(void)s;}
static int legacy_result;
static int allocations,frees,allocation_fail;
static void *test_malloc(size_t n){if(allocation_fail)return NULL;++allocations;return malloc(n);}
static void test_free(void *p){if(p)++frees;free(p);}
#define malloc test_malloc
#define free test_free
int wm_read_legacy_rgba(UBYTE *pixels,int w,int h){
#ifdef WM_READPIXELS_ONLY
 assert(!"READPIXELS must never call the legacy accessor");
#endif
 assert(w==2&&h==height);if(legacy_result>0){memset(pixels,31,8);memset(pixels+8,42,8);}return legacy_result;}
struct BitMap {unsigned char pixels[64];};
typedef struct {struct BitMap *drawregion;int yoffset;} W3D_Context;
struct RastPort {struct BitMap *BitMap;};
struct RenderInfo {void *Memory;short BytesPerRow,pad;int RGBFormat;};
static unsigned get_error(void *ctx){(void)ctx;if(prior_error){prior_error=0;return 1;}return fail&&phase?1:0;}
static struct {unsigned (*GLGetError)(void *);} dispatch={get_error},*MiniGLDispatch=&dispatch;
static void _glPixelStorei(int p,int value){assert(p==GL_PACK_ALIGNMENT&&value==1);}
static void _glReadPixels(int x,int y,int w,int h,int f,int type,void *data){assert(x==0&&y==0&&f==GL_RGBA&&type==GL_UNSIGNED_BYTE);if(w==1&&h==1){++probes;if(!noop){if(seed_match){unsigned char *p=data;int j;for(j=0;j<4;++j)p[j]=0x35+j*37;}else memset(data,11,4);}return;}assert(w==2&&h>=2&&h<=3);++reads;phase=1;{int row;for(row=0;row<h;++row)memset((char *)data+row*8,11*(row+1),8);}}
static void InitRastPort(struct RastPort *rp){memset(rp,0,sizeof(*rp));}
static void p96WritePixelArray(struct RenderInfo *ri,int sx,int sy,struct RastPort *rp,int x,int y,int w,int h){int row;assert(sx==0&&sy==0&&x==0&&w==2&&h==height&&ri->RGBFormat==RGBFB_R8G8B8A8&&ri->BytesPerRow==-8);for(row=0;row<h;++row)memcpy(rp->BitMap->pixels+(y+row)*8,(char *)ri->Memory+row*ri->BytesPerRow,8);++writes;}
'''
main='''int main(void){struct BitMap bitmap;W3D_Context ctx={&bitmap,1};memset(&bitmap,99,sizeof(bitmap));
 wm_sync_bitmap(&ctx);assert(reads==1&&writes==1&&wm_dirty==0&&wm_copy_error==0&&probes==1);
 assert(bitmap.pixels[0]==99&&bitmap.pixels[8]==22&&bitmap.pixels[16]==11&&bitmap.pixels[24]==99);
 wm_sync_bitmap(&ctx);assert(reads==1&&writes==1);
 fail=1;phase=0;wm_dirty=1;wm_sync_bitmap(&ctx);assert(reads==2&&writes==1&&wm_copy_error==W3D_UNSUPPORTED&&wm_dirty==1);
 fail=0;phase=0;noop=1;wm_reset_readback();wm_dirty=1;wm_sync_bitmap(&ctx);
 assert(probes==3&&reads==2&&writes==1&&wm_dirty==1&&wm_copy_error==W3D_UNSUPPORTED);
 noop=0;seed_match=1;wm_reset_readback();wm_sync_bitmap(&ctx);
 assert(probes==5&&reads==3&&writes==2&&wm_dirty==0&&wm_copy_error==0);
 prior_error=1;wm_dirty=1;wm_sync_bitmap(&ctx);
 assert(reads==3&&writes==2&&wm_dirty==1&&wm_copy_error==W3D_UNSUPPORTED);
 legacy_result=1;wm_sync_bitmap(&ctx);
#ifdef WM_READPIXELS_ONLY
 assert(reads==4&&writes==3&&wm_dirty==0&&wm_copy_error==0);
 assert(bitmap.pixels[8]==22&&bitmap.pixels[16]==11);
 legacy_result=-1;wm_dirty=1;wm_sync_bitmap(&ctx);
 assert(reads==5&&writes==4&&wm_dirty==0&&wm_copy_error==0);
 wm_dirty=1;
#else
 assert(reads==3&&writes==3&&wm_dirty==0&&wm_copy_error==0&&probes==5);
 assert(bitmap.pixels[8]==42&&bitmap.pixels[16]==31);
 legacy_result=-1;wm_dirty=1;wm_sync_bitmap(&ctx);
 assert(writes==3&&wm_dirty==1&&wm_copy_error==W3D_UNSUPPORTED);
#endif
 assert(allocations==1&&frees==0);
 legacy_result=0;seed_match=0;phase=0;fail=0;
 height=3;allocation_fail=1;wm_dirty=1;
 {int old_reads=reads,old_writes=writes;UBYTE *old=wm_readback_pixels;
  wm_sync_bitmap(&ctx);assert(wm_copy_error==W3D_NOMEMORY&&wm_dirty);
  assert(reads==old_reads&&writes==old_writes&&wm_readback_pixels==old&&wm_readback_capacity==16);}
 allocation_fail=0;wm_sync_bitmap(&ctx);
 assert(allocations==2&&frees==1&&!wm_dirty&&wm_readback_capacity==24);
 assert(bitmap.pixels[8]==33&&bitmap.pixels[16]==22&&bitmap.pixels[24]==11&&bitmap.pixels[32]==99);
 height=2;wm_dirty=1;wm_sync_bitmap(&ctx);assert(allocations==2&&frees==1);
 wm_dirty=1;ctx.drawregion=NULL;wm_sync_bitmap(&ctx);assert(wm_copy_error==W3D_ILLEGALBITMAP);
 wm_release_readback();wm_release_readback();
 assert(allocations==frees&&!wm_readback_pixels&&!wm_readback_capacity);
 ctx.drawregion=&bitmap;wm_reset_readback();wm_dirty=1;wm_sync_bitmap(&ctx);
 assert(allocations==3&&frees==2&&!wm_dirty);wm_release_readback();assert(allocations==frees);
 puts("PASS: cached buffers, growth failure/reuse/cleanup, one signed-stride P96 rectangle; MiniGL readback -> Warp3D bitmap, row order, y-offset, clean frames failed readback, silent no-op, matching probe color and pending GL errors");return 0;}
'''
with tempfile.TemporaryDirectory() as d:
 f=Path(d)/'test.c';f.write_text(header+body+main)
 for flags in ([],['-DWM_READPIXELS_ONLY']):
  subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-pie','-no-pie']+flags+[str(f),'-o',d+'/test'],check=True)
  subprocess.run([d+'/test'],check=True)
