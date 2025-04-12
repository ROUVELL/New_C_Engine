#include "dynamic_allocator.h"

#include "core/logger.h"
#include "core/asserts.h"
#include "memory/memory.h"
#include "containers/freelist.h"


typedef struct dynamic_allocator_state {
    u64 total_size;
    freelist list;
    void* freelist_block;
    void* memory_block;
} dynamic_allocator_state;

typedef struct alloc_header {
    void* start;
    u16 alignment;
} alloc_header;

#define KSIZE_STORAGE sizeof(u32)

b8 dynamic_allocator_create(u64 total_size, u64* memory_requirement, void* memory, dynamic_allocator* out_allocator) {
    if (total_size < 1) {
        MERROR("dynamic_allocator_create - Cannot have a total_size of 0! Creation failed!");
        return false;
    }

    u64 freelist_requirement = 0;
    freelist_create(total_size, &freelist_requirement, nullptr, nullptr);

    *memory_requirement = freelist_requirement + sizeof(dynamic_allocator_state) + total_size;
    if (!memory) {
        return true;
    }

    out_allocator->memory = memory;
    dynamic_allocator_state* state = out_allocator->memory;
    state->total_size = total_size;
    state->freelist_block = (void*)(out_allocator->memory + sizeof(dynamic_allocator_state));
    state->memory_block = (void*)(state->freelist_block + freelist_requirement);

    freelist_create(total_size, &freelist_requirement, state->freelist_block, &state->list);

    memory_zero(state->memory_block, total_size);
    return true;
}

b8 dynamic_allocator_destroy(dynamic_allocator* allocator) {
    if (allocator) {
        dynamic_allocator_state* state = allocator->memory;
        freelist_destroy(&state->list);
        memory_zero(state->memory_block, state->total_size);
        state->total_size = 0;
        allocator->memory = nullptr;
        return true;
    }

    MWARN("dynamic_allocator_destroy - A pointer to an allocator is required! Destroying failed!");
    return false;
}

void* dynamic_allocator_allocate(dynamic_allocator* allocator, u64 size) {
    return dynamic_allocator_allocate_aligned(allocator, size, 1);
}

void* dynamic_allocator_allocate_aligned(dynamic_allocator* allocator, u64 size, u16 alignment) {
    if (!allocator || !size || !alignment) {
        MERROR("dynamic_allocator_allocate_aligned requires a valid allocator, size and alignment!");
        return nullptr;
    }
    dynamic_allocator_state* state = allocator->memory;

    u64 header_size = sizeof(alloc_header);
    u64 storage_size = KSIZE_STORAGE;
    u64 required_size = alignment + header_size + storage_size + size;

    MASSERT_MSG(required_size < 4294967295U, "dynamic_allocator_allocate_aligned called with required size > 4 GiB. Don't do that.");

    u64 base_offset = 0;
    if (freelist_allocate_block(&state->list, required_size, &base_offset)) {
        void* ptr = (void*)((u64)state->memory_block + base_offset);

        u64 aligned_block_offset = get_aligned((u64)ptr + KSIZE_STORAGE, alignment);

        u32* block_size = (u32*)(aligned_block_offset - KSIZE_STORAGE);
        *block_size = (u32)size;

        alloc_header* header = (alloc_header*)(aligned_block_offset + size);
        header->start = ptr;

        MASSERT_MSG(header->start, "dynamic_allocator_allocate_aligned got a null pointer (0x0). Memory corruption likely as this should always be nonzero.");
        header->alignment = alignment;
        MASSERT_MSG(header->alignment, "dynamic_allocator_allocate_aligned got an alignment of 0. Memory corruption likely as this should always be nonzero.");

        return (void*)aligned_block_offset;
    }

    MERROR("dynamic_allocator_allocate_aligned no blocks of memory large enough to allocate from.");
    u64 available = freelist_free_space(&state->list);
    MERROR("Requested size: %llu, total space available: %llu", size, available);
    // TODO: Report fragmentation?
    return nullptr;
}

b8 dynamic_allocator_free(dynamic_allocator* allocator, void* block) {
    return dynamic_allocator_free_aligned(allocator, block);
}

b8 dynamic_allocator_free_aligned(dynamic_allocator* allocator, void* block) {
    if (!allocator || !block) {
        MERROR("dynamic_allocator_free_aligned requires both a valid allocator (0x%p) and a block (0x%p) to be freed.", allocator, block);
        return false;
    }

    dynamic_allocator_state* state = allocator->memory;
    if (block < state->memory_block || block > state->memory_block + state->total_size) {
        void* end_of_block = (void*)(state->memory_block + state->total_size);
        MWARN("dynamic_allocator_free_aligned trying to release block (0x%p) outside of allocator range (0x%p)-(0x%p)", block, state->memory_block, end_of_block);
        return false;
    }

    u32* block_size = (u32*)((u64)block - KSIZE_STORAGE);
    alloc_header* header = (alloc_header*)((u64)block + *block_size);
    u64 required_size = header->alignment + sizeof(alloc_header) + KSIZE_STORAGE + *block_size;
    u64 offset = (u64)header->start - (u64)state->memory_block;
    if (!freelist_free_block(&state->list, required_size, offset)) {
        MERROR("dynamic_allocator_free_aligned failed.");
        return false;
    }

    return true;
}

b8 dynamic_allocator_get_size_alignment(dynamic_allocator* allocator, void* block, u64* out_size, u16* out_alignment) {
    dynamic_allocator_state* state = allocator->memory;
    if (block < state->memory_block || block >= ((void*)((u8*)state->memory_block) + state->total_size)) {
        // Not owned by this block.
        return false;
    }

    // Get the header.
    *out_size = *(u32*)((u64)block - KSIZE_STORAGE);
    MASSERT_MSG(*out_size, "dynamic_allocator_get_size_alignment found an out_size of 0. Memory corruption likely.");
    alloc_header* header = (alloc_header*)((u64)block + *out_size);
    *out_alignment = header->alignment;
    MASSERT_MSG(header->start, "dynamic_allocator_get_size_alignment found a header->start of 0. Memory corruption likely as this should always be at least 1.");
    MASSERT_MSG(header->alignment, "dynamic_allocator_get_size_alignment found a header->alignment of 0. Memory corruption likely as this should always be at least 1.");
    return true;
}

u64 dynamic_allocator_free_space(dynamic_allocator* allocator) {
    dynamic_allocator_state* state = allocator->memory;
    return freelist_free_space(&state->list);
}

u64 dynamic_allocator_total_space(dynamic_allocator* allocator) {
    dynamic_allocator_state* state = allocator->memory;
    return state->total_size;
}

u64 dynamic_allocator_header_size(void) {
    return sizeof(alloc_header) + KSIZE_STORAGE;
}