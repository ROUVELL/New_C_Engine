#include "queue.h"

#include "memory/memory.h"
#include "core/logger.h"

static void queue_ensure_allocated(queue* q, u32 count) {
    if (q->allocated < q->stride * count) {
        void* tmp = memory_allocate(count * q->stride, MEMORY_TAG_QUEUE);
        if (q->memory) {
            memory_copy(tmp, q->memory, q->allocated);
            memory_free(q->memory, q->allocated, MEMORY_TAG_QUEUE);
        }
        q->memory = tmp;
        q->allocated = count * q->stride;
    }
}

b8 queue_create(u32 stride, queue* out_queue) {
    if (!out_queue) {
        MERROR("queue_create requires a pointer to a valid queue!");
        return false;
    }

    memory_zero(out_queue, sizeof(queue));
    out_queue->stride = stride;
    out_queue->count = 0;
    queue_ensure_allocated(out_queue, 1);
    return true;
}

void queue_destroy(queue* q) {
    if (q) {
        if (q->memory) {
            memory_free(q->memory, q->allocated, MEMORY_TAG_QUEUE);
        }
        memory_zero(q, sizeof(queue));
    }
}

void queue_reserve(queue* q, u32 count) {
    if (q) {
        queue_ensure_allocated(q, count);
    }
}

b8 queue_push(queue* q, void* data) {
    if (!q) {
        MERROR("queue_push requires a pointer to a valid queue!");
        return false;
    }

    queue_ensure_allocated(q, q->count + 1);
    memory_copy((void*)((u64*)q->memory + (q->count * q->stride)), data, q->stride);
    q->count++;
    return true;
}

b8 queue_peek(queue* q, void* out_data) {
    if (!q || !out_data) {
        MERROR("queue_peek requires a pointer to a valid queue and to hold element data output!");
        return false;
    }

    if (q->count < 1) {
        MWARN("queue_peek - Queue is empty!");
        return false;
    }

    memory_copy(out_data, q->memory, q->stride);
    return true;
}

b8 queue_pop(queue* q, void* out_data) {
    if (!q || !out_data) {
        MERROR("queue_pop requires a pointer to a valid queue and to hold element data output!");
        return false;
    }

    if (q->count < 1) {
        MWARN("queue_pop - Queue is empty!");
        return false;
    }

    memory_copy(out_data, q->memory, q->stride);

    memory_copy(q->memory, (void*)(((u64*)q->memory) + q->stride), q->stride * (q->count - 1));

    q->count--;

    return true;
}