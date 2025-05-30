#include "mtwister.h"

#define UPPER_MASK 0x80000000
#define LOWER_MASK 0x7fffffff
#define TEMPERING_MASK_B 0x9d2c5680
#define TEMPERING_MASK_C 0xefc60000


static void mtrand_seed(u64 seed, struct mtrand_state* out_generator) {
    out_generator->mt[0] = seed & 0xffffffff;
    for (out_generator->index = 1; out_generator->index < STATE_VECTOR_LENGTH; out_generator->index++) {
        out_generator->mt[out_generator->index] = (6069 * out_generator->mt[out_generator->index - 1]) & 0xffffffff;
    }
}


mtrand_state mtrand_create(u64 seed) {
    mtrand_state generator;
    mtrand_seed(seed, &generator);
    return generator;
}

u64 mtrand_generate_u64(mtrand_state* generator) {
    u64 result;
    static u64 mag[2] = { 0x0, 0x9908b0df };
    if (generator->index >= STATE_VECTOR_LENGTH || generator->index < 0) {
        if (generator->index >= STATE_VECTOR_LENGTH + 1 || generator->index < 0) {
            mtrand_seed(4357, generator);
        }

        i32 key;
        for (key = 0; key < STATE_VECTOR_LENGTH - STATE_VECTOR_M; key++) {
            result = (generator->mt[key] & UPPER_MASK) | (generator->mt[key + 1] & LOWER_MASK);
            generator->mt[key] = generator->mt[key + STATE_VECTOR_M] ^ (result >> 1) ^ mag[result & 0x1];
        }

        for (; key < STATE_VECTOR_LENGTH - 1; key++) {
            result = (generator->mt[key] & UPPER_MASK) | (generator->mt[key + 1] & LOWER_MASK);
            generator->mt[key] = generator->mt[key + (STATE_VECTOR_M - STATE_VECTOR_LENGTH)] ^ (result >> 1) ^ mag[result & 0x1];
        }

        result = (generator->mt[STATE_VECTOR_LENGTH - 1] & UPPER_MASK) | (generator->mt[0] & LOWER_MASK);
        generator->mt[STATE_VECTOR_LENGTH - 1] = generator->mt[STATE_VECTOR_M - 1] ^ (result >> 1) ^ mag[result & 0x1];
        generator->index = 0;
    }

    result = generator->mt[generator->index++];
    result ^= (result >> 11);
    result ^= (result << 7) & TEMPERING_MASK_B;
    result ^= (result << 15) & TEMPERING_MASK_C;
    result ^= (result >> 18);

    return result;
}

f64 mtrand_generate_f64(mtrand_state* generator) {
    return ((f64)mtrand_generate_u64(generator) / (u64)0xffffffff);
}