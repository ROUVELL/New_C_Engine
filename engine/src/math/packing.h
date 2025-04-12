#pragma once

#include "math_types.h"


// Colors

MINLINE u32 rgbau8_to_u32(u8 r, u8 g, u8 b, u8 a) {
    return ((u32)r << 24) | ((u32)g << 16) | ((u32)b << 8) | (u32)a;
}

MINLINE u32 rgbau32_to_u32(u32 r, u32 g, u32 b, u32 a) {
    return ((r & 0xFF) << 24) | ((g & 0xFF) << 16) | ((b & 0xFF) << 8) | (a & 0xFF);
}


MINLINE vec4 u32_to_vec4(u32 value) {
    return (vec4){
        (value >> 24) & 0xFF,
        (value >> 16) & 0xFF,
        (value >> 8) & 0xFF,
        value & 0xFF
    };
}