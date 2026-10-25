/* TARGET COPY: pixels the program draws with the CPU (included by backend.c).
 *
 * Warp3D games draw 2D (menus, HUD, mouse pointer) straight into the
 * destination through context->drawmem while the hardware is locked, and mix
 * that with 3D. MiniGL renders into its own bitmap and replaces the
 * destination on every copy, so before the first GPU drawing after a
 * W3D_LockHardware the destination's pixels are drawn into the renderer as
 * textured quads ("seed"). Only public MiniGL calls are used: no backend
 * knowledge of how the render bitmap is stored. */
#define SEED_TILE 256
#ifndef GL_TEXTURE_BINDING_2D
#define GL_TEXTURE_BINDING_2D 0x8069
#endif
int wm_seed_pending;
static W3D_Context *seed_context;
static GLuint seed_names[64];
static int seed_count;
static UBYTE *seed_pixels;
static int seeds;

static void seed_forget(void){seed_count=0;seed_context=NULL;wm_seed_pending=0;}
static void seed_free(void){
    if(seed_count&&MC)_glDeleteTextures(seed_count,seed_names);
    seed_count=0;free(seed_pixels);seed_pixels=NULL;
}

static ULONG expand5(ULONG v){return (v<<3)|(v>>2);}
static ULONG expand6(ULONG v){return (v<<2)|(v>>4);}
/* One source row to RGBA bytes. */
static void seed_row(const UBYTE *src,UBYTE *dst,int n,ULONG format){
    int i;ULONG v;
    for(i=0;i<n;++i,dst+=4){
        switch(format){
        case RGBFB_R5G6B5:   v=src[0]<<8|src[1];src+=2;goto rgb565;
        case RGBFB_R5G6B5PC: v=src[1]<<8|src[0];src+=2;goto rgb565;
        case RGBFB_B5G6R5PC: v=src[1]<<8|src[0];src+=2;
            dst[2]=expand5(v>>11);dst[1]=expand6((v>>5)&63);dst[0]=expand5(v&31);break;
        rgb565: dst[0]=expand5(v>>11);dst[1]=expand6((v>>5)&63);dst[2]=expand5(v&31);break;
        case RGBFB_R5G5B5:   v=src[0]<<8|src[1];src+=2;goto rgb555;
        case RGBFB_R5G5B5PC: v=src[1]<<8|src[0];src+=2;goto rgb555;
        case RGBFB_B5G5R5PC: v=src[1]<<8|src[0];src+=2;
            dst[2]=expand5((v>>10)&31);dst[1]=expand5((v>>5)&31);dst[0]=expand5(v&31);break;
        rgb555: dst[0]=expand5((v>>10)&31);dst[1]=expand5((v>>5)&31);dst[2]=expand5(v&31);break;
        case RGBFB_A8B8G8R8: dst[0]=src[3];dst[1]=src[2];dst[2]=src[1];src+=4;break;
        case RGBFB_R8G8B8A8: dst[0]=src[0];dst[1]=src[1];dst[2]=src[2];src+=4;break;
        case RGBFB_B8G8R8A8: dst[0]=src[2];dst[1]=src[1];dst[2]=src[0];src+=4;break;
        default:             dst[0]=src[1];dst[1]=src[2];dst[2]=src[3];src+=4;break;
        }
        dst[3]=255;
    }
}

void wm_seed(void){
    W3D_Context *c=seed_context;
    struct BitMap *source;
    const UBYTE *memory;
    ULONG bpr,format,bpp;
    GLint bound=-1;
    GLboolean depth,blend,alpha,scissor,cull,fog,texture;
    int tx,ty,tiles_x,tiles_y,y0;
    wm_seed_pending=0;
    if(!c||!c->drawregion||!backend)return;
    source=shadow?shadow:c->drawregion;
    memory=(const UBYTE *)p96GetBitMapAttr(source,P96BMA_MEMORY);
    bpr=p96GetBitMapAttr(source,P96BMA_BYTESPERROW);
    format=p96GetBitMapAttr(source,P96BMA_RGBFORMAT);
    bpp=p96GetBitMapAttr(source,P96BMA_BYTESPERPIXEL);
    if(!memory||(bpp!=2&&bpp!=4))return;
    tiles_x=(target_width+SEED_TILE-1)/SEED_TILE;tiles_y=(target_height+SEED_TILE-1)/SEED_TILE;
    if(tiles_x*tiles_y>(int)(sizeof(seed_names)/sizeof(seed_names[0])))return;
    if(!seed_pixels&&!(seed_pixels=malloc(SEED_TILE*SEED_TILE*4)))return;
    if(seeds<3){qt_debug("[WarpMiniGL] seed\n");++seeds;}
    MiniGLDispatch->GLGetIntegerv(MC,GL_TEXTURE_BINDING_2D,&bound);
    depth=MiniGLDispatch->GLIsEnabled(MC,GL_DEPTH_TEST);
    blend=MiniGLDispatch->GLIsEnabled(MC,GL_BLEND);
    alpha=MiniGLDispatch->GLIsEnabled(MC,GL_ALPHA_TEST);
    scissor=MiniGLDispatch->GLIsEnabled(MC,GL_SCISSOR_TEST);
    cull=MiniGLDispatch->GLIsEnabled(MC,GL_CULL_FACE);
    fog=MiniGLDispatch->GLIsEnabled(MC,GL_FOG);
    texture=MiniGLDispatch->GLIsEnabled(MC,GL_TEXTURE_2D);
    _glDisable(GL_DEPTH_TEST);_glDisable(GL_BLEND);_glDisable(GL_ALPHA_TEST);
    _glDisable(GL_SCISSOR_TEST);_glDisable(GL_CULL_FACE);_glDisable(GL_FOG);
    _glEnable(GL_TEXTURE_2D);
    if(!seed_count){
        seed_count=tiles_x*tiles_y;
        _glGenTextures(seed_count,seed_names);
    }
    /* White vertices: REPLACE, MODULATE and DECAL all give the texel. */
    _glColor4f(1.0f,1.0f,1.0f,1.0f);
    for(ty=0;ty<tiles_y;++ty)for(tx=0;tx<tiles_x;++tx){
        int x=tx*SEED_TILE,y=ty*SEED_TILE,row;
        int w=target_width-x<SEED_TILE?target_width-x:SEED_TILE;
        int h=target_height-y<SEED_TILE?target_height-y:SEED_TILE;
        GLfloat s=(GLfloat)w/SEED_TILE,t=(GLfloat)h/SEED_TILE;
        y0=target_yoffset+y;
        for(row=0;row<h;++row)
            seed_row(memory+(ULONG)(y0+row)*bpr+(ULONG)x*bpp,seed_pixels+row*SEED_TILE*4,w,format);
        _glBindTexture(GL_TEXTURE_2D,seed_names[ty*tiles_x+tx]);
        _glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        _glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        _glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
        _glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
        _glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,SEED_TILE,SEED_TILE,0,GL_RGBA,GL_UNSIGNED_BYTE,seed_pixels);
        _glBegin(GL_QUADS);
        _glTexCoord2f(0.0f,0.0f);_glVertex2f((GLfloat)x,(GLfloat)y);
        _glTexCoord2f(s,0.0f);_glVertex2f((GLfloat)(x+w),(GLfloat)y);
        _glTexCoord2f(s,t);_glVertex2f((GLfloat)(x+w),(GLfloat)(y+h));
        _glTexCoord2f(0.0f,t);_glVertex2f((GLfloat)x,(GLfloat)(y+h));
        _glEnd();
    }
    if(depth)_glEnable(GL_DEPTH_TEST);
    if(blend)_glEnable(GL_BLEND);
    if(alpha)_glEnable(GL_ALPHA_TEST);
    if(scissor)_glEnable(GL_SCISSOR_TEST);
    if(cull)_glEnable(GL_CULL_FACE);
    if(fog)_glEnable(GL_FOG);
    if(!texture)_glDisable(GL_TEXTURE_2D);
    if(bound>=0)_glBindTexture(GL_TEXTURE_2D,(GLuint)bound);
    _glColor4f(wm_current_color.r,wm_current_color.g,wm_current_color.b,wm_current_color.a);
}

/* W3D_LockHardware: the program may draw into the destination from now on. */
void wm_lock(W3D_Context *c){if(backend){seed_context=c;wm_seed_pending=1;}}

/* W3D_UnLockHardware without GPU drawing: pixels drawn into the Fake16 copy
 * must still reach the destination. */
void wm_unlock_cpu(W3D_Context *c){
    if(!backend||!shadow||wm_dirty||!c->drawregion)return;
    BltBitMap(shadow,0,c->yoffset,c->drawregion,0,c->yoffset,target_width,target_height,0xc0,0xff,NULL);
    WaitBlit();
}

/* Warp3D publishes the destination for the program's own CPU drawing:
 * base address, row length and pixel format. Hyperion's games write through
 * context->drawmem while the hardware is locked, so these must describe the
 * real bitmap, never stay NULL. */
void wm_describe_target(W3D_Context *c){
    struct BitMap *bm=shadow?shadow:c->drawregion;
    GLint limit=256;
    if(!bm||!P96Base)return;
    c->drawmem=(void *)p96GetBitMapAttr(bm,P96BMA_MEMORY);
    c->bprow=p96GetBitMapAttr(bm,P96BMA_BYTESPERROW);
    c->depth=p96GetBitMapAttr(bm,P96BMA_BYTESPERPIXEL)*8;   /* P96 says 24 for 4-byte pixels */
    switch(p96GetBitMapAttr(bm,P96BMA_RGBFORMAT)){
    case RGBFB_A8B8G8R8: c->format=W3D_FMT_A8B8G8R8;break;
    case RGBFB_R8G8B8A8: c->format=W3D_FMT_R8G8B8A8;break;
    case RGBFB_B8G8R8A8: c->format=W3D_FMT_B8G8R8A8;break;
    case RGBFB_R5G6B5:   c->format=W3D_FMT_R5G6B5;c->depth=16;break;
    case RGBFB_R5G6B5PC: c->format=W3D_FMT_R5G6B5PC;c->depth=16;break;
    case RGBFB_B5G6R5PC: c->format=W3D_FMT_B5G6R5PC;c->depth=16;break;
    case RGBFB_R5G5B5:   c->format=W3D_FMT_R5G5B5;c->depth=15;break;
    case RGBFB_R5G5B5PC: c->format=W3D_FMT_R5G5B5PC;c->depth=15;break;
    case RGBFB_B5G5R5PC: c->format=W3D_FMT_B5G5R5PC;c->depth=15;break;
    default:             c->format=W3D_FMT_A8R8G8B8;break;
    }
    c->supportedfmt=W3D_FMT_A8R8G8B8|W3D_FMT_A8B8G8R8|W3D_FMT_R8G8B8A8|W3D_FMT_B8G8R8A8|
                    W3D_FMT_R5G6B5|W3D_FMT_R5G6B5PC|W3D_FMT_B5G6R5PC|
                    W3D_FMT_R5G5B5|W3D_FMT_R5G5B5PC|W3D_FMT_B5G5R5PC;
    c->chunky=W3D_FALSE;
    c->destalpha=W3D_FALSE;
    c->drivertype=W3D_DRIVER_3DHW;
    if(MC)MiniGLDispatch->GLGetIntegerv(MC,GL_MAX_TEXTURE_SIZE,&limit);
    c->maxtexwidth=c->maxtexheight=c->maxtexwidthp=c->maxtexheightp=limit>0?limit:256;
    target=c->drawregion;target_yoffset=c->yoffset;
}
