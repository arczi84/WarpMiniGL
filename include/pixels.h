#ifndef WM_PIXELS_H
#define WM_PIXELS_H
#include <stddef.h>
/* Warp3D source formats, converted to native byte RGBA for MiniGL. */
static unsigned wm_pixel_size(unsigned f){static const unsigned sizes[]={0,1,2,2,3,2,4,1,1,2,1,4};return f<12?sizes[f]:0;}
static int wm_convert(unsigned char *out,const unsigned char *src,unsigned format,size_t count,const unsigned long *palette){
    size_t i;unsigned size=wm_pixel_size(format),r,g,b,a,v;
    if(!src||!out||!size||(format==1&&!palette))return 0;
    for(i=0;i<count;++i,src+=size,out+=4){
        r=g=b=0;a=255;
        switch(format){
        case 1:v=palette[*src];a=v>>24;r=(v>>16)&255;g=(v>>8)&255;b=v&255;break;
        case 2:v=(src[0]<<8)|src[1];a=(v&32768)?255:0;r=((v>>10)&31)*255/31;g=((v>>5)&31)*255/31;b=(v&31)*255/31;break;
        case 3:v=(src[0]<<8)|src[1];r=(v>>11)*255/31;g=((v>>5)&63)*255/63;b=(v&31)*255/31;break;
        case 4:r=src[0];g=src[1];b=src[2];break;
        case 5:v=(src[0]<<8)|src[1];a=(v>>12)*17;r=((v>>8)&15)*17;g=((v>>4)&15)*17;b=(v&15)*17;break;
        case 6:a=src[0];r=src[1];g=src[2];b=src[3];break;
        case 7:r=g=b=255;a=src[0];break;
        case 8:r=g=b=src[0];break;
        case 9:r=g=b=src[0];a=src[1];break;
        case 10:r=g=b=a=src[0];break;
        case 11:r=src[0];g=src[1];b=src[2];a=src[3];break;
        }
        out[0]=r;out[1]=g;out[2]=b;out[3]=a;
    }
    return 1;
}
#endif
