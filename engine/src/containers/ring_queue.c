#include "ring_queue.h"

#include "memory/memory.h"
#include "core/logger.h"

b8 ring_queue_create(u32 stride, u32 capacity, void* memory, ring_queue* out_queue) {
    if (!out_queue) {
        MERROR("ring_queue_create requires a valid pointer to hold the queue!");
        return false;
    }

    out_queue->length = 0;
    out_queue->capacity = capacity;
    out_queue->stride = stride;
    out_queue->head = 0;
    out_queue->tail = -1;
    if (memory) {
        out_queue->owns_memory = false;
        out_queue->block = memory;
    } else {
        out_queue->owns_memory = true;
        out_queue->block = memory_allocate(capacity * stride, MEMORY_TAG_RING_QUEUE);
    }

    return true;
}

void ring_queue_destroy(ring_queue* q) {
    if (q) {
        if (q->owns_memory) {
            memory_free(q->block, q->capacity * q->stride, MEMORY_TAG_RING_QUEUE);
        }
        memory_zero(q, sizeof(ring_queue));
    }
}

b8 ring_queue_enqueue(ring_queue* q, void* value) {
    if (!q || !value) {
        MERROR("ring_queue_enqueue requires valid pointers to queue and value!");
        return false;
    }

    if (q->length == q->capacity) {
        MERROR("ring_queue_enqueue - Attempted to enqueue value in full ring queue: %p", q);
        return false;
    }

    q->tail = (q->tail + 1) % q->capacity;

    memory_copy(q->block + (q->tail * q->stride), value, q->stride);
    q->length++;
    
    return true;
}

b8 ring_queue_dequeue(ring_queue* q, void* out_value) {
    if (!q || !out_value) {
        MERROR("ring_queue_dequeue requires valid pointers to queue and out_value!");
        return false;
    }

    if (q->length == 0) {
        MERROR("ring_queue_dequeue - Attempted to dequeue value in empty ring queue: %p", q);
        return false;
    }

    memory_copy(out_value, q->block + (q->head * q->stride), q->stride);
    q->head = (q->head + 1) % q->capacity;
    q->length--;

    return true;
}

b8 ring_queue_peek(const ring_queue* q, void* out_value) {
    if (!q || !out_value) {
        MERROR("ring_queue_peek requires valid pointers to queue and out_value!");
        return false;
    }

    if (q->length == 0) {
        MERROR("ring_queue_peek - Attempted to dequeue value in empty ring queue: %p", q);
        return false;
    }

    memory_copy(out_value, q->block + (q->head * q->stride), q->stride);

    return true;
}