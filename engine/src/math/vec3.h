#pragma once

#include "math.h"
#include "math_types.h"

MINLINE vec3 vec3_create(f32 x, f32 y, f32 z) {
    return (vec3){x, y, z};
}

MINLINE vec2 vec3_to_vec2(vec3 v) {
    return (vec2){v.x, v.y};
}

MINLINE vec4 vec3_to_vec4(vec3 v, f32 w) {
    return (vec4){v.x, v.y, v.z, w};
}

MINLINE vec3 vec3_zero() {
    return (vec3){0.0f, 0.0f, 0.0f};
}

MINLINE vec3 vec3_one() {
    return (vec3){1.0f, 1.0f, 1.0f};
}

MINLINE vec3 vec3_up() {
    return (vec3){0.0f, 1.0f, 0.0f};
}

MINLINE vec3 vec3_down() {
    return (vec3){0.0f, -1.0f, 0.0f};
}

MINLINE vec3 vec3_left() {
    return (vec3){-1.0f, 0.0f, 0.0f};
}

MINLINE vec3 vec3_right() {
    return (vec3){1.0f, 0.0f, 0.0f};
}

MINLINE vec3 vec3_forward() {
    return (vec3){0.0f, 0.0f, -1.0f};
}

MINLINE vec3 vec3_backward() {
    return (vec3){0.0f, 0.0f, 1.0f};
}

MINLINE vec3 vec3_add(vec3 v1, vec3 v2) {
    return (vec3){
        v1.x + v2.x,
        v1.y + v2.y,
        v1.z + v2.z,
    };
}

MINLINE vec3 vec3_add_scalar(vec3 v, f32 s) {
    return (vec3){
        v.x + s,
        v.y + s,
        v.z + s,
    };
}

MINLINE vec3 vec3_sub(vec3 v1, vec3 v2) {
    return (vec3){
        v1.x - v2.x,
        v1.y - v2.y,
        v1.z - v2.z,
    };
}

MINLINE vec3 vec3_sub_scalar(vec3 v, f32 s) {
    return (vec3){
        v.x - s,
        v.y - s,
        v.z - s,
    };
}

MINLINE vec3 vec3_mul(vec3 v1, vec3 v2) {
    return (vec3){
        v1.x * v2.x,
        v1.y * v2.y,
        v1.z * v2.z,
    };
}

MINLINE vec3 vec3_mul_scalar(vec3 v, f32 s) {
    return (vec3){
        v.x * s,
        v.y * s,
        v.z * s,
    };
}

MINLINE vec3 vec3_mul_mat4(vec3 v, mat4 m) {
    return (vec3){
        v.x * m.data[0] + v.y * m.data[4] + v.z * m.data[8] + m.data[12],
        v.x * m.data[1] + v.y * m.data[5] + v.z * m.data[9] + m.data[13],
        v.x * m.data[2] + v.y * m.data[6] + v.z * m.data[10] + m.data[14]
    };
}

MINLINE vec3 vec3_div_mat4(vec3 v, mat4 m) {
    return (vec3){
        v.x / m.data[0] + v.y / m.data[4] + v.z / m.data[8],
        v.x / m.data[1]
    };
}

MINLINE vec3 vec3_div(vec3 v1, vec3 v2) {
    return (vec3){
        v1.x / v2.x,
        v1.y / v2.y,
        v1.z / v2.z,
    };
}

MINLINE vec3 vec3_div_scalar(vec3 v, f32 s) {
    return (vec3){
        v.x / s,
        v.y / s,
        v.z / s,
    };
}

MINLINE f32 vec3_length_squared(vec3 v) {
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

MINLINE f32 vec3_length(vec3 v) {
    return msqrt(vec3_length_squared(v));
}

MINLINE void vec3_normalize(vec3 *v) {
    const f32 length = vec3_length(*v);
    v->x /= length;
    v->y /= length;
    v->z /= length;
}

MINLINE vec3 vec3_normalized(vec3 v) {
    vec3_normalize(&v);
    return v;
}

MINLINE vec3 vec3_cross(vec3 v1, vec3 v2) {
    return (vec3){
        v1.y * v2.z - v1.z * v2.y,
        v1.z * v2.x - v1.x * v2.z,
        v1.x * v2.y - v1.y * v2.x,
    };
}

MINLINE f32 vec3_dot(vec3 v1, vec3 v2) {
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

MINLINE b8 vec3_compare(vec3 v1, vec3 v2, f32 tolerance) {
    if (mabs(v1.x - v2.x) > tolerance) {
        return false;
    }

    if (mabs(v1.y - v2.y) > tolerance) {
        return false;
    }

    if (mabs(v1.z - v2.z) > tolerance) {
        return false;
    }

    return true;
} 

MINLINE f32 vec3_distance(vec3 v1, vec3 v2) {
    return vec3_length(vec3_sub(v1, v2));
}

MINLINE vec3 vec3_lerp(vec3 v1, vec3 v2, f32 t) {
    return vec3_add(vec3_mul_scalar(v1, 1.0f - t), vec3_mul_scalar(v2, t));
}

MINLINE vec3 vec3_min(vec3 v1, vec3 v2) {
    return (vec3){
        MMIN(v1.x, v2.x),
        MMIN(v1.y, v2.y),
        MMIN(v1.z, v2.z),
    };
}

MINLINE vec3 vec3_max(vec3 v1, vec3 v2) {
    return (vec3){
        MMAX(v1.x, v2.x),
        MMAX(v1.y, v2.y),
        MMAX(v1.z, v2.z),
    };
}

MINLINE vec3 vec3_clamp(vec3 v, vec3 min, vec3 max) {
    return (vec3){
        MCLAMP(v.x, min.x, max.x),
        MCLAMP(v.y, min.y, max.y),
        MCLAMP(v.z, min.z, max.z)
    };
}

MINLINE vec3 vec3_inverse(vec3 v) {
    return (vec3){-v.x, -v.y, -v.z};
}


