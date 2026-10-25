#ifndef UAE_W3D
#define UAE_W3D

#include <stdlib.h>
#include <exec/libraries.h>
#include <proto/asl.h>

#include "Warp3D.h"
#include "../gl/gl.h"
#ifdef WM_FRAME_TRACE
void wm_trace_event(const char *event,W3D_Context *context);
void wm_trace_start(struct BitMap *bitmap);
#endif

typedef struct {
	GLuint glID;
	ULONG envparam;
	W3D_Color envcolor;
	ULONG MinFilter, MagFilter;
	ULONG s_mode, t_mode;
	W3D_Color bordercolor;
} Texture;

void drawVertex(W3D_Context* context, W3D_Vertex* v, W3D_Texture* tex);
void bindTexture(W3D_Texture* tex);

extern W3D_Driver driver;
extern W3D_Driver *drivers[];
/* Warp3D's constant color survives per-vertex Gouraud color submissions. */
extern W3D_Color wm_current_color;
extern void wm_trace(const char *name,ULONG a,ULONG b,ULONG c);

extern struct IntuitionBase *IntuitionBase;
extern struct GfxBase *GfxBase;
extern struct Library *P96Base;

extern char *bp, b;
extern int i;

void sync_window(W3D_Context *context);

extern int fullscreen;
extern int width, height;

#endif
