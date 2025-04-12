#pragma once

#include "math.h"
#include "math_types.h"


MINLINE vec2 vec2_create(f32 x, f32 y) {
    return (vec2){x, y};
}

MINLINE vec3 vec2_to_vec3(vec2 v, f32 z) {
    return (vec3){v.x, v.y, z};
}

MINLINE vec4 vec2_to_vec4(vec2 v, f32 z, f32 w) {
    return (vec4){v.x, v.y, z, w};
}

MINLINE vec2 vec2_zero() {
    return (vec2){0.0f, 0.0f};
}

MINLINE vec2 vec2_one() {
    return (vec2){1.0f, 1.0f};
}

MINLINE vec2 vec2_up() {
    return (vec2){0.0f, 1.0f};
}

MINLINE vec2 vec2_down() {
    return (vec2){0.0f, -1.0f};
}

MINLINE vec2 vec2_left() {
    return (vec2){-1.0f, 0.0f};
}

MINLINE vec2 vec2_right() {
    return (vec2){1.0f, 0.0f};
}

MINLINE vec2 vec2_add(vec2 v1, vec2 v2) {
    return (vec2){
        v1.x + v2.x,
        v1.y + v2.y,
    };
}

MINLINE vec2 vec2_add_scalar(vec2 v, f32 s) {
    return (vec2){
        v.x + s,
        v.y + s,
    };
}

MINLINE vec2 vec2_sub(vec2 v1, vec2 v2) {
    return (vec2){
        v1.x - v2.x,
        v1.y - v2.y,
    };
}

MINLINE vec2 vec2_sub_scalar(vec2 v, f32 s) {
    return (vec2){
        v.x - s,
        v.y - s,
    };
}

MINLINE vec2 vec2_mul(vec2 v1, vec2 v2) {
    return (vec2){
        v1.x * v2.x,
        v1.y * v2.y,
    };
}

MINLINE vec2 vec2_mul_scalar(vec2 v, f32 s) {
    return (vec2){
        v.x * s,
        v.y * s,
    };
}

MINLINE vec2 vec2_div(vec2 v1, vec2 v2) {
    return (vec2){
        v1.x / v2.x,
        v1.y / v2.y,
    };
}

MINLINE vec2 vec2_div_scalar(vec2 v, f32 s) {
    return (vec2){
        v.x / s,
        v.y / s,
    };
}

MINLINE f32 vec2_length_squared(vec2 v) {
    return v.x * v.x + v.y * v.y;
}

MINLINE f32 vec2_length(vec2 v) {
    return msqrt(vec2_length_squared(v));
}

MINLINE void vec2_normalize(vec2* v) {
    const f32 length = vec2_length(*v);
    v->x /= length;
    v->y /= length;
}

MINLINE vec2 vec2_normalized(vec2 v) {
    const f32 length = vec2_length(v);
    return (vec2){
        v.x / length,
        v.y / length,
    };
}

MINLINE b8 vec2_compare(vec2 v1, vec2 v2, f32 tolerance) {
    if (mabs(v1.x - v2.x) > tolerance) {
        return false;
    }
    if (mabs(v1.y - v2.y) > tolerance) {
        return false;
    }
    return true;
}

MINLINE f32 vec2_distance(vec2 v1, vec2 v2) {
    return vec2_length(vec2_sub(v1, v2));
}

MINLINE vec2 vec2_lerp(vec2 v1, vec2 v2, f32 t) {
    return vec2_add(vec2_mul_scalar(v1, 1.0f - t), vec2_mul_scalar(v2, t));
}

MINLINE vec2 vec2_min(vec2 v1, vec2 v2) {
    return (vec2){
        MMIN(v1.x, v2.x),
        MMIN(v1.y, v2.y),
    };
}
MINLINE vec2 vec2_max(vec2 v1, vec2 v2) {
    return (vec2){
        MMAX(v1.x, v2.x),
        MMAX(v1.y, v2.y),
    };
}

MINLINE vec2 vec2_clamp(vec2 v, vec2 min, vec2 max) {
    return (vec2){
        MCLAMP(v.x, min.x, max.x),
        MCLAMP(v.y, min.y, max.y)
    };
}

MINLINE vec2 vec2_abs(vec2 v) {
    return (vec2){
        mabs(v.x),
        mabs(v.y),
    };
}
