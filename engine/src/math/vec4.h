#pragma once

#include "math.h"
#include "math_types.h"

MINLINE vec4 vec4_create(f32 x, f32 y, f32 z, f32 w) {
    return (vec4){x, y, z, w};
}

MINLINE vec3 vec4_to_vec3(vec4 v) {
    return (vec3){v.x, v.y, v.z};
}

MINLINE vec2 vec4_to_vec2(vec4 v) {
    return (vec2){v.x, v.y};
}

MINLINE vec4 vec4_zero() {
    return (vec4){0.0f, 0.0f, 0.0f, 0.0f};
}

MINLINE vec4 vec4_one() {
    return (vec4){1.0f, 1.0f, 1.0f, 1.0f};
}

MINLINE vec4 vec4_add(vec4 v1, vec4 v2) {
    return (vec4){
        v1.x + v2.x,
        v1.y + v2.y,
        v1.z + v2.z,
        v1.w + v2.w
    };
}

MINLINE vec4 vec4_sub(vec4 v1, vec4 v2) {
    return (vec4){
        v1.x - v2.x,
        v1.y - v2.y,
        v1.z - v2.z,
        v1.w - v2.w
    };
}

MINLINE vec4 vec4_mul(vec4 v1, vec4 v2) {
    return (vec4){
        v1.x * v2.x,
        v1.y * v2.y,
        v1.z * v2.z,
        v1.w * v2.w
    };
}

MINLINE vec4 vec4_div(vec4 v1, vec4 v2) {
    return (vec4){
        v1.x / v2.x,
        v1.y / v2.y,
        v1.z / v2.z,
        v1.w / v2.w
    };
}

MINLINE vec4 vec4_mul_scalar(vec4 v, f32 s) {
    return (vec4){
        s * v.x,
        s * v.y,
        s * v.z,
        s * v.w
    };
}

MINLINE vec4 vec4_mul_mat4(vec4 v, mat4 m) {
    return (vec4){
        v.x * m.data[0] + v.y * m.data[4] + v.z * m.data[8] + v.w * m.data[12],
        v.x * m.data[1] + v.y * m.data[5] + v.z * m.data[9] + v.w * m.data[13],
        v.x * m.data[2] + v.y * m.data[6] + v.z * m.data[10] + v.w * m.data[14],
        v.x * m.data[3] + v.y * m.data[7] + v.z * m.data[11] + v.w * m.data[15]
    };
}

MINLINE vec4 vec4_div_scalar(vec4 v, f32 s) {
    return (vec4){
        v.x / s,
        v.y / s,
        v.z / s,
        v.w / s
    };
}

MINLINE f32 vec4_length_squared(vec4 v) {
    return v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w;
}

MINLINE f32 vec4_length(vec4 v) {
    return msqrt(vec4_length_squared(v));
}

MINLINE void vec4_normalize(vec4 *v) {
    const f32 length = vec4_length(*v);
    v->x /= length;
    v->y /= length;
    v->z /= length;
    v->w /= length;
}

MINLINE vec4 vec4_normalized(vec4 v) {
    vec4_normalize(&v);
    return v;
}

MINLINE f32 vec4_dot_f32(
    f32 a1, f32 a2, f32 a3, f32 a4,
    f32 b1, f32 b2, f32 b3, f32 b4
) {
    return a1 * b1 + a2 * b2 + a3 * b3 + a4 * b4;
}