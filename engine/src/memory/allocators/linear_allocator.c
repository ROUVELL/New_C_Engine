#include "linear_allocator.h"

#include "memory/memory.h"
#include "core/logger.h"


void linear_allocator_create(u64 total_size, void* memory, linear_allocator* out_allocator) {
    out_allocator->total_size = total_size;
    out_allocator->allocated = 0;
    if (memory) {
        out_allocator->memory = memory;
        out_allocator->owns_memory = false;
    } else {
        out_allocator->memory = memory_allocate(total_size, MEMORY_TAG_LINEAR_ALLOCATOR);
        out_allocator->owns_memory = true;
    }
}

void linear_allocator_destroy(linear_allocator* allocator) {
    if (allocator) {
        if (allocator->owns_memory && allocator->memory) {
            memory_free(allocator->memory, allocator->total_size, MEMORY_TAG_LINEAR_ALLOCATOR);
            allocator->memory = nullptr;
            allocator->owns_memory = false;
        }
        allocator->allocated = 0;
        allocator->total_size = 0;
    }
}

void* linear_allocator_allocate(linear_allocator* allocator, u64 size) {
    if (!allocator || !allocator->memory) {
        MERROR("linear_allocator_allocate - Provided allocator not initialized!");
        return nullptr;
    }

    if (allocator->allocated + size > allocator->total_size) {
        u64 remaining = allocator->total_size - allocator->allocated;
        MERROR("linear_allocator_allocate - Tried to allocate %lluB, only %lluB remaining!", size, remaining);
        return nullptr;
    }

    void* block = ((u8*)allocator->memory) + allocator->allocated;
    allocator->allocated += size;
    return block;
}

void linear_allocator_free_all(linear_allocator* allocator, b8 clear) {
    if (allocator && allocator->memory) {
        allocator->allocated = 0;
        if (clear) {
            memory_zero(allocator->memory, allocator->total_size);
        }
    }
}