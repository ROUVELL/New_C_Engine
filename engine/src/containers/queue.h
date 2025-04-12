#pragma once

#include "defines.h"

typedef struct queue {
    u32 stride;
    u32 count;
    u32 allocated;
    void* memory;
} queue;

MAPI b8 queue_create(u32 stride, queue* out_queue);

MAPI void queue_destroy(queue* q);

MAPI void queue_reserve(queue* q, u32 count);

MAPI b8 queue_push(queue* q, void* data);

MAPI b8 queue_peek(queue* q, void* out_data);

MAPI b8 queue_pop(queue* q, void* out_data);