#include "w3d.h"
#include <string.h>
#include "../include/pixels.h"
/* Our own registry of live textures. W3D_Texture.link is NOT used for it:
 * MiniGL builds keep their own lists through that node (glhexen2 does), which
 * left freed textures in a list kept there. Some programs also pass stale or
 * garbage texture pointers; only registered ones are used, anything else is
 * treated as no texture. The last match is cached: draws repeat the same one. */
static W3D_Texture **live, *last_valid;
static int live_count, live_capacity;
W3D_Texture *wm_valid_texture(W3D_Texture *t){
    int i;
    if(!t||t==last_valid)return t;
    for(i=live_count-1;i>=0;--i)if(live[i]==t){last_valid=t;return t;}
    return NULL;
}
static int register_texture(W3D_Texture *t){
    if(live_count==live_capacity){
        int capacity=live_capacity?live_capacity*2:256;
        W3D_Texture **grown=malloc(capacity*sizeof(*grown));
        if(!grown)return 0;
        if(live){memcpy(grown,live,live_count*sizeof(*grown));free(live);}
        live=grown;live_capacity=capacity;
    }
    live[live_count++]=t;return 1;
}
static void unregister_texture(W3D_Texture *t){
    int i;
    for(i=0;i<live_count;++i)if(live[i]==t){live[i]=live[--live_count];break;}
    if(t==last_valid)last_valid=NULL;
}
static const GLenum filter[]={0,GL_NEAREST,GL_LINEAR,GL_NEAREST_MIPMAP_NEAREST,GL_NEAREST_MIPMAP_LINEAR,GL_LINEAR_MIPMAP_NEAREST,GL_LINEAR_MIPMAP_LINEAR};
static ULONG upload(W3D_Texture *tex,const void *source,ULONG level,const ULONG *palette,int x,int y,int w,int h,ULONG stride,int sub){
    unsigned char *rgba;int row;unsigned bpp=wm_pixel_size(tex->texfmtsrc);
    if(level>15||w<=0||h<=0||!bpp||w>8191||h>0x7fffffff/(w*4))return W3D_ILLEGALINPUT;
    if(!stride)stride=w*bpp;
    if(stride<(ULONG)w*bpp)return W3D_ILLEGALINPUT;
    rgba=malloc(w*h*4);if(!rgba)return W3D_NOMEMORY;
    for(row=0;row<h;++row)if(!wm_convert(rgba+row*w*4,(const UBYTE *)source+(ULONG)row*stride,tex->texfmtsrc,w,palette)){free(rgba);return W3D_UNSUPPORTEDTEXFMT;}
    _glBindTexture(GL_TEXTURE_2D,((Texture *)tex->driver)->glID);
    if(sub)_glTexSubImage2D(GL_TEXTURE_2D,level,x,y,w,h,GL_RGBA,GL_UNSIGNED_BYTE,rgba);
    else _glTexImage2D(GL_TEXTURE_2D,level,GL_RGBA,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,rgba);
    free(rgba);return W3D_SUCCESS;
}
W3D_Texture *W3D_AllocTexObj(__REGA0(W3D_Context *context),__REGA1(ULONG *error),__REGA2(struct TagItem *tags)){
    W3D_Texture *tex;Texture *data;ULONG result;
    if(error)*error=W3D_NOMEMORY;
    tex=malloc(sizeof(*tex));data=malloc(sizeof(*data));
    if(!tex||!data){free(tex);free(data);return NULL;}
    memset(tex,0,sizeof(*tex));memset(data,0,sizeof(*data));tex->driver=data;
    for(;tags&&tags->ti_Tag!=TAG_DONE;++tags)switch(tags->ti_Tag){
    case W3D_ATO_IMAGE:tex->texsource=(void *)tags->ti_Data;break;
    case W3D_ATO_FORMAT:tex->texfmtsrc=tags->ti_Data;break;
    case W3D_ATO_WIDTH:tex->texwidth=tags->ti_Data;break;
    case W3D_ATO_HEIGHT:tex->texheight=tags->ti_Data;break;
    case W3D_ATO_PALETTE:tex->palette=(void *)tags->ti_Data;break;
    }
    wm_trace("AllocTexObj",tex->texfmtsrc,tex->texwidth,tex->texheight);
    if(!tex->texsource||!wm_pixel_size(tex->texfmtsrc)){if(error)*error=W3D_UNSUPPORTEDTEXFMT;free(tex);free(data);return NULL;}
    _glGenTextures(1,&data->glID);_glBindTexture(GL_TEXTURE_2D,data->glID);
    /* Warp3D: a new texture is point-sampled until W3D_SetFilter. */
    _glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);_glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    _glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);_glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    result=upload(tex,tex->texsource,0,tex->palette,0,0,tex->texwidth,tex->texheight,0,0);
    if(result!=W3D_SUCCESS){_glDeleteTextures(1,&data->glID);free(data);free(tex);if(error)*error=result;return NULL;}
    if(!register_texture(tex)){_glDeleteTextures(1,&data->glID);free(data);free(tex);if(error)*error=W3D_NOMEMORY;return NULL;}
    tex->resident=W3D_TRUE;if(error)*error=W3D_SUCCESS;return tex;
}
void W3D_FreeTexObj(__REGA0(W3D_Context *context),__REGA1(W3D_Texture *texture)){
    if(!wm_valid_texture(texture))return;
    unregister_texture(texture);
    if(context&&context->CurrentTex[0]==texture)context->CurrentTex[0]=NULL;
    _glDeleteTextures(1,&((Texture *)texture->driver)->glID);free(texture->driver);free(texture);
}
void wm_free_textures(W3D_Context *context){while(live_count)W3D_FreeTexObj(context,live[live_count-1]);}
ULONG W3D_FreeAllTexObj(__REGA0(W3D_Context *context)){
    extern void qt_debug(const char *);
    qt_debug("[WarpMiniGL exit] FreeAllTexObj: begin\n");
    wm_free_textures(context);
    qt_debug("[WarpMiniGL exit] FreeAllTexObj: done\n");
    return W3D_SUCCESS;
}
void W3D_ReleaseTexture(__REGA0(W3D_Context *context),__REGA1(W3D_Texture *texture)){}
void W3D_FlushTextures(__REGA0(W3D_Context *context)){}
ULONG W3D_SetFilter(__REGA0(W3D_Context *context),__REGA1(W3D_Texture *texture),__REGD0(ULONG min),__REGD1(ULONG mag)){
    if(!wm_valid_texture(texture)||min<1||min>6||mag<1||mag>2)return W3D_UNSUPPORTEDFILTER;
    bindTexture(texture);_glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,filter[min]);_glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,filter[mag]);return W3D_SUCCESS;
}
ULONG W3D_SetTexEnv(__REGA0(W3D_Context *context),__REGA1(W3D_Texture *texture),__REGD1(ULONG env),__REGA2(W3D_Color *color)){
    GLenum mode;wm_trace("SetTexEnv",(ULONG)texture,env,0);if(env==W3D_REPLACE)mode=GL_REPLACE;else if(env==W3D_DECAL)mode=GL_DECAL;else if(env==W3D_MODULATE)mode=GL_MODULATE;else return W3D_UNSUPPORTEDTEXENV;
    if(wm_valid_texture(texture))((Texture *)texture->driver)->envparam=mode;
    _glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,mode);return W3D_SUCCESS;
}
ULONG W3D_SetWrapMode(__REGA0(W3D_Context *context),__REGA1(W3D_Texture *texture),__REGD0(ULONG s),__REGD1(ULONG t),__REGA2(W3D_Color *color)){
    if(!wm_valid_texture(texture)||s<1||s>2||t<1||t>2)return W3D_UNSUPPORTEDWRAPMODE;
    bindTexture(texture);_glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,s==1?GL_REPEAT:GL_CLAMP);_glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,t==1?GL_REPEAT:GL_CLAMP);return W3D_SUCCESS;
}
ULONG W3D_UpdateTexImage(__REGA0(W3D_Context *context),__REGA1(W3D_Texture *texture),__REGA2(void *image),__REGD1(int level),__REGA3(ULONG *palette)){
    int w,h;if(!wm_valid_texture(texture)||!image||level<0||level>15)return W3D_ILLEGALINPUT;
    w=texture->texwidth>>level;h=texture->texheight>>level;if(!w)w=1;if(!h)h=1;
    if(level==0)texture->texsource=image;
    return upload(texture,image,level,palette?palette:texture->palette,0,0,w,h,0,0);
}
ULONG W3D_UpdateTexSubImage(__REGA0(W3D_Context *context),__REGA1(W3D_Texture *texture),__REGA2(void *image),__REGD1(ULONG level),__REGA3(ULONG *palette),__REGA4(W3D_Scissor *rect),__REGD0(ULONG stride)){
    if(!wm_valid_texture(texture)||!image||!rect)return W3D_ILLEGALINPUT;
    return upload(texture,image,level,palette?palette:texture->palette,rect->left,rect->top,rect->width,rect->height,stride,1);
}
ULONG W3D_UploadTexture(__REGA0(W3D_Context *context),__REGA1(W3D_Texture *texture)){if(!texture)return W3D_ILLEGALINPUT;return W3D_SUCCESS;}
ULONG W3D_SetChromaTestBounds(__REGA0(W3D_Context *context),__REGA1(W3D_Texture *texture),__REGD0(ULONG low),__REGD1(ULONG high),__REGD2(ULONG mode)){return W3D_UNSUPPORTED;}
