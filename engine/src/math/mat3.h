#pragma once

#include "math.h"
#include "math_types.h"


MINLINE mat3 mat3_zero() {
    return (mat3){
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f
    };
}

MINLINE mat3 mat3_identity() {
    return (mat3){
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f
    };
}