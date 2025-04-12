#pragma once

#include "defines.h"

struct game;

typedef struct engine_config {
    const char* app_name;
    u32 window_width;
    u32 window_height;
} engine_config;

MAPI b8 engine_initialize(struct game* game_instance);
MAPI b8 engine_run();