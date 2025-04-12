#include "random.h"

#include <stdlib.h>

#include "platform/platform.h"
#include "mtwister.h"


static b8 rand_seeded = false;
static mtrand_state rng_u64 = {0};

static void seed_randoms(void) {
    u32 ptime_u32;
    u32 ptime_u64;
#if ENGINE_DEBUG
    ptime_u32 = 42;
    ptime_u64 = 42;
#else
    ptime_u32 = (u32)platform_get_absolute_time();
    ptime_u64 = (u64)platform_get_absolute_time();
#endif

    srand(ptime_u32);

    rng_u64 = mtrand_create(ptime_u64);

    rand_seeded = true;
}

i32 random_i32(void) {
    if (!rand_seeded) {
        seed_randoms();
    }

    return rand();
}

i32 random_i32_in_range(i32 min, i32 max) {
    if (!rand_seeded) {
        seed_randoms();
    }

    return (rand() % (max - min + 1)) + min;
}

u64 random_u64(void) {
    if (!rand_seeded) {
        seed_randoms();
    }

    return mtrand_generate_u64(&rng_u64);
}

u64 random_u64_in_range(u64 min, u64 max) {
    return (random_u64() % (max - min + 1)) + min;
}

f32 random_f32(void) {
    return (f32)random_i32() / (f32)RAND_MAX;
}

f32 random_f32_in_range(f32 min, f32 max) {
    return min + ((f32)random_i32() / ((f32)RAND_MAX / (max - min)));
}