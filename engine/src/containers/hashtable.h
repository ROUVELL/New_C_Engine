#pragma once

#include "defines.h"


typedef struct hashtable {
    u32 element_size;
    u32 element_count;
    b8 is_pointer_type;
    void* memory;
} hashtable;


MAPI b8 hashtable_create(u64 element_size, u32 element_count, b8 is_pointer_type, void* memory, hashtable* out_table);

MAPI void hashtable_destroy(hashtable* table);

MAPI b8 hashtable_set(hashtable* table, const char* key, void* value);

MAPI b8 hashtable_set_ptr(hashtable* table, const char* key, void** value);

MAPI b8 hashtable_get(hashtable* table, const char* key, void* out_value);

MAPI b8 hashtable_get_ptr(hashtable* table, const char* key, void** out_value);

MAPI b8 hashtable_fill(hashtable* table, void* value);