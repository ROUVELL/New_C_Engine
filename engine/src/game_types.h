#pragma once

#include "core/engine.h"

typedef struct game {
    engine_config config;

    b8 (*initialize)(struct game* instance);
    
    b8 (*update)(struct game* instance, f32 dt);
    
    b8 (*render)(struct game* instance, f32 dt);
    
    void (*on_resize)(struct game* instance, u32 width, u32 height);

    void* game_state;
} game;