#ifndef __DL_ST77355_FONTS_H__
#define __DL_ST77355_FONTS_H__

#include <stdint.h>

typedef struct
{
    const uint8_t width;
    uint8_t height;
    const uint16_t *data;
} FontDef;


extern FontDef Font_7x10;
extern FontDef Font_11x18;
extern FontDef Font_16x26;

#endif
