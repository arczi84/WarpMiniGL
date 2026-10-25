from pathlib import Path
import subprocess
import tempfile

root=Path(__file__).resolve().parent.parent
gl='\n'.join(line for line in (root/'gl/gl.h').read_text().splitlines() if not line.startswith('#include'))
emit=''
source=(root/'Warp3D.library/V4Array.c').read_text()
arrays=source[source.index('inline void setTexture'):source.index('GLenum face[]')]
header=r'''
#include <assert.h>
#include <stddef.h>
#include <float.h>
#include <stdio.h>
typedef unsigned char UBYTE;typedef unsigned short UWORD;typedef unsigned int ULONG;
typedef float W3D_Float;typedef double W3D_Double;
typedef struct {int texwidth,texheight;} W3D_Texture;
typedef struct {
 unsigned state;void *VertexPointer,*TexCoordPointer[1];
 int VPStride,VPMode,TPStride[1],TPVOffs[1],TPWOffs[1],TPFlags[1];
 W3D_Texture *CurrentTex[1];
} W3D_Context;
#define W3D_SUCCESS 0
#define W3D_ILLEGALINPUT 1
#define W3D_TEXMAPPING 1
#define W3D_PERSPECTIVE 2
#define W3D_TEXCOORD_NORMALIZED 1
#define W3D_VERTEX_F_F_F 0
#define W3D_VERTEX_F_F_D 1
#define W3D_VERTEX_D_D_D 2
#define W3D_INDEX_UBYTE 0
#define W3D_INDEX_UWORD 1
#define W3D_INDEX_ULONG 2
#define __REGA0(x) x
#define __REGA1(x) x
#define __REGD0(x) x
#define __REGD1(x) x
#define __REGD2(x) x
static void *current=(void *)1;
static float result[3][8],uv[4];static int count;
int wm_dirty;
/* Diagnostics and the Texture.c registry are not under test here. */
static inline void wm_trace(const char *n,unsigned long a,unsigned long b,unsigned long c){(void)n;(void)a;(void)b;(void)c;}
static inline W3D_Texture *wm_valid_texture(W3D_Texture *t){return t;}
static unsigned int primitives[]={10,11,12};
static void tex4(void *c,float u,float v,float r,float q){assert(c==current);uv[0]=u;uv[1]=v;uv[2]=r;uv[3]=q;}
static void vertex(void *c,float x,float y,float z,float w){
 assert(c==current&&count<3);result[count][0]=x;result[count][1]=y;
 result[count][2]=z;result[count][3]=w;result[count][4]=uv[0];result[count][5]=uv[1];result[count][6]=uv[2];result[count++][7]=uv[3];
}
static void begin(void *c,unsigned p){assert(c==current&&p==10);count=0;}
static void end(void *c){assert(c==current);}
static struct {void **currentContext;void (*GLTexCoord4f)(void *,float,float,float,float);
 void (*GLVertex4f)(void *,float,float,float,float);
 void (*GLBegin)(void *,unsigned);void (*GLEnd)(void *);
} table={&current,tex4,vertex,begin,end},*MiniGLDispatch=&table;
static void setColor(W3D_Context *c,unsigned i){(void)c;(void)i;}
static void bindTexture(W3D_Texture *t){(void)t;}
'''
main=r'''
struct Input {float x,y;double z;float u,v,rhw;unsigned padding;};
static struct Input input[3]={{10,20,0.125,0,0,1,0},{30,40,0.5,256,0,0.5,0},{50,60,0.75,256,128,0.25,0}};
static void check(unsigned a,unsigned b,unsigned c,int perspective){
 unsigned index[3]={a,b,c};int i;assert(count==3);
 for(i=0;i<3;++i){const struct Input *v=&input[index[i]];
  assert(result[i][0]/result[i][3]==v->x&&result[i][1]/result[i][3]==v->y);
  assert(result[i][2]/result[i][3]==v->z);
  assert(result[i][4]/result[i][7]==v->u/256&&result[i][5]/result[i][7]==v->v/128);
  assert(result[i][3]==1&&result[i][0]==v->x&&result[i][1]==v->y&&result[i][2]==v->z);
  assert(result[i][7]==(perspective?v->rhw:1));
 }
}
int main(void){
 W3D_Texture tex={256,128};W3D_Context c={0};
 UBYTE byteidx[]={2,0,1};UWORD shortidx[]={1,2,0};ULONG longidx[]={0,2,1};
 float fff[3][3]={{10,20,0.125},{30,40,0.5},{50,60,0.75}};
 double ddd[3][3]={{10,20,0.125},{30,40,0.5},{50,60,0.75}};
 float normalized[3][3]={{0,0,1},{1,0,0.5},{1,1,0.25}};
 c.state=3;c.VertexPointer=input;c.VPStride=sizeof(input[0]);c.VPMode=W3D_VERTEX_F_F_D;
 c.CurrentTex[0]=&tex;c.TexCoordPointer[0]=&input[0].u;c.TPStride[0]=sizeof(input[0]);
 c.TPVOffs[0]=offsetof(struct Input,v)-offsetof(struct Input,u);
 c.TPWOffs[0]=offsetof(struct Input,rhw)-offsetof(struct Input,u);
 assert(!W3D_DrawArray(&c,0,0,3));check(0,1,2,1);
 assert(!W3D_DrawElements(&c,0,W3D_INDEX_UBYTE,3,byteidx));check(2,0,1,1);
 assert(!W3D_DrawElements(&c,0,W3D_INDEX_UWORD,3,shortidx));check(1,2,0,1);
 assert(!W3D_DrawElements(&c,0,W3D_INDEX_ULONG,3,longidx));check(0,2,1,1);
 c.VertexPointer=fff;c.VPStride=sizeof(fff[0]);c.VPMode=W3D_VERTEX_F_F_F;
 assert(!W3D_DrawArray(&c,0,0,3));check(0,1,2,1);
 c.VertexPointer=ddd;c.VPStride=sizeof(ddd[0]);c.VPMode=W3D_VERTEX_D_D_D;
 c.TPFlags[0]=W3D_TEXCOORD_NORMALIZED;c.TexCoordPointer[0]=normalized;c.TPStride[0]=sizeof(normalized[0]);
 assert(!W3D_DrawArray(&c,0,0,3));check(0,1,2,1);
 c.state&=~W3D_PERSPECTIVE;
 c.TPWOffs[0]=0x7fffffff; /* Disabled perspective must not read reciprocal W. */
 assert(!W3D_DrawArray(&c,0,0,3));check(0,1,2,0);
 puts("PASS: unscaled geometry in arrays/elements, all index/vertex formats, strided/normalized UV and affine mode");
 return 0;
}
'''
with tempfile.TemporaryDirectory() as directory:
    path=Path(directory)
    (path/'test.c').write_text(header+gl+'\n'+emit+arrays+main)
    subprocess.run(['cc','-std=gnu99','-fgnu89-inline','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined',str(path/'test.c'),'-o',str(path/'test')],check=True)
    subprocess.run([str(path/'test')],check=True)
