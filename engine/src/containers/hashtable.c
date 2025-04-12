#include "hashtable.h"

#include "memory/memory.h"
#include "core/logger.h"

static u64 hash_key(const char* key, u32 element_count) {
    static const u64 multiplier = 97;

    unsigned const char* us;
    u64 hash = 0;

    for (us = (unsigned const char*)key; *us; us++) {
        hash = hash * multiplier + *us;
    }

    return hash % element_count;
}

b8 hashtable_create(u64 element_size, u32 element_count, b8 is_pointer_type, void* memory, hashtable* out_table) {
    if (!element_size || !element_count) {
        MERROR("hashtable_create - element_size and element_count must be a positive non-zero value!");
        return false;
    }

    if (!memory || !out_table) {
        MERROR("hashtable_create - Required valid pointers to memory and out_table!");
        return false;
    }

    out_table->memory = memory;
    out_table->element_count = element_count;
    out_table->element_size = element_size;
    out_table->is_pointer_type = is_pointer_type;
    memory_zero(out_table->memory, element_size * element_count);
    return true;
}

void hashtable_destroy(hashtable* table) {
    if (table) {
        memory_zero(table, sizeof(hashtable));
    }
}

b8 hashtable_set(hashtable* table, const char* key, void* value) {
    if (!table || !key || !value) {
        MERROR("hashtable_set requires table, key and value to exist.");
        return false;
    }

    if (table->is_pointer_type) {
        MERROR("hashtable_set should not be used with tables that have pointer types. Use hashtable_set_ptr instead.");
        return false;
    }

    u64 hash = hash_key(key, table->element_count);
    memory_copy(table->memory + (table->element_size * hash), value, table->element_size);
    return true;
}

b8 hashtable_set_ptr(hashtable* table, const char* key, void** value) {
    if (!table || !key) {
        MERROR("hashtable_set_ptr requires table and key to exist.");
        return false;
    }

    if (!table->is_pointer_type) {
        MERROR("hashtable_set_ptr should not be used with tables that do not have pointer types. Use hashtable_set instead.");
        return false;
    }

    u64 hash = hash_key(key, table->element_count);
    ((void**)table->memory)[hash] = value ? *value : nullptr;
    return true;
}

b8 hashtable_get(hashtable* table, const char* key, void* out_value) {
    if (!table || !key || !out_value) {
        MERROR("hashtable_get requires table, key and out_value to exist.");
        return false;
    }

    if (table->is_pointer_type) {
        MERROR("hashtable_get should not be used with tables that have pointer types. Use hashtable_get_ptr instead.");
        return false;
    }

    u64 hash = hash_key(key, table->element_count);
    memory_copy(out_value, table->memory + (table->element_size * hash), table->element_size);
    return true;
}

b8 hashtable_get_ptr(hashtable* table, const char* key, void** out_value) {
    if (!table || !key || !out_value) {
        MERROR("hashtable_get_ptr requires table, key and out_value to exist.");
        return false;
    }

    if (!table->is_pointer_type) {
        MERROR("hashtable_get_ptr should not be used with tables that do not have pointer types. Use hashtable_get instead.");
        return false;
    }

    u64 hash = hash_key(key, table->element_count);
    *out_value = ((void**)table->memory)[hash];
    return *out_value != nullptr;
}

b8 hashtable_fill(hashtable* table, void* value) {
    if (!table || !value) {
        MERROR("hashtable_fill requires table and value to exist.");
        return false;
    }

    if (table->is_pointer_type) {
        MERROR("hashtable_fill should not be used with tables that have pointer types");
        return false;
    }

    for (u32 i = 0; i < table->element_count; ++i) {
        memory_copy(table->memory + (table->element_size * i), value, table->element_size);
    }

    return true;
}