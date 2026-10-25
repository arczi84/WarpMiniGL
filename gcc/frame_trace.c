#include "../Warp3D.library/w3d.h"
#ifdef WM_FRAME_TRACE
#include <intuition/intuitionbase.h>
extern void qt_debug(const char *);
unsigned long wm_trace_draws,wm_trace_clears,wm_trace_depth;
static unsigned lines;
static void number(const char *label,ULONG value){
    char text[9];int i;
    for(i=7;i>=0;--i){text[i]="0123456789abcdef"[value&15];value>>=4;}
    text[8]=0;qt_debug(label);qt_debug(text);
}
void wm_trace_event(const char *event,W3D_Context *context){
    if(lines++>=256)return;
    qt_debug("[WMTRACE] ");qt_debug(event);
    number(" bitmap=",context?(ULONG)context->drawregion:0);
    if(context){number(" yoffset=",context->yoffset);number(" width=",context->width);
        number(" height=",context->height);number(" state=",context->state);}
    number(" dirty=",wm_dirty);
    number(" draws=",wm_trace_draws);number(" clears=",wm_trace_clears);
    number(" zclears=",wm_trace_depth);
    number(" active=",(ULONG)IntuitionBase->ActiveWindow);
    qt_debug("\n");
    if(lines==256)qt_debug("[WMTRACE] event limit reached\n");
}
void wm_trace_start(struct BitMap *bitmap){
    lines=0;wm_trace_draws=wm_trace_clears=wm_trace_depth=0;
    qt_debug("[WMTRACE] WarpMiniGL 4.18 TARGET COPY TRACE\n");
    qt_debug("[WMTRACE] MiniGL: ");
    qt_debug(MiniGLBase->lib_IdString?(const char *)MiniGLBase->lib_IdString:"unknown");
    number("\n[WMTRACE] dispatch-size=",MiniGLDispatch->structSize);
    number(" flags=",MiniGLDispatch->backendFlags);
    number(" game-bitmap=",(ULONG)bitmap);qt_debug("\n");
    wm_trace_event("before-create",NULL);
}
#endif
