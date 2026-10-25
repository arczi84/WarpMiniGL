#include <exec/types.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/Picasso96.h>
#if defined(WM_BORROWED_WINDOW) || defined(WM_BORROWED_BITMAP)
#include <intuition/intuitionbase.h>
#endif
#include <stdio.h>
#include <string.h>
#include "../Warp3D.library/Warp3D.h"
struct Library *Warp3DBase;
struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
struct Library *P96Base;
#if !defined(WM_DIRECT_PRESENT) || defined(WM_BORROWED_BITMAP)
static void show_bitmap(struct Window *window,struct BitMap *bitmap){
    BltBitMapRastPort(bitmap,0,0,window->RPort,window->BorderLeft,window->BorderTop,320,240,0xc0);
}
#endif
#if !defined(WM_DIRECT_PRESENT) || defined(WM_BORROWED_WINDOW) || defined(WM_BORROWED_BITMAP)
static int inspect_pixels(struct RastPort *rp,int x,int y){
    struct RenderInfo ri;UBYTE background[4],center[4];
    memset(&ri,0,sizeof(ri));ri.BytesPerRow=4;ri.RGBFormat=RGBFB_R8G8B8A8;
    memset(background,0xa5,sizeof(background));memset(center,0xa5,sizeof(center));
    ri.Memory=background;p96ReadPixelArray(&ri,0,0,rp,x+5,y+5,1,1);
    ri.Memory=center;p96ReadPixelArray(&ri,0,0,rp,x+160,y+120,1,1);
    printf("Bitmap RGB: background=%u,%u,%u center=%u,%u,%u\n",
        (unsigned)background[0],(unsigned)background[1],(unsigned)background[2],
        (unsigned)center[0],(unsigned)center[1],(unsigned)center[2]);
    /* Allow RGB565 quantization. The center must contain interpolated color. */
    if(background[0]<8||background[0]>24||background[1]<24||background[1]>40||
       background[2]<40||background[2]>56||center[0]<32||center[1]<32||center[2]<64){
        puts("FAIL: bitmap does not contain the expected background and triangle.");return 0;
    }
    puts("PASS: sampled bitmap contains background and triangle colors.");return 1;
}
#endif
#if defined(WM_BORROWED_WINDOW) || defined(WM_BORROWED_BITMAP)
static int owned_windows(void){
    struct Screen *s;struct Window *w;struct Task *task=FindTask(NULL);
    ULONG lock=LockIBase(0);int count=0;
    for(s=IntuitionBase->FirstScreen;s;s=s->NextScreen)
        for(w=s->FirstWindow;w;w=w->NextWindow)
            if(w->UserPort&&w->UserPort->mp_SigTask==task)++count;
    UnlockIBase(lock);return count;
}
#endif
extern W3D_Context *test_create(ULONG *error __asm("a0"),struct TagItem *tags __asm("a1"));
extern void test_destroy(W3D_Context *context __asm("a0"));
extern ULONG test_lock(W3D_Context *context __asm("a0"));
extern void test_unlock(W3D_Context *context __asm("a0"));
extern ULONG test_state(W3D_Context *context __asm("a0"),ULONG state __asm("d0"),ULONG action __asm("d1"));
extern ULONG test_clear(W3D_Context *context __asm("a0"),ULONG color __asm("d0"));
extern ULONG test_triangle(W3D_Context *context __asm("a0"),W3D_Triangle *triangle __asm("a1"));
extern ULONG test_status(W3D_Context *context __asm("a0"));
#ifdef WM_TARGET_COPY
extern ULONG test_region(W3D_Context *context __asm("a0"),struct BitMap *bitmap __asm("a1"),int offset __asm("d1"),W3D_Scissor *scissor __asm("a2"));
#endif
int main(int argc,char **argv){
    struct Window *window=NULL;struct BitMap *bitmap=NULL;W3D_Context *context=NULL;
    struct TagItem tags[3];W3D_Triangle triangle;ULONG error;int result=20,frame,closed=0;
    struct Library *minigl;
#ifdef WM_TARGET_COPY
    struct BitMap *second_bitmap=NULL;
#endif
#if defined(WM_BORROWED_WINDOW) || defined(WM_BORROWED_BITMAP)
    int window_count=0;
#endif
    const char *library_path=argc>1?argv[1]:"Warp3D.library";
    puts("WarpMiniGL smoke diagnostic 4");
    IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);
    GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);
    P96Base=OpenLibrary("Picasso96API.library",2);
    if(!IntuitionBase||!GfxBase||!P96Base){puts("Cannot open graphics/Intuition/Picasso96 libraries");goto done;}
    printf("Requested library: %s\n",library_path);
    Warp3DBase=OpenLibrary(library_path,4);if(!Warp3DBase){puts("Cannot open Warp3D.library");goto done;}
    printf("Loaded Warp3D: %s\n",Warp3DBase->lib_IdString?(const char *)Warp3DBase->lib_IdString:"(no ID)");
    if(!Warp3DBase->lib_IdString||!strstr(Warp3DBase->lib_IdString,"WarpMiniGL")){
        puts("FAIL: loaded a different Warp3D driver; this would not test WarpMiniGL.");goto done;
    }
#ifdef WM_DIRECT_PRESENT
#ifdef WM_TARGET_COPY
    if(!strstr(Warp3DBase->lib_IdString,"TARGET COPY")){puts("This smoke requires WarpMiniGL TARGET COPY.");goto done;}
#else
    if(!strstr(Warp3DBase->lib_IdString,"DIRECT")){puts("This smoke requires WarpMiniGL DIRECT.");goto done;}
#endif
#ifdef WM_BORROWED_BITMAP
    if(!strstr(Warp3DBase->lib_IdString,"TARGET")){puts("This smoke requires WarpMiniGL TARGET V27_2.");goto done;}
#elif defined(WM_BORROWED_WINDOW)
    if(!strstr(Warp3DBase->lib_IdString,"WINDOW")){puts("This smoke requires WarpMiniGL WINDOW V27.");goto done;}
#endif
#else
    if(strstr(Warp3DBase->lib_IdString,"DIRECT")){puts("Use the smoke from the DIRECT build to test this driver.");goto done;}
#ifdef WM_READPIXELS_ONLY
    if(!strstr(Warp3DBase->lib_IdString,"READPIXELS")){puts("This smoke requires WarpMiniGL READPIXELS; unload the previous driver first.");goto done;}
#endif
#endif
    minigl=OpenLibrary("minigl.library",0);
    if(minigl){printf("Loaded MiniGL: %s\n",minigl->lib_IdString?(const char *)minigl->lib_IdString:"(no ID)");CloseLibrary(minigl);}
    window=OpenWindowTags(NULL,WA_Title,(ULONG)"WarpMiniGL test",WA_InnerWidth,320,WA_InnerHeight,240,WA_DragBar,TRUE,WA_DepthGadget,TRUE,WA_Activate,TRUE,WA_CloseGadget,TRUE,WA_SimpleRefresh,TRUE,WA_IDCMP,
#if defined(WM_BORROWED_WINDOW) || defined(WM_BORROWED_BITMAP)
        0, /* Reproduce Hexen: create the renderer before enabling IDCMP. */
#else
        IDCMP_REFRESHWINDOW|IDCMP_CLOSEWINDOW,
#endif
        TAG_DONE);
    if(!window)goto done;
#ifdef WM_TARGET_COPY
    bitmap=AllocBitMap(320,480,32,BMF_CLEAR|BMF_DISPLAYABLE,window->RPort->BitMap);if(!bitmap)goto done;
    second_bitmap=AllocBitMap(320,480,32,BMF_CLEAR|BMF_DISPLAYABLE,bitmap);if(!second_bitmap)goto done;
#else
    bitmap=AllocBitMap(320,240,32,BMF_CLEAR|BMF_DISPLAYABLE,window->RPort->BitMap);if(!bitmap)goto done;
#endif
    tags[0].ti_Tag=W3D_CC_BITMAP;tags[0].ti_Data=(ULONG)bitmap;tags[1].ti_Tag=TAG_DONE;tags[1].ti_Data=0;
#ifdef WM_TARGET_COPY
    tags[1].ti_Tag=W3D_CC_DOUBLEHEIGHT;tags[1].ti_Data=TRUE;
    tags[2].ti_Tag=TAG_DONE;tags[2].ti_Data=0;
#endif
#if defined(WM_BORROWED_WINDOW) || defined(WM_BORROWED_BITMAP)
    window_count=owned_windows();
#endif
    context=test_create(&error,tags);if(!context){printf("CreateContext failed: %ld\n",error);goto done;}
#if defined(WM_BORROWED_WINDOW) || defined(WM_BORROWED_BITMAP)
    if(owned_windows()!=window_count){puts("FAIL: creating the context opened another window.");goto done;}
    if(!ModifyIDCMP(window,IDCMP_REFRESHWINDOW|IDCMP_CLOSEWINDOW)){
        puts("FAIL: cannot enable IDCMP after context creation.");goto done;
    }
    window_count=owned_windows();
    puts("Context created before IDCMP setup, as in Hexen.");
#endif
    memset(&triangle,0,sizeof(triangle));triangle.v1.x=30;triangle.v1.y=210;triangle.v2.x=290;triangle.v2.y=210;triangle.v3.x=160;triangle.v3.y=30;
    triangle.v1.color.r=1;triangle.v2.color.g=1;triangle.v3.color.b=1;
    triangle.v1.color.a=triangle.v2.color.a=triangle.v3.color.a=1;
    error=test_lock(context);if(error){printf("Lock failed: %ld\n",error);goto done;}
    error=test_state(context,W3D_TEXMAPPING,W3D_DISABLE);
    if(!error)error=test_clear(context,0xff102030);
    if(!error)error=test_triangle(context,&triangle);
    test_unlock(context);
    if(error){printf("Draw failed: %ld\n",error);goto done;}
    error=test_status(context);printf("Driver state: %ld\n",error);
    if(error==W3D_SUCCESS){
#ifdef WM_TARGET_COPY
        for(frame=0;frame<4;++frame){
            struct BitMap *next=frame<2?bitmap:second_bitmap;
            int offset=(frame%2)*240;
            struct RastPort rp;
            error=test_region(context,next,offset,NULL);
            if(error){printf("FAIL: rotating destination: %ld\n",error);goto done;}
            InitRastPort(&rp);rp.BitMap=next;
            error=test_lock(context);if(error)goto done;
            error=test_clear(context,0xff102030);
            if(!error)error=test_triangle(context,&triangle);
            test_unlock(context);
            if(error||test_status(context)||!inspect_pixels(&rp,0,offset))goto done;
            if(offset&&!inspect_pixels(&rp,0,0))goto done;
        }
        puts("PASS: two DOUBLEHEIGHT bitmaps, both Y offsets, one context; upper half preserved.");
#endif
#ifdef WM_BORROWED_BITMAP
        {struct RastPort rp;InitRastPort(&rp);rp.BitMap=bitmap;
         if(inspect_pixels(&rp,0,0)){
             /* Reproduce the game's final blit: this exposed WINDOW's bug. */
             show_bitmap(window,bitmap);
             if(inspect_pixels(window->RPort,window->BorderLeft,window->BorderTop))result=0;
         }}
        puts("TARGET: samples check the supplied bitmap AND the game's final blit.");
        puts("Expect a colored triangle in this window; no helper window/readback.");
#elif defined(WM_BORROWED_WINDOW)
        if(inspect_pixels(window->RPort,window->BorderLeft,window->BorderTop))result=0;
        puts("WINDOW: expect the triangle in this single test window, with no helper window.");
        puts("Samples above are from the visible window; the offscreen Warp3D bitmap is not updated.");
#elif defined(WM_DIRECT_PRESENT)
        result=0;
        puts("DIRECT: inspect the colored triangle in MiniGL's window (10 seconds).");
        puts("No bitmap readback or pixel verification in this mode.");
#else
        {struct RastPort rp;InitRastPort(&rp);rp.BitMap=bitmap;if(inspect_pixels(&rp,0,0))result=0;}
        WindowToFront(window);ActivateWindow(window);show_bitmap(window,bitmap);
        puts("Displaying bitmap for 10 seconds (or close the window).");
#endif
        fflush(stdout);
        for(frame=0;frame<500&&!closed;++frame){
            struct IntuiMessage *message;
            while((message=(struct IntuiMessage *)GetMsg(window->UserPort))){
                ULONG event=message->Class;ReplyMsg((struct Message *)message);
                if(event==IDCMP_CLOSEWINDOW)closed=1;
                if(event==IDCMP_REFRESHWINDOW){
                    BeginRefresh(window);
#if !defined(WM_DIRECT_PRESENT) || defined(WM_BORROWED_BITMAP)
                    show_bitmap(window,bitmap);
#endif
                    EndRefresh(window,TRUE);
                }
            }
            Delay(1);
        }
    }
 done:
    if(context)test_destroy(context);
#if defined(WM_BORROWED_WINDOW) || defined(WM_BORROWED_BITMAP)
    if(context&&owned_windows()!=window_count){puts("FAIL: context deletion changed game window ownership.");result=20;}
#endif
    /* Borrowed bitmap/window outlive the rendering context. Keep dependencies
     * open until after the caller-owned resources have been released. */
    if(bitmap)FreeBitMap(bitmap);
#ifdef WM_TARGET_COPY
    if(second_bitmap)FreeBitMap(second_bitmap);
#endif
    if(window)CloseWindow(window);
    if(Warp3DBase)CloseLibrary(Warp3DBase);
    if(P96Base)CloseLibrary(P96Base);
    if(GfxBase)CloseLibrary((struct Library *)GfxBase);
    if(IntuitionBase)CloseLibrary((struct Library *)IntuitionBase);
    return result;
}
