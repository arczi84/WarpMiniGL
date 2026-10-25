#ifndef WM_GL_H
#define WM_GL_H
#include <proto/minigl.h>
#include <stdlib.h>
/* Both interfaces run on the same Amiga: pointers and numeric arrays stay native. */
#define memoffset 0L
#define SWAP16(a,n)
#define SWAP32(a,n)
#define SWAP64(a,n)
#define LOG
extern int wm_dirty;
#ifdef WM_FRAME_TRACE
extern unsigned long wm_trace_draws,wm_trace_clears,wm_trace_depth;
#define WM_TRACE_DRAW() (++wm_trace_draws)
#define WM_TRACE_CLEAR(mask) ((void)(((mask)&GL_COLOR_BUFFER_BIT)?++wm_trace_clears:0), (void)(((mask)&GL_DEPTH_BUFFER_BIT)?++wm_trace_depth:0))
#else
#define WM_TRACE_DRAW() ((void)0)
#define WM_TRACE_CLEAR(mask) ((void)0)
#endif
#define MC (*MiniGLDispatch->currentContext)
#ifdef WM_TARGET_COPY
/* The program's CPU-drawn pixels go into the renderer first (gcc/seed.c). */
extern int wm_seed_pending;
extern void wm_seed(void);
#define WM_SEED() (wm_seed_pending?wm_seed():(void)0)
#else
#define WM_SEED() ((void)0)
#endif
#define _glAlphaFunc(a,b) MiniGLDispatch->GLAlphaFunc(MC,a,b)
#define _glBegin(a) (WM_SEED(), MiniGLDispatch->GLBegin(MC,a))
#define _glEnd() (MiniGLDispatch->GLEnd(MC), WM_TRACE_DRAW(), wm_dirty=1)
#define _glBindTexture(a,b) MiniGLDispatch->GLBindTexture(MC,a,b)
#define _glBlendFunc(a,b) MiniGLDispatch->GLBlendFunc(MC,a,b)
#define _glClear(a) (WM_SEED(), MiniGLDispatch->GLClear(MC,a), WM_TRACE_CLEAR(a), wm_dirty=1)
#define _glClearColor(a,b,c,d) MiniGLDispatch->GLClearColor(MC,a,b,c,d)
#define _glColor4f(a,b,c,d) MiniGLDispatch->GLColor4f(MC,a,b,c,d)
#define _glColor3f(a,b,c) _glColor4f(a,b,c,1.0f)
#define _glColor4ub(a,b,c,d) MiniGLDispatch->GLColor4ub(MC,a,b,c,d)
#define _glColor3ub(a,b,c) _glColor4ub(a,b,c,255)
#define _glColorMask(a,b,c,d) MiniGLDispatch->GLColorMask(MC,a,b,c,d)
#define _glDeleteTextures(a,b) MiniGLDispatch->GLDeleteTextures(MC,a,b)
#define _glDepthFunc(a) MiniGLDispatch->GLDepthFunc(MC,a)
#define _glDepthMask(a) MiniGLDispatch->GLDepthMask(MC,a)
#define _glEnable(a) MiniGLDispatch->MGLSetState(MC,a,GL_TRUE)
#define _glDisable(a) MiniGLDispatch->MGLSetState(MC,a,GL_FALSE)
#define _glFinish() MiniGLDispatch->GLFinish(MC)
#define _glFogf(a,b) MiniGLDispatch->GLFogf(MC,a,b)
#define _glFogfv(a,b) MiniGLDispatch->GLFogfv(MC,a,b)
#define _glFogi(a,b) _glFogf(a,b)
#define _glFrontFace(a) MiniGLDispatch->GLFrontFace(MC,a)
#define _glGenTextures(a,b) MiniGLDispatch->GLGenTextures(MC,a,b)
#define _glGetIntegerv(a,b) (MiniGLDispatch&&MC?MiniGLDispatch->GLGetIntegerv(MC,a,b):(void)0)
#define _glClearDepth(a) MiniGLDispatch->GLClearDepth(MC,a)
#define _glLoadIdentity() MiniGLDispatch->GLLoadIdentity(MC)
#define _glMatrixMode(a) MiniGLDispatch->GLMatrixMode(MC,a)
#define _glPixelStorei(a,b) MiniGLDispatch->GLPixelStorei(MC,a,b)
#define _glReadPixels(a,b,c,d,e,f,g) MiniGLDispatch->GLReadPixels(MC,a,b,c,d,e,f,g)
#define _glScalef(a,b,c) MiniGLDispatch->GLScalef(MC,a,b,c)
#define _glScissor(a,b,c,d) MiniGLDispatch->GLScissor(MC,a,b,c,d)
#define _glShadeModel(a) MiniGLDispatch->GLShadeModel(MC,a)
#define _glTexCoord2f(a,b) MiniGLDispatch->GLTexCoord2f(MC,a,b)
#define _glTexCoord4f(a,b,c,d) MiniGLDispatch->GLTexCoord4f(MC,a,b,c,d)
#define _glTexEnvi(a,b,c) MiniGLDispatch->GLTexEnvi(MC,a,b,c)
#define _glTexImage2D(a,b,c,d,e,f,g,h,i) MiniGLDispatch->GLTexImage2D(MC,a,b,c,d,e,f,g,h,i)
#define _glTexParameteri(a,b,c) MiniGLDispatch->GLTexParameteri(MC,a,b,c)
#define _glTexSubImage2D(a,b,c,d,e,f,g,h,i) MiniGLDispatch->GLTexSubImage2D(MC,a,b,c,d,e,f,g,h,i)
#define _glTranslatef(a,b,c) MiniGLDispatch->GLTranslatef(MC,a,b,c)
#define _glVertex3f(a,b,c) MiniGLDispatch->GLVertex4f(MC,a,b,c,1.0f)
#define _glVertex2f(a,b) _glVertex3f(a,b,0.0f)
#define _glViewport(a,b,c,d) MiniGLDispatch->GLViewport(MC,a,b,c,d)
#endif
