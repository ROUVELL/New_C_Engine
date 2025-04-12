#pragma once

#include "defines.h"

typedef struct stack {
    u32 stride;
    u32 count;
    u32 allocated;
    void* memory;
} stack;

MAPI b8 stack_create(u32 stride, stack* out_stack);

MAPI void stack_destroy(stack* s);

MAPI void stack_reserve(stack* s, u32 count);

MAPI b8 stack_push(stack* s, void* data);

MAPI b8 stack_peek(stack* s, void* out_data);

MAPI b8 stack_pop(stack* s, void* out_data);