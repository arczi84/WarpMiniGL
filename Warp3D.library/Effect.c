#include "w3d.h"

void W3D_SetScissor(__REGA0(W3D_Context *context), __REGA1(W3D_Scissor *scissor));

GLenum w3dalpha[] = {0, GL_NEVER, GL_LESS, GL_GEQUAL, GL_LEQUAL, GL_GREATER, GL_NOTEQUAL, GL_EQUAL, GL_ALWAYS};
GLenum w3dblend[] = {0, GL_ZERO, GL_ONE, GL_SRC_COLOR, GL_DST_COLOR, GL_ONE_MINUS_SRC_COLOR, GL_ONE_MINUS_DST_COLOR, GL_SRC_ALPHA,
	GL_ONE_MINUS_SRC_ALPHA, GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA, GL_SRC_ALPHA_SATURATE, 0,0,0,0};
long w3dfog[] = {0, GL_LINEAR, GL_EXP, GL_EXP2, GL_EXP2}; //W3D_FOG_INTERPOLATED


ULONG W3D_SetAlphaMode(__REGA0(W3D_Context *context), __REGD1(ULONG mode), __REGA1(W3D_Float *refval)) {
	LOG;
	wm_trace("SetAlphaMode",mode,refval?*(ULONG *)refval:0,0);
	if(mode<1||mode>8||!refval)return W3D_UNSUPPORTEDATEST;
	_glAlphaFunc(w3dalpha[mode], *refval);
	return W3D_SUCCESS;
}
ULONG W3D_SetBlendMode(__REGA0(W3D_Context *context), __REGD0(ULONG srcfunc), __REGD1(ULONG dstfunc)) {
	LOG;
	wm_trace("SetBlendMode",srcfunc,dstfunc,0);
	if(srcfunc<1||srcfunc>11||dstfunc<1||dstfunc>11)return W3D_UNSUPPORTEDBLEND;
	_glBlendFunc(w3dblend[srcfunc], w3dblend[dstfunc]);
	return W3D_SUCCESS;
}
extern int wm_resize(int,int);
extern void wm_sync_bitmap(W3D_Context *context);
#ifdef WM_BORROWED_BITMAP
extern int wm_target_region(struct BitMap *,int,int,int);
extern int wm_retarget(struct BitMap *,int,int,int);
extern ULONG W3D_GetDriverState(__REGA0(W3D_Context *context));
#endif
ULONG W3D_SetDrawRegion(__REGA0(W3D_Context *context), __REGA1(struct BitMap *bm), __REGD1(int yoffset), __REGA2(W3D_Scissor *scissor)) {
    int w,h;
    if(!context||!bm||yoffset<0)return W3D_ILLEGALINPUT;
#ifdef WM_FRAME_TRACE
    wm_trace_event("region-old",context);
    { W3D_Context next=*context;next.drawregion=bm;next.yoffset=yoffset;wm_trace_event("region-request",&next); }
#endif
    w=GetBitMapAttr(bm,BMA_WIDTH);h=GetBitMapAttr(bm,BMA_HEIGHT)-yoffset;
#ifdef WM_TARGET_HOOKS
    if(context->state&W3D_DOUBLEHEIGHT)h=GetBitMapAttr(bm,BMA_HEIGHT)/2;
#endif
    if(w<=0||h<=0)return W3D_ILLEGALBITMAP;
#ifdef WM_BORROWED_BITMAP
    /* Validate before changing the destination or scissor. TARGET COPY keeps
     * the renderer fixed and allows compatible game buffers to rotate. */
    if(!wm_target_region(bm,yoffset,w,h))return W3D_UNSUPPORTED;
#endif
    wm_sync_bitmap(context);
#ifdef WM_BORROWED_BITMAP
    {ULONG status=W3D_GetDriverState(context);if(status!=W3D_SUCCESS)return status;}
#endif
#ifdef WM_GLES
    /* ogles2 renders into the game's own bitmap: follow its buffer rotation. */
    if(!wm_retarget(bm,yoffset,w,h))return W3D_UNSUPPORTED;
#endif
    if((w!=width||h!=height)&&!wm_resize(w,h))return W3D_UNSUPPORTED;
    width=w;height=h;context->width=w;context->height=h;
    context->drawregion=bm;context->yoffset=yoffset;
#ifdef WM_TARGET_HOOKS
    {extern void wm_describe_target(W3D_Context *);wm_describe_target(context);}
#endif
    if(scissor)W3D_SetScissor(context,scissor);
    return W3D_SUCCESS;
}
ULONG W3D_SetDrawRegionWBM(__REGA0(W3D_Context *context), __REGA1(W3D_Bitmap *bm), __REGA2(W3D_Scissor *scissor)) {return W3D_UNSUPPORTED;}

GLfloat color[] = { 0.0, 0.0, 0.0, 0.0 };

ULONG W3D_SetFogParams(__REGA0(W3D_Context *context), __REGA1(W3D_Fog *fogparams), __REGD1(ULONG fogmode)) {
	int i;
	LOG;
	_glFogf(GL_FOG_DENSITY, fogparams->fog_density);
	_glFogf(GL_FOG_START, fogparams->fog_start);
	_glFogf(GL_FOG_END, fogparams->fog_end);
	color[0] = fogparams->fog_color.r;
	color[1] = fogparams->fog_color.g;
	color[2] = fogparams->fog_color.b;
	SWAP32(color, 4)
	_glFogfv(GL_FOG_COLOR, (GLfloat*) (memoffset + (long) color));
	_glFogi(GL_FOG_MODE, w3dfog[fogmode]);
	return W3D_SUCCESS;
}
ULONG W3D_SetLogicOp(__REGA0(W3D_Context *context), __REGD1(ULONG operation)) { return W3D_UNSUPPORTEDLOGICOP; }

ULONG W3D_SetColorMask(__REGA0(W3D_Context *context), __REGD0(W3D_Bool red), __REGD1(W3D_Bool green), __REGD2(W3D_Bool blue), __REGD3(W3D_Bool alpha)) {
	LOG;
	_glColorMask((GLboolean) red, (GLboolean) green, (GLboolean) blue, (GLboolean) alpha);
	return W3D_SUCCESS;
}

ULONG W3D_SetPenMask(__REGA0(W3D_Context *context), __REGD1(ULONG pen)) {
	LOG;
	return W3D_SUCCESS;
}

ULONG W3D_SetCurrentColor(__REGA0(W3D_Context *context), __REGA1(W3D_Color *color)) {
	LOG;
	wm_current_color = *color;
	_glColor4f(color->r, color->g, color->b, color->a);
	return W3D_SUCCESS;
}

ULONG W3D_SetCurrentPen(__REGA0(W3D_Context *context), __REGD1(ULONG pen)) {
	LOG;
	return W3D_SUCCESS;
}

void W3D_SetScissor(__REGA0(W3D_Context *context), __REGA1(W3D_Scissor *scissor)) {
	LOG;
	if (!scissor) return;
	context->scissor = *scissor;
	sync_window(context);
	_glScissor(scissor->left,
	height - (scissor->top + scissor->height),
	//60,
	scissor->width, scissor->height);
	
	//{0,50,630,370}
	//640x480
	
	//480-(50+370)=60
}

void W3D_FlushFrame(__REGA0(W3D_Context *context)) {
#ifdef WM_FRAME_TRACE
    wm_trace_event("W3D_FlushFrame",context);
#endif

	LOG;
	_glFinish();
	wm_sync_bitmap(context);
}
