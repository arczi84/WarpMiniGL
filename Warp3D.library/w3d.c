#include "w3d.h"
#include <proto/exec.h>

struct Warp3DBase {
	struct Library base;
};

#pragma libbase Warp3DBase

W3D_Driver driver;
W3D_Driver *drivers[] = {&driver, NULL};
W3D_Color wm_current_color = {1.0f,1.0f,1.0f,1.0f};

struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
struct Library *P96Base;

char *bp, b;
int i;

int fullscreen = 0;
int width = 0;
int height = 0;

long envs[] = {0, GL_REPLACE, GL_DECAL, GL_MODULATE, GL_BLEND};

extern int wm_init(void);
extern void wm_exit(void);
void INIT_0_Warp3D(void) {
    driver.ChipID=W3D_CHIP_UNKNOWN;
    driver.formats=W3D_FMT_R8G8B8|W3D_FMT_A8R8G8B8|W3D_FMT_R8G8B8A8;
    driver.name="WarpMiniGL";driver.swdriver=W3D_FALSE;
    IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);
    GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);
    P96Base=OpenLibrary("Picasso96API.library",2);
    wm_init();
}
void EXIT_0_Warp3D(void) {
    wm_exit();
    if(P96Base){CloseLibrary(P96Base);P96Base=NULL;}
    if(GfxBase){CloseLibrary((struct Library *)GfxBase);GfxBase=NULL;}
    if(IntuitionBase){CloseLibrary((struct Library *)IntuitionBase);IntuitionBase=NULL;}
}
//W3D_Texture* lasttex;

extern W3D_Texture *wm_valid_texture(W3D_Texture *);
void bindTexture(W3D_Texture *tex) {
    Texture *data;tex=wm_valid_texture(tex);if(!tex)return;data=tex->driver;
    _glBindTexture(GL_TEXTURE_2D,data->glID);
    if(data->envparam)_glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,data->envparam);
}

void drawVertex(W3D_Context* context, W3D_Vertex* v, W3D_Texture* tex) {
	tex = wm_valid_texture(tex);
	if (context->state & W3D_TEXMAPPING && tex) {
		if(context->state & W3D_PERSPECTIVE)
			_glTexCoord4f(v->u*v->w/tex->texwidth,v->v*v->w/tex->texheight,0.0f,v->w);
		else _glTexCoord2f((float)((double)v->u/tex->texwidth),(float)((double)v->v/tex->texheight));
	}
	if (context->state & W3D_GOURAUD) _glColor4f(v->color.r, v->color.g, v->color.b, v->color.a);
	/* Keep raster positions independent of reciprocal depth. MiniGL does not
	 * honor vertex W consistently across all of this backend's shader paths. */
	if(context->state & W3D_ZBUFFER)_glVertex3f(v->x,v->y,v->z);
	else _glVertex2f(v->x,v->y);
}
