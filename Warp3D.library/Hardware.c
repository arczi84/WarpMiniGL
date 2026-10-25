#include "w3d.h"
void wm_sync_bitmap(W3D_Context *context);
static ULONG wm_copy_error;
#ifndef WM_DIRECT_PRESENT
static int wm_readback_checked;
static UBYTE *wm_readback_pixels;
static int wm_readback_capacity;
extern void qt_debug(const char *text);
extern int wm_read_legacy_rgba(UBYTE *pixels,int w,int h);
void wm_reset_readback(void) {wm_copy_error=W3D_SUCCESS;wm_readback_checked=0;}
void wm_release_readback(void) {
	free(wm_readback_pixels);wm_readback_pixels=NULL;wm_readback_capacity=0;
	wm_readback_checked=0;
}

/* Some MiniGL implementations silently ignore unsupported pixel formats.
 * Probe with two different seeds so a legitimate pixel matching the first
 * seed cannot be mistaken for an unimplemented readback. Do this once per
 * context, before any uninitialized output can reach the Amiga bitmap. */
static int wm_check_readback(void) {
	UBYTE pixel[4];
	int attempt,j,changed;
	for(attempt=0;attempt<2;++attempt){
		for(j=0;j<4;++j)pixel[j]=(UBYTE)(0x35+j*37+attempt*83);
		_glReadPixels(0,0,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);
		if(MiniGLDispatch->GLGetError(MC)!=GL_NO_ERROR)return 0;
		changed=0;
		for(j=0;j<4;++j)if(pixel[j]!=(UBYTE)(0x35+j*37+attempt*83))changed=1;
		if(changed)return 1;
	}
	qt_debug("[WarpMiniGL] MiniGL GLReadPixels(RGBA) did not write output; readback unsupported\n");
	return 0;
}
#else
extern int wm_present_direct(void);
#ifdef WM_TARGET_COPY
extern int wm_copy_target(struct BitMap *bitmap,int yoffset);
#endif
void wm_reset_readback(void) {wm_copy_error=W3D_SUCCESS;}
void wm_release_readback(void) {}
#endif

#include <proto/graphics.h>
#include <proto/picasso96.h>

ULONG W3D_CheckDriver(void) {
	LOG;
	return W3D_DRIVER_3DHW;
}
ULONG W3D_LockHardware(__REGA0(W3D_Context *context)) {
#ifdef WM_FRAME_TRACE
    wm_trace_event("W3D_LockHardware",context);
#endif

	LOG;
	sync_window(context);
#ifdef WM_TARGET_HOOKS
	{extern void wm_lock(W3D_Context *);wm_lock(context);}
#endif
	context->HWlocked = W3D_TRUE;
	return W3D_SUCCESS;
}
void W3D_UnLockHardware(__REGA0(W3D_Context *context)) {
#ifdef WM_FRAME_TRACE
    wm_trace_event("W3D_UnLockHardware",context);
#endif

	LOG;
	_glFinish();
#ifdef WM_TARGET_HOOKS
	{extern void wm_unlock_cpu(W3D_Context *);wm_unlock_cpu(context);}
#endif
	wm_sync_bitmap(context);
	context->HWlocked = W3D_FALSE;
}
/* Synchronize the MiniGL back buffer to the Amiga destination at the explicit
 * CPU synchronization points and hardware unlock. Skip unchanged frames.
 * This makes standard Warp3D + CyberGraphX readback work without UAE calls
 * or knowledge of QuarkTex inside MiniGL/the application. */
void wm_sync_bitmap(W3D_Context *context) {
#ifdef WM_DIRECT_PRESENT
	(void)context;
#ifdef WM_FRAME_TRACE
    wm_trace_event("sync",context);
#endif
	if(!wm_dirty)return;
	if(!wm_present_direct()){wm_copy_error=W3D_UNSUPPORTED;return;}
#ifdef WM_TARGET_COPY
    if(!wm_copy_target(context->drawregion,context->yoffset)){wm_copy_error=W3D_UNSUPPORTED;return;}
#endif
#ifdef WM_FRAME_TRACE
    wm_trace_event("sync-complete",context);
#endif
	wm_dirty=0;wm_copy_error=W3D_SUCCESS;
#else
	struct RastPort rp;
	struct RenderInfo ri;
	UBYTE *pixels;
	int bytes, required, legacy;
	if (!wm_dirty) return;
	if (!context->drawregion || width <= 0 || height <= 0 || width > 8191 ||
		height > 0x7fffffff / (width * 4)) {wm_copy_error=W3D_ILLEGALBITMAP;return;}
	/* Preserve rendering errors instead of draining them and reporting success. */
	if(MiniGLDispatch->GLGetError(MC)!=GL_NO_ERROR){wm_copy_error=W3D_UNSUPPORTED;return;}
	bytes = width * 4;
	required = bytes * height;
	if(required > wm_readback_capacity){
		pixels = malloc(required);
		if(!pixels){wm_copy_error=W3D_NOMEMORY;return;}
		free(wm_readback_pixels);
		wm_readback_pixels=pixels;wm_readback_capacity=required;
	}
	pixels=wm_readback_pixels;
#ifdef WM_READPIXELS_ONLY
	/* Test the public readback implementation regardless of backend ID strings. */
	legacy=0;
#else
	legacy=wm_read_legacy_rgba(pixels,width,height);
#endif
	if(legacy<0){wm_copy_error=W3D_UNSUPPORTED;return;}
	if(!legacy){
	_glPixelStorei(GL_PACK_ALIGNMENT, 1);
	if(!wm_readback_checked){
		if(!wm_check_readback()){wm_copy_error=W3D_UNSUPPORTED;return;}
		wm_readback_checked=1;
	}
	_glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE,
		(GLvoid *)(memoffset + (ULONG)pixels));
	if(MiniGLDispatch->GLGetError(MC)!=GL_NO_ERROR){wm_copy_error=W3D_UNSUPPORTED;return;}
	}
	InitRastPort(&rp);
	rp.BitMap = context->drawregion;
	/* RenderInfo uses a signed WORD stride: start at the top GL row and
	 * walk backwards. P96 can copy/convert the rectangle in one call,
	 * with no extra CPU buffer flip or per-scanline locking overhead. */
	ri.BytesPerRow = -bytes;
	ri.pad = 0;
	ri.RGBFormat = RGBFB_R8G8B8A8;
	ri.Memory = pixels + (height - 1) * bytes;
	p96WritePixelArray(&ri, 0, 0, &rp, 0, context->yoffset, width, height);
	wm_dirty=0;wm_copy_error=W3D_SUCCESS;
#endif
}
void W3D_WaitIdle(__REGA0(W3D_Context *context)) {
#ifdef WM_FRAME_TRACE
    wm_trace_event("W3D_WaitIdle",context);
#endif

	LOG;
	_glFinish();
#ifdef WM_TARGET_HOOKS
	{extern void wm_unlock_cpu(W3D_Context *);wm_unlock_cpu(context);}
#endif
	wm_sync_bitmap(context);
}
ULONG W3D_CheckIdle(__REGA0(W3D_Context *context)) {
#ifdef WM_FRAME_TRACE
    wm_trace_event("W3D_CheckIdle",context);
#endif

	LOG;
	_glFinish();
	return W3D_SUCCESS;
}
static ULONG wm_query(ULONG query){
    GLint limit=256;
    if(query>=W3D_Q_MAXTEXWIDTH && query<=W3D_Q_MAXTEXHEIGHT_P){
        _glGetIntegerv(GL_MAX_TEXTURE_SIZE,&limit);
        return limit>0?limit:256;
    }
    switch(query){
    case W3D_Q_DRAW_POINT:case W3D_Q_DRAW_LINE:case W3D_Q_DRAW_TRIANGLE:
    case W3D_Q_TEXMAPPING:case W3D_Q_BILINEARFILTER:case W3D_Q_LINEAR_REPEAT:case W3D_Q_LINEAR_CLAMP:
    case W3D_Q_PERSPECTIVE:case W3D_Q_PERSP_REPEAT:case W3D_Q_PERSP_CLAMP:
    case W3D_Q_ENV_REPLACE:case W3D_Q_ENV_DECAL:case W3D_Q_ENV_MODULATE:case W3D_Q_WRAP_ASYM:
    case W3D_Q_FLATSHADING:case W3D_Q_GOURAUDSHADING:case W3D_Q_ZBUFFER:case W3D_Q_ZBUFFERUPDATE:
    case W3D_Q_ZCOMPAREMODES:case W3D_Q_ALPHATEST:case W3D_Q_ALPHATESTMODES:
    case W3D_Q_BLENDING:case W3D_Q_ONE_ONE:case W3D_Q_SCISSOR:case W3D_Q_RECTTEXTURES:case W3D_Q_MASKING:case W3D_Q_CULLFACE:return W3D_FULLY_SUPPORTED;
    case W3D_Q_SRCFACTORS:case W3D_Q_DESTFACTORS:return W3D_PARTIALLY_SUPPORTED;
    default:return W3D_NOT_SUPPORTED;
    }
}
ULONG W3D_Query(__REGA0(W3D_Context *context),__REGD0(ULONG query),__REGD1(ULONG destfmt)){return wm_query(query);}
ULONG W3D_GetTexFmtInfo(__REGA0(W3D_Context *context),__REGD0(ULONG format),__REGD1(ULONG destfmt)){return format>=1&&format<=11?W3D_TEXFMT_SUPPORTED:W3D_TEXFMT_UNSUPPORTED;}
ULONG W3D_GetDriverState(__REGA0(W3D_Context *context)) {
	LOG;
	return wm_copy_error;
}
ULONG W3D_GetDestFmt(void) {
	LOG;
	return W3D_FMT_CLUT | W3D_FMT_R5G5B5 | W3D_FMT_B5G5R5 | W3D_FMT_R5G5B5PC | W3D_FMT_B5G5R5PC |
		W3D_FMT_R5G6B5 | W3D_FMT_B5G6R5 | W3D_FMT_R5G6B5PC | W3D_FMT_B5G6R5PC | W3D_FMT_R8G8B8 |
		W3D_FMT_B8G8R8 | W3D_FMT_A8R8G8B8 | W3D_FMT_A8B8G8R8 | W3D_FMT_R8G8B8A8 | W3D_FMT_B8G8R8A8;
}
W3D_Driver **W3D_GetDrivers(void) {
	LOG;
	return drivers;
}
ULONG W3D_QueryDriver(__REGA0(W3D_Driver *driver),__REGD0(ULONG query),__REGD1(ULONG destfmt)){return wm_query(query);}
ULONG W3D_GetDriverTexFmtInfo(__REGA0(W3D_Driver *driver),__REGD0(ULONG format),__REGD1(ULONG destfmt)){return format>=1&&format<=11?W3D_TEXFMT_SUPPORTED:W3D_TEXFMT_UNSUPPORTED;}
ULONG W3D_RequestMode(__REGA0(struct TagItem *taglist)) {
	int i;
	//ULONG modeID = 0;
	ULONG fmt = 0;
	//struct ScreenModeRequester *requester = AllocAslRequest(ASL_ScreenModeRequest, NULL);
	LOG;
	for (i = 0; taglist[i].ti_Tag != TAG_DONE; ++i) {
		switch (taglist[i].ti_Tag) {
		case W3D_SMR_SIZEFILTER: taglist[i].ti_Tag = TAG_IGNORE; break;
		case W3D_SMR_DRIVER: taglist[i].ti_Tag = TAG_IGNORE; break;
		//case W3D_SMR_DESTFMT: taglist[i].ti_Tag = TAG_IGNORE; break;
		case W3D_SMR_DESTFMT:
			taglist[i].ti_Tag = P96MA_FormatsAllowed;
			if (taglist[i].ti_Data & W3D_FMT_CLUT) fmt |= RGBFF_CLUT;
			if (taglist[i].ti_Data & W3D_FMT_R5G5B5) fmt |= RGBFF_HICOLOR;
			if (taglist[i].ti_Data & W3D_FMT_B5G5R5) fmt |= RGBFF_HICOLOR;
			if (taglist[i].ti_Data & W3D_FMT_R5G5B5PC) fmt |= RGBFF_HICOLOR;
			if (taglist[i].ti_Data & W3D_FMT_B5G5R5PC) fmt |= RGBFF_HICOLOR;
			if (taglist[i].ti_Data & W3D_FMT_R5G6B5) fmt |= RGBFF_HICOLOR;
			if (taglist[i].ti_Data & W3D_FMT_B5G6R5) fmt |= RGBFF_HICOLOR;
			if (taglist[i].ti_Data & W3D_FMT_R5G6B5PC) fmt |= RGBFF_HICOLOR;
			if (taglist[i].ti_Data & W3D_FMT_B5G6R5PC) fmt |= RGBFF_HICOLOR;
			if (taglist[i].ti_Data & W3D_FMT_R8G8B8) fmt |= RGBFF_TRUECOLOR;
			if (taglist[i].ti_Data & W3D_FMT_B8G8R8) fmt |= RGBFF_TRUECOLOR;
			if (taglist[i].ti_Data & W3D_FMT_A8R8G8B8) fmt |= RGBFF_TRUEALPHA;
			if (taglist[i].ti_Data & W3D_FMT_A8B8G8R8) fmt |= RGBFF_TRUEALPHA;
			if (taglist[i].ti_Data & W3D_FMT_R8G8B8A8) fmt |= RGBFF_TRUEALPHA;
			if (taglist[i].ti_Data & W3D_FMT_B8G8R8A8) fmt |= RGBFF_TRUEALPHA;
			taglist[i].ti_Data = fmt;
			break;
		case W3D_SMR_TYPE: taglist[i].ti_Tag = TAG_IGNORE; break;
		
		case ASLSM_MinWidth: taglist[i].ti_Tag = P96MA_MinWidth; break;
		case ASLSM_MinHeight: taglist[i].ti_Tag = P96MA_MinHeight; break;
		case ASLSM_MaxWidth: taglist[i].ti_Tag = P96MA_MaxWidth; break;
		case ASL_MaxHeight: taglist[i].ti_Tag = P96MA_MaxHeight; break;
		case ASLSM_MinDepth: taglist[i].ti_Tag = P96MA_MinDepth; break;
		case ASLSM_MaxDepth: taglist[i].ti_Tag = P96MA_MaxDepth; break;
		}
	}
	//AslRequest(requester, taglist);
	//modeID = requester->sm_DisplayID;
	//FreeAslRequest(requester);
	return p96RequestModeIDTagList(taglist);
	//return modeID;
}
W3D_Driver *W3D_TestMode(__REGD0(ULONG ModeID)) {
	LOG;
	return &driver;
}
