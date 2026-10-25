#include "w3d.h"
GLenum w3dz[]={0,GL_NEVER,GL_LESS,GL_GEQUAL,GL_LEQUAL,GL_GREATER,GL_NOTEQUAL,GL_EQUAL,GL_ALWAYS};
ULONG W3D_AllocZBuffer(__REGA0(W3D_Context *context)){context->zbufferalloc=W3D_TRUE;return W3D_SUCCESS;}
ULONG W3D_FreeZBuffer(__REGA0(W3D_Context *context)){context->zbufferalloc=W3D_FALSE;return W3D_SUCCESS;}
ULONG W3D_ClearZBuffer(__REGA0(W3D_Context *context),__REGA1(W3D_Double *value)){if(!value)return W3D_ILLEGALINPUT;MiniGLDispatch->GLClearDepth(MC,*value);_glDepthMask(GL_TRUE);_glClear(GL_DEPTH_BUFFER_BIT);_glDepthMask((context->state&W3D_ZBUFFERUPDATE)!=0);return W3D_SUCCESS;}
ULONG W3D_SetZCompareMode(__REGA0(W3D_Context *context),__REGD1(ULONG mode)){if(mode<1||mode>8)return W3D_UNSUPPORTEDZCMP;_glDepthFunc(w3dz[mode]);return W3D_SUCCESS;}
ULONG W3D_ReadZPixel(__REGA0(W3D_Context *context),__REGD0(ULONG x),__REGD1(ULONG y),__REGA1(W3D_Double *z)){return W3D_UNSUPPORTED;}
ULONG W3D_ReadZSpan(__REGA0(W3D_Context *context),__REGD0(ULONG x),__REGD1(ULONG y),__REGD2(ULONG n),__REGA1(W3D_Double *z)){return W3D_UNSUPPORTED;}
void W3D_WriteZPixel(__REGA0(W3D_Context *context),__REGD0(ULONG x),__REGD1(ULONG y),__REGA1(W3D_Double *z)){}
void W3D_WriteZSpan(__REGA0(W3D_Context *context),__REGD0(ULONG x),__REGD1(ULONG y),__REGD2(ULONG n),__REGA1(W3D_Double *z),__REGA2(UBYTE *mask)){}
