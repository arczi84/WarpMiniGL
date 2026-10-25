#include "../include/pixels.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void){
    unsigned char out[8];
    const unsigned char rgb[]={1,2,3,4,5,6},argb[]={128,10,20,30},rgb565[]={0xf8,0,0x07,0xe0},argb4444[]={0x8f,0x42};
    const unsigned char alpha[]={64},la[]={90,180},indexed[]={3};unsigned long palette[256]={0};
    assert(wm_convert(out,rgb,4,2,NULL));assert(!memcmp(out,(unsigned char[]){1,2,3,255,4,5,6,255},8));
    assert(wm_convert(out,argb,6,1,NULL));assert(!memcmp(out,(unsigned char[]){10,20,30,128},4));
    assert(wm_convert(out,rgb565,3,2,NULL));assert(!memcmp(out,(unsigned char[]){255,0,0,255,0,255,0,255},8));
    assert(wm_convert(out,argb4444,5,1,NULL));assert(!memcmp(out,(unsigned char[]){255,68,34,136},4));
    assert(wm_convert(out,alpha,7,1,NULL));assert(!memcmp(out,(unsigned char[]){255,255,255,64},4));
    assert(wm_convert(out,la,9,1,NULL));assert(!memcmp(out,(unsigned char[]){90,90,90,180},4));
    palette[3]=0x80402010;assert(wm_convert(out,indexed,1,1,palette));assert(!memcmp(out,(unsigned char[]){64,32,16,128},4));
    assert(!wm_convert(out,indexed,1,1,NULL));assert(!wm_convert(out,rgb,12,1,NULL));
    assert(!memcmp(rgb,(unsigned char[]){1,2,3,4,5,6},6));
    puts("PASS: RGB, ARGB, RGB565, ARGB4444, alpha, luminance-alpha, palette and input preservation");return 0;
}
