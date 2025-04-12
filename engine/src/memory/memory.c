#include "memory.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "memory/allocators/dynamic_allocator.h"
#include "threads/mutex.h"

#include "core/logger.h"
#include "strings/string.h"

#define USE_CUSTOM_MEMORY_ALLOCATOR 1

#if !USE_CUSTOM_MEMORY_ALLOCATOR
#   if _MSC_VER
#       include <malloc.h>
#       define maligned_alloc _aligned_malloc
#       define maligned_free _aligned_free
#   else
#       include <stdlib.h>
#       define maligned_alloc(size, alignment) aligned_alloc(alignment, size)
#       define maligned_free free
#   endif
#endif


static char* memory_tag_strings[MEMORY_TAG_MAX_TAGS] = {
    "UNKNOWN     ",

    "DARRAY      ",
    "QUEUE       ",
    "RING_QUEUE  ",
    "STACK       ",
    "BST         ",
    "STRING      ",

    "LINEAR_ALLOC",

    "ENGINE      ",
    "PLATFORM    ",
    "RENDERER    ",
    "GAME        ",

    "GPU_LOCAL   ",

    "VULKAN      "
};

typedef struct memory_system_state {
    u64 allocation_count;
    u64 total_allocated;
    u64 tagged_allocations[MEMORY_TAG_MAX_TAGS];

    u64 allocator_memory_requirement;
    dynamic_allocator allocator;
    void* allocator_block;

    mutex allocation_mutex;
} memory_system_state;

static memory_system_state* state_ptr;

b8 memory_system_initialize() {
#if USE_CUSTOM_MEMORY_ALLOCATOR
    u64 state_memory_requirement = sizeof(memory_system_state);

    u64 alloc_requirement = 0;
    dynamic_allocator_create(GIBIBYTES(1), &alloc_requirement, nullptr, nullptr);

    void* block = malloc(state_memory_requirement + alloc_requirement);
    if (!block) {
        MFATAL("Memory system allocation failed and the system cannot continue!");
        return false;
    }

    state_ptr = (memory_system_state*)block;
    state_ptr->allocation_count = 0;
    state_ptr->allocator_memory_requirement = alloc_requirement;
    state_ptr->total_allocated = 0;
    memory_zero(&state_ptr->tagged_allocations, sizeof(u64) * MEMORY_TAG_MAX_TAGS);
    state_ptr->allocator_block = ((void*)block + state_memory_requirement);

    if (!dynamic_allocator_create(
        GIBIBYTES(1),
        &state_ptr->allocator_memory_requirement,
        state_ptr->allocator_block,
        &state_ptr->allocator
    )) {
        MFATAL("Memory system is unable to setup internal allocator. Application cannot continue!");
        return false;
    }
#else
    state_ptr = maligned_alloc(sizeof(memory_system_state), 16);
    state_ptr->allocation_count = 0;
    state_ptr->allocator_memory_requirement = 0;
    state_ptr->total_allocated = 0;
    memory_zero(&state_ptr->tagged_allocations, sizeof(u64) * MEMORY_TAG_MAX_TAGS);
    state_ptr->allocator_block = nullptr;
#endif

    if (!mutex_create(&state_ptr->allocation_mutex)) {
        MFATAL("Unable to create allocation mutex!");
        return false;
    }

    return true;
}

void memory_system_shutdown() {
    if (state_ptr) {
        mutex_destroy(&state_ptr->allocation_mutex);
#if USE_CUSTOM_MEMORY_ALLOCATOR
        dynamic_allocator_destroy(&state_ptr->allocator);
        free(state_ptr);
#else
        maligned_free(state_ptr);
#endif
        state_ptr = nullptr;
    }
}

void* memory_allocate(u64 size, memory_tag tag) {
    return memory_allocate_aligned(size, 1, tag);
}

void* memory_allocate_aligned(u64 size, u16 alignment, memory_tag tag) {
    if (tag == MEMORY_TAG_UNKNOWN) {
        MWARN("memory_allocate_aligned - Called using MEMORY_TAG_UNKNOWN. Re-class this allocation!");
    }

    void* block = nullptr;
    if (state_ptr) {
        if (!mutex_lock(&state_ptr->allocation_mutex)) {
            MFATAL("Error obtaining mutex lock during allocation!");
            return nullptr;
        }

        // FIXME: Alignment
        state_ptr->allocation_count++;
        state_ptr->total_allocated += size;
        state_ptr->tagged_allocations[tag] += size;

#if USE_CUSTOM_MEMORY_ALLOCATOR
        block = dynamic_allocator_allocate_aligned(&state_ptr->allocator, size, alignment);
#else
        block = maligned_alloc(size, alignment);
#endif
        mutex_unlock(&state_ptr->allocation_mutex);
    } else {
        block = malloc(size);
    }

    if (block) {
        memory_zero(block, size);
        return block;
    }

    MFATAL("memory_allocate_aligned - Failed to allocate memory!");
    return nullptr;
}

void memory_allocate_report(u64 size, memory_tag tag) {
    if (!mutex_lock(&state_ptr->allocation_mutex)) {
        MERROR("Error obtaining mutex lock during allocation reporting!");
        return;
    }

    state_ptr->allocation_count++;
    state_ptr->total_allocated += size;
    state_ptr->tagged_allocations[tag] += size;

    mutex_unlock(&state_ptr->allocation_mutex);
}

void* memory_reallocate(void* block, u64 old_size, u64 new_size, memory_tag tag) {
    return memory_reallocate_aligned(block, old_size, new_size, 1, tag);
}

void* memory_reallocate_aligned(void* block, u64 old_size, u64 new_size, u16 alignment, memory_tag tag) {
    void* new_block = memory_allocate_aligned(new_size, alignment, tag);
    if (block && new_block) {
        memory_copy(new_block, block, old_size);
        memory_free_aligned(block, old_size, alignment, tag);
    }
    return new_block;
}

void memory_reallocate_report(u64 old_size, u64 new_size, memory_tag tag) {
    memory_free_report(old_size, tag);
    memory_allocate_report(new_size, tag);
}

void memory_free(void* block, u64 size, memory_tag tag) {
    memory_free_aligned(block, size, 1, tag);
}

void memory_free_aligned(void* block, u64 size, u16 alignment, memory_tag tag) {
    if (tag == MEMORY_TAG_UNKNOWN) {
        MWARN("memory_free_aligned - Called using MEMORY_TAG_UNKNOWN. Re-class this allocation!");
    }

    if (state_ptr) {
        if (!mutex_lock(&state_ptr->allocation_mutex)) {
            MFATAL("Unable to obtain mutex lock for free operation! Heap corruption is likely!");
            return;
        }
#if USE_CUSTOM_MEMORY_ALLOCATOR
        u64 osize = 0;
        u16 oalignment = 0;
        dynamic_allocator_get_size_alignment(&state_ptr->allocator, block, &osize, &oalignment);
        if (osize != size) {
            MWARN("Free size mismatch! (orig=%llu, req=%llu)", osize, size);
        }
        if (oalignment != alignment) {
            MWARN("Free alignment mismatch! (orig=%llu, req=%llu)", oalignment, alignment);
        }

        b8 result = dynamic_allocator_free_aligned(&state_ptr->allocator, block);
#else
        maligned_free(block);
        b8 result = true;
#endif

        state_ptr->allocation_count--;
        state_ptr->total_allocated -= size;
        state_ptr->tagged_allocations[tag] -= size;

        mutex_unlock(&state_ptr->allocation_mutex);

        if (!result) {
            // TODO: Alignment
            free(block);
        }
    } else {
        // Alignment
        free(block);
    }
}

void memory_free_report(u64 size, memory_tag tag) {
    if (!mutex_lock(&state_ptr->allocation_mutex)) {
        MFATAL("Error obtaining mutex lock during free reporting!");
        return;
    }

    state_ptr->allocation_count--;
    state_ptr->total_allocated -= size;
    state_ptr->tagged_allocations[tag] -= size;

    mutex_unlock(&state_ptr->allocation_mutex);
}

void* memory_zero(void* block, u64 size) {
    return memset(block, 0, size);
}

void* memory_copy(void* dst, const void* src, u64 size) {
    return memcpy(dst, src, size);
}

void* memory_set(void* block, i32 value, u64 size) {
    return memset(block, value, size);
}

static const char* get_unit_for_size(u64 size_bytes, f32* out_amount) {
    if (size_bytes >= GIBIBYTES(1)) {
        *out_amount = (f64)size_bytes / GIBIBYTES(1);
        return "GiB";
    } else if (size_bytes >= MEBIBYTES(1)) {
        *out_amount = (f64)size_bytes / MEBIBYTES(1);
        return "MiB";
    } else if (size_bytes >= KIBIBYTES(1)) {
        *out_amount = (f64)size_bytes / KIBIBYTES(1);
        return "KiB";
    } else {
        *out_amount = (f32)size_bytes;
        return "B";
    }
}

char* memory_get_usage_str() {
    char buffer[1024] = "System memory usage (tagged):\n";
    u64 offset = strlen(buffer);

    for (u32 i = 0; i < MEMORY_TAG_MAX_TAGS; ++i) {
        f32 amount = 1.0f;
        const char* unit = get_unit_for_size(state_ptr->tagged_allocations[i], &amount);
        
        i32 length = snprintf(buffer + offset, 1024, "  %s: %.2f%s\n", memory_tag_strings[i], amount, unit);
        offset += length;
    }

    snprintf(buffer + offset, 1024, "  Total allocated: %llu\n  Allocations count: %llu\n", state_ptr->total_allocated, state_ptr->allocation_count);

    char* out_str = cstr_duplicate(buffer);
    return out_str;
}