#pragma once

#include "defines.h"


typedef struct freelist {
    void* memory;
} freelist;


MAPI void freelist_create(u64 total_size, u64* memory_requirement, void* memory, freelist* out_list);

MAPI void freelist_destroy(freelist* list);

MAPI b8 freelist_allocate_block(freelist* list, u64 size, u64* out_offset);

MAPI b8 freelist_free_block(freelist* list, u64 size, u64 offset);

MAPI b8 freelist_resize(freelist* list, u64* memory_requirement, void* new_memory, u64 new_size, void** out_old_memory);

MAPI void freelist_clear(freelist* list);

MAPI u64 freelist_free_space(freelist* list);