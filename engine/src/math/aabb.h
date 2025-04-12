#pragma once


#include "vec3.h"

MINLINE aabb aabb_create(vec3 min, vec3 max) {
    min = vec3_min(min, max);
    max = vec3_max(min, max);

    return (aabb){
        min.x, min.y, min.z,
        max.x, max.y, max.z
    };
}

MINLINE void aabb_resolve(aabb* a) {
    vec3 min = vec3_min(a->min, a->max);
    vec3 max = vec3_max(a->min, a->max);

    a->min = min;
    a->max = max;
}

MINLINE aabb aabb_move(aabb a, vec3 delta) {
    vec3 min = vec3_add(a.min, delta);
    vec3 max = vec3_add(a.max, delta);

    return aabb_create(min, max);
}

MINLINE void aabb_move_ip(aabb* a, vec3 delta) {
    a->min = vec3_add(a->min, delta);
    a->max = vec3_add(a->max, delta);
}

MINLINE vec3 aabb_get_size(aabb a) {
    return vec3_sub(a.max, a.min);
}

MINLINE vec3 aabb_get_half_size(aabb a) {
    return vec3_mul_scalar(aabb_get_size(a), 0.5f);
}

MINLINE vec3 aabb_get_center(aabb a) {;
    return vec3_add(a.min, aabb_get_half_size(a));
}

MINLINE void aabb_set_size(aabb* a, vec3 size) {
    vec3 half_size = vec3_mul_scalar(size, 0.5f);

    vec3 center = aabb_get_center(*a);

    a->min = vec3_sub(center, half_size);
    a->max = vec3_add(center, half_size);
}

MINLINE void aabb_set_center(aabb* a, vec3 center) {
    vec3 half_size = vec3_mul_scalar(aabb_get_size(*a), 0.5f);

    a->min = vec3_sub(center, half_size);
    a->max = vec3_add(center, half_size);
}

MINLINE void aabb_set_center_size(aabb* a, vec3 center, vec3 size) {
    vec3 half_size = vec3_mul_scalar(size, 0.5f);

    a->min = vec3_sub(center, half_size);
    a->max = vec3_add(center, half_size);
}

MINLINE b8 aabb_contains_point(aabb a, vec3 p) {
    return p.x >= a.min.x && p.x <= a.max.x &&
           p.y >= a.min.y && p.y <= a.max.y &&
           p.z >= a.min.z && p.z <= a.max.z;
}

MINLINE b8 aabb_contains_aabb(aabb a, aabb b) {
    return aabb_contains_point(a, b.min) &&
           aabb_contains_point(a, b.max);
}

MINLINE aabb aabb_union(aabb a, aabb b) {
    vec3 min = vec3_min(a.min, b.min);
    vec3 max = vec3_max(a.max, b.max);

    return (aabb){
        min.x, min.y, min.z,
        max.x, max.y, max.z
    };
}
