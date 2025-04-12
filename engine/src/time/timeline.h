#pragma once

#include "defines.h"


typedef struct timeline {
    f64 total;
    f32 dt;
    // 1.0 - normal (default), 0.0 - pause, -1.0 - reverse time
    f32 scale;
} timeline;

b8 timeline_system_initialize();
void timeline_system_shutdown();
void timeline_system_update(f32 dt);

MAPI timeline* timeline_create(f32 scale);

MAPI void timeline_destroy(timeline* t);