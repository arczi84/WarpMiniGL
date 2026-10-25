"""Exercise real state/color/vertex code, not just the projective UV formula."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
context = (root / 'Warp3D.library/Context.c').read_text()
state = context[context.index('ULONG W3D_GetState'):context.index('ULONG W3D_Hint')]
state = re.sub(r' __asm\("[ad][0-7]"\)', '', state)
effect = (root / 'Warp3D.library/Effect.c').read_text()
setter = effect[effect.index('ULONG W3D_SetCurrentColor'):effect.index('ULONG W3D_SetCurrentPen')]
vertices = (root / 'Warp3D.library/w3d.c').read_text()
vertices = vertices[vertices.index('void drawVertex'):]
arrays = (root / 'Warp3D.library/V4Array.c').read_text()
color = arrays[arrays.index('inline void setColor'):arrays.index('inline void setTexture')]
gl = '\n'.join(line for line in (root / 'gl/gl.h').read_text().splitlines()
               if not line.startswith('#include'))
constants = ''
for name in ('Warp3D.library/Warp3D.h', 'sdk/v23/include/mgl/gl.h'):
    for line in (root / name).read_text(encoding="latin-1").splitlines():
        if re.match(r'#define\s+(?:W3D|GL)_\w+\s+', line) and not line.endswith('\\'):
            constants += line.split('/*')[0] + '\n'

sdk_gl = (root / 'sdk/v23/include/mgl/gl.h').read_text(encoding='latin-1')
constants += sdk_gl[sdk_gl.index('enum\n{'):sdk_gl.index('};')+2] + '\n'

header = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
typedef unsigned char UBYTE,GLubyte;
typedef unsigned int ULONG;
typedef float W3D_Float;
typedef struct {float r,g,b,a;} W3D_Color;
typedef struct {unsigned state,CPMode;UBYTE *ColorPointer;int CPStride;} W3D_Context;
typedef struct {float x,y;double z;float w,u,v;W3D_Color color;} W3D_Vertex;
typedef struct {int texwidth,texheight;} W3D_Texture;
#define __REGA0(x) x
#define __REGA1(x) x
#define __REGD1(x) x
W3D_Color wm_current_color={1,1,1,1};
int wm_dirty;
/* Diagnostics and the Texture.c registry are not under test here. */
static inline void wm_trace(const char *n,unsigned long a,unsigned long b,unsigned long c){(void)n;(void)a;(void)b;(void)c;}
static inline W3D_Texture *wm_valid_texture(W3D_Texture *t){return t;}
static void *current=(void *)1;
static unsigned shade;
static W3D_Color gpu_color,captured_color;
static float pos[4],uv[4];
static void setshade(void *c,unsigned s){assert(c==current);shade=s;}
static void setstate(void *c,unsigned s,unsigned v){assert(c==current);(void)s;(void)v;}
static void depthmask(void *c,unsigned v){assert(c==current);(void)v;}
static void col(void *c,float r,float g,float b,float a){assert(c==current);gpu_color=(W3D_Color){r,g,b,a};}
static void colub(void *c,UBYTE r,UBYTE g,UBYTE b,UBYTE a){col(c,r/255.f,g/255.f,b/255.f,a/255.f);}
static void tex4(void *c,float s,float t,float r,float q){assert(c==current);uv[0]=s;uv[1]=t;uv[2]=r;uv[3]=q;}
static void tex2(void *c,float s,float t){tex4(c,s,t,0,1);}
static void vertex(void *c,float x,float y,float z,float w){
 assert(c==current);pos[0]=x;pos[1]=y;pos[2]=z;pos[3]=w;captured_color=gpu_color;
}
static struct {
 void **currentContext;
 void (*GLShadeModel)(void *,unsigned);
 void (*MGLSetState)(void *,unsigned,unsigned);
 void (*GLDepthMask)(void *,unsigned);
 void (*GLColor4f)(void *,float,float,float,float);
 void (*GLColor4ub)(void *,UBYTE,UBYTE,UBYTE,UBYTE);
 void (*GLTexCoord4f)(void *,float,float,float,float);
 void (*GLTexCoord2f)(void *,float,float);
 void (*GLVertex4f)(void *,float,float,float,float);
} table={&current,setshade,setstate,depthmask,col,colub,tex4,tex2,vertex},*MiniGLDispatch=&table;
'''
main = r'''
static void same(W3D_Color a,W3D_Color b){
 assert(a.r==b.r&&a.g==b.g&&a.b==b.b&&a.a==b.a);
}
int main(void){
 W3D_Context c={0};W3D_Texture tex={256,128};
 W3D_Vertex v={120,180,0.5,0.125,256,128,{0,1,0,1}};
 W3D_Color flat={0.75,0.5,0.25,0.5};
 float colors[2][4]={{1,0,0,1},{0,0,1,0.25}};
 float weights[3]={0.2,0.3,0.5},q[3]={1,0.25,0.125},u[3]={0,1,1};
 double num=0,den=0;int i;
 c.state=W3D_TEXMAPPING|W3D_PERSPECTIVE|W3D_ZBUFFER;
 c.ColorPointer=(UBYTE *)colors;c.CPStride=sizeof(colors[0]);
 c.CPMode=W3D_COLOR_FLOAT|W3D_CMODE_RGBA;
 W3D_SetCurrentColor(&c,&flat);
 W3D_SetState(&c,W3D_GOURAUD,W3D_ENABLE);
 drawVertex(&c,&v,&tex);same(captured_color,v.color);
 setColor(&c,1);same(gpu_color,(W3D_Color){0,0,1,0.25});
 /* A prior Gouraud draw must not replace Warp3D's saved constant color. */
 W3D_SetState(&c,W3D_GOURAUD,W3D_DISABLE);
 assert(W3D_GetState(&c,W3D_GOURAUD)==W3D_DISABLED);
 assert(shade==GL_SMOOTH);same(gpu_color,flat);
 setColor(&c,0);same(gpu_color,flat);
 for(i=0;i<3;++i){
  v.w=q[i];v.u=u[i]*tex.texwidth;
  drawVertex(&c,&v,&tex);same(captured_color,flat);
  assert(pos[0]==v.x&&pos[1]==v.y&&pos[2]==v.z&&pos[3]==1);
  /* Model the inspected opaque, single-texture MiniGL path: smooth shading
   * enables Q; GLTexCoord4f stores s/q and the GPU interpolates with 1/W=q.
   * GL_FLAT instead drops Q, giving affine interpolation. */
  {double reciprocal=shade==GL_SMOOTH?uv[3]:1;
   num+=weights[i]*(uv[0]/uv[3])*reciprocal;den+=weights[i]*reciprocal;}
 }
 assert(fabs(num/den-(0.3*0.25+0.5*0.125)/(0.2+0.3*0.25+0.5*0.125))<1e-6);
 assert(fabs(num/den-0.8)>0.1);
 flat=(W3D_Color){1,0,0,0.25};W3D_SetCurrentColor(&c,&flat);
 drawVertex(&c,&v,&tex);same(captured_color,flat);
 W3D_SetState(&c,W3D_GOURAUD,W3D_ENABLE);
 setColor(&c,0);same(gpu_color,(W3D_Color){1,0,0,1});
 W3D_SetState(&c,W3D_GOURAUD,W3D_DISABLE);same(gpu_color,flat);
 W3D_SetState(&c,W3D_PERSPECTIVE,W3D_DISABLE);
 drawVertex(&c,&v,&tex);assert(uv[3]==1&&pos[3]==1);
 puts("PASS: flat Warp3D color retains MiniGL projective shader selection; Gouraud transitions, array colors, alpha and raster positions preserved (GPU untested)");
}
'''
with tempfile.TemporaryDirectory() as directory:
    path = Path(directory)
    (path / 'test.c').write_text(header + constants + gl + '\n' + state + setter + color + vertices + main)
    subprocess.run(['cc', '-std=gnu99', '-fgnu89-inline', '-Wall', '-Wextra',
                    '-Werror', '-Wno-unused-parameter', '-fsanitize=address,undefined',
                    '-fno-pie', '-no-pie', str(path / 'test.c'), '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
