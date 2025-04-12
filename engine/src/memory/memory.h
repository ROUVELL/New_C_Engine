#pragma once

#include "defines.h"

typedef struct frame_allocator_int {
    void* (*allocate)(u64 size);
    void (*free)(void* block, u64 size);
    void (*free_all)(void);
} frame_allocator_int;

typedef enum memory_tag {
    MEMORY_TAG_UNKNOWN,

    MEMORY_TAG_DARRAY,
    MEMORY_TAG_QUEUE,
    MEMORY_TAG_RING_QUEUE,
    MEMORY_TAG_STACK,
    MEMORY_TAG_BST,
    MEMORY_TAG_STRING,

    MEMORY_TAG_LINEAR_ALLOCATOR,

    MEMORY_TAG_ENGINE,
    MEMORY_TAG_PLATFORM,
    MEMORY_TAG_RENDERER,
    MEMORY_TAG_GAME,

    MEMORY_TAG_GPU_LOCAL,

    MEMORY_TAG_VULKAN,
    
    MEMORY_TAG_MAX_TAGS
} memory_tag;

b8 memory_system_initialize();
void memory_system_shutdown();

// Performs a memory allocation from the host of the given size.
// The allocation is tracked for the provided tag
MAPI void* memory_allocate(u64 size, memory_tag tag);

// Performs an aligned memory allocation from the host of the given size and alignment
// The allocation is tracked for the provided tag.
// NOTE: Memory allocated this way must be freed using memory_free_aligned
MAPI void* memory_allocate_aligned(u64 size, u16 alignment, memory_tag tag);

// Reports an allocation associated with the application, but made externally.
// This can be done for items allocated within 3rd party libraries, for example,
// to track allocations but not perform them
MAPI void memory_allocate_report(u64 size, memory_tag tag);

// Performs a memory reallocation from the host of the given size, and also frees the block of memory given.
// The reallocation is tracked for the provided tag
MAPI void* memory_reallocate(void* block, u64 old_size, u64 new_size, memory_tag tag);

// Performs a memory reallocation from the host of the given size and alignment, and also frees the block of memory given.
// The reallocation is tracked for the provided tag
// NOTE: Memory allocated this way must be freed using memory_free_aligned
MAPI void* memory_reallocate_aligned(void* block, u64 old_size, u64 new_size, u16 alignment, memory_tag tag);

// Reports a reallocation associated with the application, but made externally.
// This can be done for items allocated within 3rd party libraries, for example,
// to track reallocations but not perform them
MAPI void memory_reallocate_report(u64 old_size, u64 new_size, memory_tag tag);

// Frees the given block, and untrack its size from the given tag
MAPI void memory_free(void* block, u64 size, memory_tag tag);

// Frees the given block, and untrack its size from the given tag
MAPI void memory_free_aligned(void* block, u64 size, u16 alignment, memory_tag tag);

// Reports a free associated with the application, but made externally.
// This can be done for items allocated within 3rd party libraries, for example,
// to track frees but not perform them
MAPI void memory_free_report(u64 size, memory_tag tag);

MAPI void* memory_zero(void* block, u64 size);

MAPI void* memory_copy(void* dst, const void* src, u64 size);

MAPI void* memory_set(void* block, i32 value, u64 size);

MAPI char* memory_get_usage_str();