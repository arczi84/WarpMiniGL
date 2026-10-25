#include "../Warp3D.library/w3d.h"
#include <intuition/intuitionbase.h>
#include <string.h>
static GLcontext backend;
extern void qt_debug(const char *);
#ifdef WM_BORROWED_BITMAP
#include <proto/Picasso96.h>
static struct BitMap *borrowed_bitmap;
static int target_width,target_height;
#ifdef WM_TARGET_COPY
#include <proto/dos.h>
/* The game's current destination (W3D_SetDrawRegion may rotate it). */
static struct BitMap *target;
static int target_yoffset;
/* ENV:WarpMiniGL/Fake16=1 with a 32-bit destination: the program gets a 16-bit
 * R5G6B5 copy of the bitmap as context->drawmem (Hyperion's FreeSpace draws
 * its 2D with the CPU in 16 bits whatever the format says). It reaches the
 * renderer through the seed (seed.c) and is refreshed after each frame. */
static struct BitMap *shadow;
static int want_fake16,copy_logs;
/* The render bitmap came from p96AllocBitMap (15/16-bit destination). */
static int private_p96;
static void seed_forget(void);
static void seed_free(void);
static void wm_number(const char *label,unsigned long value){
    extern void qt_debug(const char *);
    char buffer[12],*p=buffer+sizeof(buffer)-1;
    *p=0;do{*--p='0'+value%10;value/=10;}while(value);
    qt_debug(label);qt_debug(p);
}
static int usable(struct BitMap *bm){
    ULONG bpp=p96GetBitMapAttr(bm,P96BMA_BYTESPERPIXEL);
    return bpp==4||bpp==2;
}
static void free_render_bitmap(void){
    if(private_p96)p96FreeBitMap(borrowed_bitmap);else FreeBitMap(borrowed_bitmap);
    borrowed_bitmap=NULL;
}
static void read_options(void){
    char value[16];
    want_fake16=DOSBase&&GetVar("WarpMiniGL/Fake16",value,sizeof(value),0)>0&&value[0]=='1';
}
#endif
#endif
#ifdef WM_BORROWED_WINDOW
static int borrowed_width,borrowed_height;
static void wm_window_number(const char *label,unsigned value){
    extern void qt_debug(const char *);
    char buffer[12],*p=buffer+sizeof(buffer)-1;
    *p=0;do{*--p='0'+value%10;value/=10;}while(value);
    qt_debug(label);qt_debug(p);
}
/* Classic MiniGL opens its window without IDCMP; Hexen installs the port only
 * AFTER mglCreateContext returns. Prefer same-task IDCMP ownership, but also
 * accept the active, portless window if it is the ONLY size-matching window.
 * This is an explicit startup heuristic, not proof of portless ownership. */
static struct Window *wm_find_window(int w,int h){
    extern void qt_debug(const char *);
    struct Task *owner=FindTask(NULL);
    struct Screen *screen;
    struct Window *window,*found=NULL,*startup=NULL;
    ULONG lock=LockIBase(0);
    int count=0,matches=0,aw=0,ah=0;
    for(screen=IntuitionBase->FirstScreen;screen;screen=screen->NextScreen){
        for(window=screen->FirstWindow;window;window=window->NextWindow){
            if(window==IntuitionBase->ActiveWindow){
                aw=window->Width-window->BorderLeft-window->BorderRight;
                ah=window->Height-window->BorderTop-window->BorderBottom;
            }
            if(window->RPort && window->WScreen==screen &&
               window->Width-window->BorderLeft-window->BorderRight==w &&
               window->Height-window->BorderTop-window->BorderBottom==h){
                ++matches;
                if(window->UserPort && window->UserPort->mp_SigTask==owner){
                    found=window;++count;
                }else if(!window->UserPort && !window->IDCMPFlags &&
                         window==IntuitionBase->ActiveWindow){startup=window;}
            }
        }
    }
    UnlockIBase(lock);
    if(count==1)return found;
    if(!count && matches==1 && startup){
        qt_debug("[WarpMiniGL] WINDOW: using active window before IDCMP setup\n");
        return startup;
    }
    wm_window_number("[WarpMiniGL] WINDOW match: wanted=",(unsigned)w);
    wm_window_number("x",(unsigned)h);
    wm_window_number(" active=",aw>0?(unsigned)aw:0);
    wm_window_number("x",ah>0?(unsigned)ah:0);
    wm_window_number(" size matches=",(unsigned)matches);
    wm_window_number(" same-task=",(unsigned)count);
    qt_debug("\n");return NULL;
}
#endif
static int legacy_direct_readback;
int wm_dirty;
static int wm_id_contains(const char *text,const char *part){
    const char *a,*b;
    for(;*text;++text){
        for(a=text,b=part;*b && *a==*b;++a,++b){}
        if(!*b)return 1;
    }
    return 0;
}
int wm_init(void){
    extern void qt_debug(const char *text);
    if(!MiniGLOpen())return 0;
#ifdef WM_BORROWED_BITMAP
#ifdef WM_TARGET_COPY
    qt_debug("[WarpMiniGL] TARGET COPY: private render bitmap -> current game buffer\n");
#else
    qt_debug("[WarpMiniGL] TARGET: rendering into game bitmap through V27_2; no GLReadPixels\n");
#endif
#elif defined(WM_BORROWED_WINDOW)
    qt_debug("[WarpMiniGL] WINDOW: borrowing game window through V27; no GLReadPixels\n");
#elif defined(WM_DIRECT_PRESENT)
    qt_debug("[WarpMiniGL] DIRECT: MiniGL window, no framebuffer readback\n");
#elif defined(WM_READPIXELS_ONLY)
    qt_debug("[WarpMiniGL] READPIXELS: GL_RGBA -> game bitmap (32-bit), no legacy back-buffer access\n");
#else
    /* The inspected 14.0 direct backend silently rejects RGBA reads and
     * reads RGB from the visible window. Its public back-buffer accessor
     * exposes the native 32-bit BGRA target instead (format ID 0). */
    legacy_direct_readback=MiniGLBase->lib_Version==14 && MiniGLBase->lib_Revision==0 &&
        MiniGLBase->lib_IdString && wm_id_contains((const char *)MiniGLBase->lib_IdString,"PiStorm3D,") &&
        wm_id_contains((const char *)MiniGLBase->lib_IdString,"(26.8.2026)");
    if(legacy_direct_readback)qt_debug("[WarpMiniGL] PiStorm3D 14.0: 32-bit BGRA back buffer -> RGBA\n");
#endif
    return 1;
}
int wm_ready(void){return MiniGLDispatch!=NULL;}
static void setup(int w,int h){
    _glViewport(0,0,w,h);
    _glMatrixMode(GL_PROJECTION);_glLoadIdentity();
    MiniGLDispatch->GLOrtho(MC,0,w,h,0,0,-1);
    _glMatrixMode(GL_MODELVIEW);_glLoadIdentity();
    _glPixelStorei(GL_PACK_ALIGNMENT,1);_glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    /* State remains internal to the wrapper's one private MiniGL context. */
}
void wm_destroy(void);
#ifdef WM_BORROWED_BITMAP
int wm_create_target(struct BitMap *bitmap,int yoffset,int w,int h){
    extern void qt_debug(const char *);
    if(!MiniGLDispatch||backend||MC||!bitmap)return 0;
#ifdef WM_FRAME_TRACE
    wm_trace_start(bitmap);
#endif
    /* COPY uses a private whole bitmap and applies the offset during blit. */
#ifdef WM_TARGET_COPY
    if(yoffset<0||w<=0||h<=0||w!=(int)GetBitMapAttr(bitmap,BMA_WIDTH)||
       h>(int)GetBitMapAttr(bitmap,BMA_HEIGHT)||
       yoffset>(int)GetBitMapAttr(bitmap,BMA_HEIGHT)-h)return 0;
#else
    if(yoffset){qt_debug("[WarpMiniGL] TARGET: nonzero Y offset unsupported by V27_2\n");return 0;}
#endif
#ifdef WM_TARGET_COPY
    { extern void wm_log_open(void); wm_log_open(); }
    read_options();
    wm_number("[WarpMiniGL] create: destination bytes/pixel ",p96GetBitMapAttr(bitmap,P96BMA_BYTESPERPIXEL));
    wm_number(" rgbformat ",p96GetBitMapAttr(bitmap,P96BMA_RGBFORMAT));
    wm_number(" region ",w);wm_number("x",h);wm_number(" y+",yoffset);qt_debug("\n");
    if(!usable(bitmap)){
        qt_debug("[WarpMiniGL] TARGET COPY: destination must be a 15/16 or 32-bit RTG bitmap\n");return 0;
    }
    /* Never render into a screen buffer which the game may already display.
     * Keep one renderer (and its textures/Z buffer) while destinations rotate.
     * MiniGL renders 32-bit: a 15/16-bit game buffer gets a 32-bit renderer
     * and Picasso96's BltBitMap converts when copying. */
    private_p96=p96GetBitMapAttr(bitmap,P96BMA_BYTESPERPIXEL)!=4;
    if(!private_p96)
        borrowed_bitmap=AllocBitMap(w,h,GetBitMapAttr(bitmap,BMA_DEPTH),
                                   BMF_MINPLANES|BMF_DISPLAYABLE|BMF_CLEAR,bitmap);
    else
        borrowed_bitmap=p96AllocBitMap(w,h,32,BMF_CLEAR|BMF_DISPLAYABLE,NULL,RGBFB_A8R8G8B8);
    if(!borrowed_bitmap){qt_debug("[WarpMiniGL] TARGET COPY: no memory for the render bitmap\n");return 0;}
    if(p96GetBitMapAttr(borrowed_bitmap,P96BMA_BYTESPERPIXEL)!=4){
        free_render_bitmap();return 0;
    }
    backend=MiniGLDispatch->MGLCreateContextFromBitMap(borrowed_bitmap);
    if(!backend){
        qt_debug("[WarpMiniGL] TARGET COPY: MiniGL rejected the render bitmap\n");
        free_render_bitmap();return 0;
    }
    if(want_fake16 && p96GetBitMapAttr(bitmap,P96BMA_BYTESPERPIXEL)==4){
        shadow=p96AllocBitMap(GetBitMapAttr(bitmap,BMA_WIDTH),GetBitMapAttr(bitmap,BMA_HEIGHT),16,
                              BMF_CLEAR|BMF_USERPRIVATE,NULL,RGBFB_R5G6B5);
        if(shadow&&p96GetBitMapAttr(shadow,P96BMA_BYTESPERPIXEL)!=2){p96FreeBitMap(shadow);shadow=NULL;}
        qt_debug(shadow?"[WarpMiniGL] Fake16: 16-bit drawmem for the program's CPU drawing\n"
                       :"[WarpMiniGL] Fake16: no 16-bit bitmap\n");
    }
    target=bitmap;target_yoffset=yoffset;
#else
    if(p96GetBitMapAttr(bitmap,P96BMA_BYTESPERPIXEL)!=4){
        qt_debug("[WarpMiniGL] TARGET: requires 4 bytes per pixel; select a 32-bit RTG target\n");return 0;
    }
    backend=MiniGLDispatch->MGLCreateContextFromBitMap(bitmap);
    if(!backend){qt_debug("[WarpMiniGL] TARGET: MiniGL rejected game bitmap\n");return 0;}
    borrowed_bitmap=bitmap;
#endif
    *MiniGLDispatch->currentContext=backend;
    target_width=w;target_height=h;
#ifdef WM_FRAME_TRACE
    wm_trace_event("after-create",NULL);
#endif
    setup(w,h);wm_dirty=0;
#ifdef WM_TARGET_COPY
    qt_debug("[WarpMiniGL] context created\n");
#endif
    return 1;
}
int wm_target_region(struct BitMap *bitmap,int yoffset,int w,int h){
    extern void qt_debug(const char *);
#ifdef WM_TARGET_COPY
    if(backend&&bitmap&&yoffset>=0&&w==target_width&&h==target_height&&
       w==(int)GetBitMapAttr(bitmap,BMA_WIDTH)&&
       h<=(int)GetBitMapAttr(bitmap,BMA_HEIGHT)&&
       yoffset<=(int)GetBitMapAttr(bitmap,BMA_HEIGHT)-h&&usable(bitmap))return 1;
    qt_debug("[WarpMiniGL] TARGET COPY: incompatible size/format or region outside bitmap\n");
#else
    if(backend&&bitmap==borrowed_bitmap&&!yoffset&&w==target_width&&h==target_height)return 1;
    qt_debug("[WarpMiniGL] TARGET: changing bitmap/offset/size requires a MiniGL retarget API\n");
#endif
    return 0;
}
#ifdef WM_TARGET_COPY
int wm_copy_target(struct BitMap *bitmap,int yoffset){
    if(!wm_target_region(bitmap,yoffset,target_width,target_height))return 0;
    BltBitMap(borrowed_bitmap,0,0,bitmap,0,yoffset,target_width,target_height,0xc0,0xff,NULL);
    /* The caller may flip or free the destination immediately after unlock. */
    WaitBlit();
    if(shadow){
        BltBitMap(bitmap,0,yoffset,shadow,0,yoffset,target_width,target_height,0xc0,0xff,NULL);
        WaitBlit();
    }
    if(copy_logs<3){qt_debug("[WarpMiniGL] frame copied\n");++copy_logs;}
    return 1;
}
#include "seed.c"
#endif
#else
int wm_create(int w,int h){
    struct Window *previous=IntuitionBase->ActiveWindow,*window;
    if(!MiniGLDispatch||backend||MC)return 0;
#ifdef WM_BORROWED_WINDOW
    extern void qt_debug(const char *);
    previous=wm_find_window(w,h);
    if(!previous){qt_debug("[WarpMiniGL] WINDOW: no unambiguous window matching render size\n");return 0;}
    backend=MiniGLDispatch->MGLCreateContextFromWindow(previous);
#else
    MiniGLDispatch->mglChooseWindowMode(GL_TRUE);
    backend=MiniGLDispatch->MGLCreateContext(0,0,w,h);
#endif
    if(!backend)return 0;
    *MiniGLDispatch->currentContext=backend;
    window=MiniGLDispatch->MGLGetWindowHandle(backend);
#ifdef WM_BORROWED_WINDOW
    if(window!=previous){wm_destroy();return 0;}
#endif
    if(!window||window->Width-window->BorderLeft-window->BorderRight!=w||window->Height-window->BorderTop-window->BorderBottom!=h){wm_destroy();return 0;}
#ifdef WM_BORROWED_WINDOW
    borrowed_width=w;borrowed_height=h;
#elif defined(WM_DIRECT_PRESENT)
    WindowToFront(window);
#else
    WindowToBack(window);
#endif
#ifndef WM_BORROWED_WINDOW
    if(previous)ActivateWindow(previous);
#endif
    setup(w,h);wm_dirty=0;return 1;
}
#endif
void wm_destroy(void){
    extern void qt_debug(const char *);
    extern void wm_release_readback(void);
    wm_release_readback();
    if(backend){
#ifdef WM_TARGET_COPY
        seed_free();
#endif
        qt_debug("[WarpMiniGL exit] MGLDeleteContext: begin\n");
        MiniGLDispatch->MGLDeleteContext(backend);
        qt_debug("[WarpMiniGL exit] MGLDeleteContext: returned\n");
        backend=NULL;*MiniGLDispatch->currentContext=NULL;
#ifdef WM_BORROWED_BITMAP
#ifdef WM_TARGET_COPY
        free_render_bitmap();
        if(shadow){p96FreeBitMap(shadow);shadow=NULL;}
        target=NULL;seed_forget();
#endif
        borrowed_bitmap=NULL;target_width=target_height=0;
#endif
        qt_debug("[WarpMiniGL exit] MiniGL current context: cleared\n");
    }
}
int wm_resize(int w,int h){
#ifdef WM_BORROWED_BITMAP
    /* Never recreate a live context and silently lose its textures. */
    return backend&&w==target_width&&h==target_height;
#else
    struct Window *window;
    if(!backend)return 0;
#ifdef WM_BORROWED_WINDOW
    /* The new API documents creation, not safe in-place resize of a borrowed
     * render target. A size change needs game-driven context recreation. */
    if(w!=borrowed_width||h!=borrowed_height)return 0;
#else
    MiniGLDispatch->MGLResizeContext(backend,w,h);
#endif
    window=MiniGLDispatch->MGLGetWindowHandle(backend);
    if(!window||window->Width-window->BorderLeft-window->BorderRight!=w||window->Height-window->BorderTop-window->BorderBottom!=h)return 0;
    setup(w,h);wm_dirty=0;return 1;
#endif
}
void wm_exit(void){
    extern void qt_debug(const char *);
    wm_destroy();
    qt_debug("[WarpMiniGL exit] MiniGLClose: begin\n");
    MiniGLClose();
    qt_debug("[WarpMiniGL exit] MiniGLClose: returned\n");
}

/* Return 0 for other backends, 1 on success, -1 on a failed legacy read.
 * Use only public MiniGL calls and MGLLockInfo, never private context offsets.
 * MGLSwitchDisplay submits the current frame in this windowed backend;
 * GLFinish alone only waits for work that has already been submitted.
 * The CPU copy is completed before releasing the public back-buffer lock. */
#ifndef WM_DIRECT_PRESENT
int wm_read_legacy_rgba(UBYTE *pixels,int w,int h){
    MGLLockInfo info;
    int x,y,result=-1;
    if(!legacy_direct_readback)return 0;
    MiniGLDispatch->MGLSwitchDisplay(backend);
    MiniGLDispatch->GLFinish(backend);
    if(MiniGLDispatch->GLGetError(backend)!=GL_NO_ERROR)return -1;
    memset(&info,0,sizeof(info));
    if(!MiniGLDispatch->MGLLockBack(backend,&info))return -1;
    if(pixels && w>0 && h>0 && w<=8191 && h<=0x7fffffff/(w*4) &&
       info.base_address && info.width==(ULONG)w && info.height==(ULONG)h &&
       info.depth==32 && info.pixel_format==0 && info.pitch>=(ULONG)w*4 &&
       info.pitch<=0x7fffffffUL/(ULONG)h){
        for(y=0;y<h;++y){
            const UBYTE *src=(const UBYTE *)info.base_address+(ULONG)y*info.pitch;
            UBYTE *dst=pixels+(h-1-y)*w*4;
            for(x=0;x<w;++x){
                dst[0]=src[2];dst[1]=src[1];dst[2]=src[0];dst[3]=src[3];
                src+=4;dst+=4;
            }
        }
        result=1;
    }
    MiniGLDispatch->MGLUnlockDisplay(backend);
    return result;
}
#else
int wm_present_direct(void){
    if(!backend)return 0;
    if(MiniGLDispatch->GLGetError(backend)!=GL_NO_ERROR)return 0;
    MiniGLDispatch->MGLSwitchDisplay(backend);
    MiniGLDispatch->GLFinish(backend);
    return MiniGLDispatch->GLGetError(backend)==GL_NO_ERROR;
}
#endif
