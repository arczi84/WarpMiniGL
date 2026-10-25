from pathlib import Path
import re,subprocess,tempfile
p=Path(__file__).resolve().parent.parent
macros='\n'.join(x for x in (p/'gl/gl.h').read_text().splitlines() if not x.startswith('#include'))
s=(p/'Warp3D.library/w3d.c').read_text();draw=s[s.index('void drawVertex'):]
prefix='''#include <assert.h>
#include <stdio.h>
#include <float.h>
#include <math.h>
typedef float GLfloat;
typedef struct {float r,g,b,a;} Color;
typedef struct {float x,y;double z;float w,u,v;Color color;} W3D_Vertex;
typedef struct {unsigned state;} W3D_Context;
typedef struct {int texwidth,texheight;} W3D_Texture;
#define W3D_TEXMAPPING 1
#define W3D_PERSPECTIVE 2
#define W3D_GOURAUD 4
#define W3D_ZBUFFER 8
int wm_dirty;
/* Texture validation (Texture.c registry): every test texture is live. */
static inline W3D_Texture *wm_valid_texture(W3D_Texture *t){return t;}
static float pos[4],uv[4],color[4];static void *current=(void *)1;
static void vertex(void *c,float x,float y,float z,float w){assert(c==current);pos[0]=x;pos[1]=y;pos[2]=z;pos[3]=w;}
static void tex4(void *c,float s,float t,float r,float q){assert(c==current);uv[0]=s;uv[1]=t;uv[2]=r;uv[3]=q;}
static void tex2(void *c,float s,float t){tex4(c,s,t,0,1);}
static void col(void *c,float r,float g,float b,float a){assert(c==current);color[0]=r;color[1]=g;color[2]=b;color[3]=a;}
static struct {void **currentContext;void (*GLVertex4f)(void *,float,float,float,float);void (*GLTexCoord4f)(void *,float,float,float,float);void (*GLTexCoord2f)(void *,float,float);void (*GLColor4f)(void *,float,float,float,float);} table={&current,vertex,tex4,tex2,col},*MiniGLDispatch=&table;
'''
main='''int main(void){
 W3D_Context c={15};W3D_Texture t={256,128};W3D_Vertex v={100,200,0.25,0.5,64,32,{1,0.5,0.25,1}};
 drawVertex(&c,&v,&t);
 assert(pos[0]/pos[3]==100&&pos[1]/pos[3]==200&&pos[2]/pos[3]==0.25);
 assert(uv[0]==0.125&&uv[1]==0.125&&uv[3]==0.5&&color[1]==0.5);
 c.state&=~W3D_PERSPECTIVE;drawVertex(&c,&v,&t);assert(uv[0]==0.25&&uv[1]==0.25&&uv[3]==1);
 c.state=0;drawVertex(&c,&v,0);assert(pos[2]==0);
 /* Compare interpolation at an interior point with the independent
  * Warp3D formula sum(lambda * uv * rhw) / sum(lambda * rhw). */
 {float weight[3]={0.2f,0.3f,0.5f},rhw[3]={1,0.25f,0.125f};
  float u[3]={0,1,1},vv[3]={0,0,1};double den=0,du=0,dv=0,refden=0,refu=0,refv=0;int i;
  c.state=15;
  for(i=0;i<3;++i){
   v.w=rhw[i];v.u=u[i]*t.texwidth;v.v=vv[i]*t.texheight;
   drawVertex(&c,&v,&t);assert(pos[3]==1&&pos[0]==v.x&&pos[1]==v.y&&pos[2]==v.z);
   /* The existing orthographic projection must preserve raster X/Y/Z. */
   assert(fabs((((2.0*pos[0]/640-pos[3])/pos[3])+1)*320-v.x)<0.001);
   assert(fabs((1-(pos[3]-2.0*pos[1]/480)/pos[3])*240-v.y)<0.001);
   assert(fabs(((2.0*pos[2]-pos[3])/pos[3]+1)*0.5-v.z)<0.00001);
   den+=weight[i]*uv[3];du+=weight[i]*uv[0];dv+=weight[i]*uv[1];
   refden+=weight[i]*rhw[i];refu+=weight[i]*rhw[i]*u[i];refv+=weight[i]*rhw[i]*vv[i];
  }
  assert(fabs(du/den-refu/refden)<0.000001&&fabs(dv/den-refv/refden)<0.000001);
  assert(fabs(du/den-0.8)>0.1); /* Reject affine interpolation. */
  v.w=0;drawVertex(&c,&v,&t);assert(pos[3]==1);
  v.w=-1;drawVertex(&c,&v,&t);assert(pos[3]==1);
 }
 puts("PASS: Warp3D vertex -> MiniGL dispatch, unscaled raster coordinates, depth preservation and projective texture arguments (GPU behavior untested)");return 0;
}
'''
with tempfile.TemporaryDirectory() as d:
 f=Path(d)/'test.c';f.write_text(prefix+macros+'\n'+draw+main)
 subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror',str(f),'-o',d+'/test'],check=True)
 subprocess.run([d+'/test'],check=True)
# Ensure this build uses only the standard MiniGL dispatch import vector.
assert '0xf0ffc0' not in (p/'gcc/backend.c').read_text()
assert 'QuarkTex.alib' not in (p/'gcc/backend.c').read_text()
