#include "w3d.h"
#include <proto/graphics.h>
#include <string.h>
extern int wm_create(int w,int h);
#ifdef WM_BORROWED_BITMAP
extern int wm_create_target(struct BitMap *,int,int,int);
#endif
extern void wm_destroy(void);
extern int wm_resize(int w,int h);
extern void wm_reset_readback(void);
static W3D_Context *active;
static int creating;
void sync_window(W3D_Context *context) { (void)context; }
W3D_Context *W3D_CreateContext(__REGA0(ULONG *error),__REGA1(struct TagItem *tags)) {
    W3D_Context *context;struct BitMap *bitmap=NULL;ULONG yoffset=0;
#ifdef WM_TARGET_HOOKS
    ULONG doubleheight=0;
#endif
    if(error)*error=W3D_NODRIVER;
    /* A Classic MiniGL backend may call us again: fail instead of recursing. */
    if(creating||active)return NULL;
    for(;tags && tags->ti_Tag!=TAG_DONE;++tags){
        if(tags->ti_Tag==W3D_CC_BITMAP)bitmap=(struct BitMap *)tags->ti_Data;
        if(tags->ti_Tag==W3D_CC_YOFFSET)yoffset=tags->ti_Data;
#ifdef WM_TARGET_HOOKS
        if(tags->ti_Tag==W3D_CC_DOUBLEHEIGHT)doubleheight=tags->ti_Data!=0;
#endif
    }
    if(!bitmap){if(error)*error=W3D_ILLEGALBITMAP;return NULL;}
    width=GetBitMapAttr(bitmap,BMA_WIDTH);height=GetBitMapAttr(bitmap,BMA_HEIGHT)-yoffset;
#ifdef WM_TARGET_HOOKS
    if(doubleheight)height=GetBitMapAttr(bitmap,BMA_HEIGHT)/2;
#endif
    if(width<=0||height<=0){if(error)*error=W3D_ILLEGALBITMAP;return NULL;}
    context=malloc(sizeof(*context));if(!context){if(error)*error=W3D_NOMEMORY;return NULL;}
    memset(context,0,sizeof(*context));creating=1;
#ifdef WM_BORROWED_BITMAP
    if(!wm_create_target(bitmap,yoffset,width,height)){creating=0;free(context);if(error)*error=W3D_UNSUPPORTEDFMT;return NULL;}
#else
    if(!wm_create(width,height)){creating=0;free(context);return NULL;}
#endif
    creating=0;active=context;wm_reset_readback();
    context->drawregion=bitmap;context->yoffset=yoffset;
    context->width=width;context->height=height;
    context->scissor.width=width;context->scissor.height=height;
#ifdef WM_TARGET_HOOKS
    {extern void wm_describe_target(W3D_Context *);wm_describe_target(context);}
#endif
    context->state=W3D_AUTOTEXMANAGEMENT|W3D_TEXMAPPING|W3D_GOURAUD|W3D_ZBUFFERUPDATE;
#ifdef WM_TARGET_HOOKS
    if(doubleheight)context->state|=W3D_DOUBLEHEIGHT;
#endif
#ifdef WM_FRAME_TRACE
    wm_trace_event("context-ready",context);
#endif
    wm_current_color=(W3D_Color){1.0f,1.0f,1.0f,1.0f};
    _glColor4f(1.0f,1.0f,1.0f,1.0f);
    _glEnable(GL_TEXTURE_2D);_glShadeModel(GL_SMOOTH);_glDepthMask(GL_TRUE);
    if(error)*error=W3D_SUCCESS;
    return context;
}
void W3D_DestroyContext(__REGA0(W3D_Context *context)) {
    extern void wm_free_textures(W3D_Context *);
    extern void qt_debug(const char *);
    if(!context||context!=active)return;
    qt_debug("[" WM_NAME " exit] DestroyContext: textures begin\n");
    wm_free_textures(context);
    qt_debug("[" WM_NAME " exit] DestroyContext: textures done\n");
    wm_destroy();active=NULL;
    qt_debug("[" WM_NAME " exit] DestroyContext: free wrapper context begin\n");
    free(context);
    qt_debug("[" WM_NAME " exit] DestroyContext: done\n");
}
ULONG W3D_GetState(__REGA0(W3D_Context *context), __REGD1(ULONG state)) {
	LOG;
	if (context->state & state) return W3D_ENABLED;
	return W3D_DISABLED;
}
ULONG W3D_SetState(W3D_Context *context __asm("a0"), ULONG state __asm("d0"), ULONG action __asm("d1")) {
	LOG;
    wm_trace("SetState",state,action,0);
    if(action==W3D_ENABLE && (state & (W3D_STENCILBUFFER|W3D_LOGICOP|W3D_CHROMATEST|W3D_SPECULAR|W3D_TEXMAPPING3D|W3D_FOGGING|W3D_ANTI_POINT|W3D_ANTI_LINE|W3D_ANTI_POLYGON|W3D_ANTI_FULLSCREEN)))return W3D_UNSUPPORTEDSTATE;

	if (action == W3D_ENABLE) {
		context->state |= state;
		switch (state) {
		case W3D_AUTOTEXMANAGEMENT:	break;
		case W3D_SYNCHRON:			break;
		case W3D_INDIRECT:			break;
		case W3D_GLOBALTEXENV:		break;
		case W3D_DOUBLEHEIGHT:		break;
		case W3D_FAST:				break;
		case W3D_TEXMAPPING:		_glEnable(GL_TEXTURE_2D); break;
		case W3D_PERSPECTIVE:		break;
		case W3D_GOURAUD:			_glShadeModel(GL_SMOOTH); break;
		case W3D_ZBUFFER:			_glEnable(GL_DEPTH_TEST); break;
		case W3D_ZBUFFERUPDATE:		_glDepthMask(GL_TRUE); break;
		case W3D_BLENDING:			_glEnable(GL_BLEND); break;
		case W3D_FOGGING:			_glEnable(GL_FOG); break;
		case W3D_ANTI_POINT:		break;
		case W3D_ANTI_LINE:			break;
		case W3D_ANTI_POLYGON:		break;
		case W3D_ANTI_FULLSCREEN:	break;
		case W3D_DITHERING:			break;
		case W3D_LOGICOP: return W3D_UNSUPPORTEDSTATE;
		case W3D_STENCILBUFFER:		return W3D_UNSUPPORTEDSTATE;
		case W3D_ALPHATEST:			_glEnable(GL_ALPHA_TEST); break;
		case W3D_SPECULAR:			break;
		case W3D_TEXMAPPING3D:		break;
		case W3D_SCISSOR:			_glEnable(GL_SCISSOR_TEST); break;
		case W3D_CHROMATEST:		return W3D_UNSUPPORTEDSTATE;
		case W3D_CULLFACE: _glEnable(GL_CULL_FACE); break;
		}
	}
	else {
		context->state &= ~state;
		switch (state) {
		case W3D_AUTOTEXMANAGEMENT:	break;
		case W3D_SYNCHRON:			break;
		case W3D_INDIRECT:			break;
		case W3D_GLOBALTEXENV:		break;
		case W3D_DOUBLEHEIGHT:		break;
		case W3D_FAST:				break;
		case W3D_TEXMAPPING:		_glDisable(GL_TEXTURE_2D); break;
		case W3D_PERSPECTIVE:		break;
		case W3D_GOURAUD:
            /* PiStorm's flat-textured shader path ignores projective Q.
             * Warp3D flat draws submit no per-vertex colors, so a constant
             * color on every vertex is equivalent under GL_SMOOTH and keeps
             * MiniGL's opaque/alpha-tested projective path available.
             * Restore the explicit constant color after any Gouraud draws. */
            _glShadeModel(GL_SMOOTH);
            _glColor4f(wm_current_color.r,wm_current_color.g,
                       wm_current_color.b,wm_current_color.a);
            break;
		case W3D_ZBUFFER:			_glDisable(GL_DEPTH_TEST); break;
		case W3D_ZBUFFERUPDATE:		_glDepthMask(GL_FALSE); break;
		case W3D_BLENDING:			_glDisable(GL_BLEND); break;
		case W3D_FOGGING:			_glDisable(GL_FOG); break;
		case W3D_ANTI_POINT:		break;
		case W3D_ANTI_LINE:			break;
		case W3D_ANTI_POLYGON:		break;
		case W3D_ANTI_FULLSCREEN:	break;
		case W3D_DITHERING:			break;
		case W3D_LOGICOP: break;
		case W3D_STENCILBUFFER:		return W3D_UNSUPPORTEDSTATE;
		case W3D_ALPHATEST:			_glDisable(GL_ALPHA_TEST); break;
		case W3D_SPECULAR:			break;
		case W3D_TEXMAPPING3D:		break;
		case W3D_SCISSOR:			_glDisable(GL_SCISSOR_TEST); break;
		case W3D_CHROMATEST:		return W3D_UNSUPPORTEDSTATE;
		case W3D_CULLFACE: _glDisable(GL_CULL_FACE); break;
		}
	}
	return W3D_SUCCESS;
}
ULONG W3D_Hint(__REGA0(W3D_Context *context), __REGD0(ULONG mode), __REGD1(ULONG quality)) {
	LOG;
	return W3D_SUCCESS;
}
