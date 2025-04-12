#pragma once

#include "defines.h"


typedef struct mutex {
    void* internal_data;
} mutex;

MAPI b8 mutex_create(mutex* out_mutex);

MAPI void mutex_destroy(mutex* m);

MAPI b8 mutex_lock(mutex* m);

MAPI b8 mutex_unlock(mutex* m);