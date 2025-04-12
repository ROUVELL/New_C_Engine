#pragma once

#include "defines.h"

#define STATE_VECTOR_LENGTH 624
#define STATE_VECTOR_M 397

typedef struct mtrand_state {
    u64 mt[STATE_VECTOR_LENGTH];
    i32 index;
} mtrand_state;

MAPI mtrand_state mtrand_create(u64 seed);

MAPI u64 mtrand_generate_u64(mtrand_state* generator);

MAPI f64 mtrand_generate_f64(mtrand_state* generator);