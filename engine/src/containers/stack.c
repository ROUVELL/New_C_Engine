#include "stack.h"

#include "memory/memory.h"
#include "core/logger.h"

static void stack_ensure_allocated(stack* s, u32 count) {
    if (s->allocated < s->stride * count) {
        void* tmp = memory_allocate(count * s->stride, MEMORY_TAG_STACK);
        if (s->memory) {
            memory_copy(tmp, s->memory, s->allocated);
            memory_free(s->memory, s->allocated, MEMORY_TAG_STACK);
        }
        s->memory = tmp;
        s->allocated = count * s->stride;
    }
}

b8 stack_create(u32 stride, stack* out_stack) {
    if (!out_stack) {
        MERROR("stack_create resuires a pointer to a valid stack!");
        return false;
    }

    memory_zero(out_stack, sizeof(stack));
    out_stack->stride = stride;
    out_stack->count = 0;
    stack_ensure_allocated(out_stack, 1);
    return true;
}

void stack_destroy(stack* s) {
    if (s) {
        if (s->memory) {
            memory_free(s->memory, s->allocated, MEMORY_TAG_STACK);
        }
        memory_zero(s, sizeof(stack));
    }
}

void stack_reserve(stack* s, u32 count) {
    if (s) {
        stack_ensure_allocated(s, count);
    }
}

b8 stack_push(stack* s, void* data) {
    if (!s) {
        MERROR("stack_push requires a pointer to a valid stack!");
        return false;
    }

    stack_ensure_allocated(s, s->count + 1);
    memory_copy((void*)((u64*)s->memory + (s->count * s->stride)), data, s->stride);
    s->count++;
    return true;
}

b8 stack_peek(stack* s, void* out_data) {
    if (!s || !out_data) {
        MERROR("stack_peek requires a pointer to a valid stack and to hold element data output!");
        return false;
    }

    if (s->count < 1) {
        MWARN("stack_peek - stack is empty!");
        return false;
    }

    memory_copy(out_data, (void*)((u64)s->memory + ((s->count - 1) * s->stride)), s->stride);
    return true;
}

b8 stack_pop(stack* s, void* out_data) {
    if (!s || !out_data) {
        MERROR("stack_pop resuires a pointer to a valid stack and to hold element data output!");
        return false;
    }

    if (s->count < 1) {
        MWARN("stack_pop - stack is empty!");
        return false;
    }

    memory_copy(out_data, (void*)((u64)s->memory + ((s->count - 1) * s->stride)), s->stride);

    s->count--;

    return true;
}