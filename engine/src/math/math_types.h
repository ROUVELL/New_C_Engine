#pragma once

#include "defines.h"

typedef union vec2_u {
    f32 data[2];
    struct {
        union {
            f32 x, r, s, u, width;
        };
        union {
            f32 y, g, t, v, height;
        };
    };
} vec2;

typedef union vec3_u {
    f32 data[3];
    struct {
        union {
            f32 x, r, s, u;
        };
        union {
            f32 y, g, t, v;
        };
        union {
            f32 z, b, p, w;
        };
    };
} vec3;

typedef union vec4_u {
    f32 data[4];
    struct {
        union {
            f32 x, r, s;
        };
        union {
            f32 y, g, t;
        };
        union {
            f32 z, b, p, width;
        };
        union {
            f32 w, a, q, height;
        };
    };
} vec4;

typedef vec4 quat;

typedef union mat3_u {
    f32 data[9];
    vec3 rows[3];
} mat3;

typedef union mat4_u {
    f32 data[16];
    vec4 rows[4];
} mat4;

typedef union aabb_u {
    f32 data[6];
    struct {
        vec3 min;
        vec3 max;
    };
} aabb;