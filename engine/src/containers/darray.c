#include "darray.h"

#include "memory/memory.h"
#include "core/asserts.h"
#include "core/logger.h"


void* _darray_create(u64 length, u64 stride, struct frame_allocator_int* allocator) {
    if (length == 0) {
        MERROR("darray created with zero length!");
    }

    u64 header_size = sizeof(darray_header);
    u64 array_size = length * stride;
    void* new_array = nullptr;
    if (allocator) {
        new_array = allocator->allocate(header_size + array_size);
    } else {
        new_array = memory_allocate(header_size + array_size, MEMORY_TAG_DARRAY);
    }
    memory_zero(new_array, header_size + array_size);

    darray_header* header = (darray_header*)new_array;
    header->capacity = length;
    header->length = 0;
    header->stride = stride;
    header->allocator = allocator;

    return (void*)((u8*)new_array + header_size);
}

void darray_destroy(void* arr) {
    darray_header* header = (darray_header*)((u8*)arr - sizeof(darray_header));
    u64 total_size = sizeof(darray_header) + header->capacity * header->stride;
    if (header->allocator) {
        header->allocator->free(header, total_size);
    } else {
        memory_free(header, total_size, MEMORY_TAG_DARRAY);
    }
}

void* darray_resize(void* arr, u64 size) {
    darray_header* header = (darray_header*)((u8*)arr - sizeof(darray_header));

    void* temp = _darray_create(size, header->stride, header->allocator);

    darray_header* new_header = (darray_header*)((u8*)temp - sizeof(darray_header));
    new_header->length = header->length;

    memory_copy(temp, arr, header->length * header->stride);

    darray_destroy(arr);
    return temp;
}

void* _darray_push(void* arr, const void* value_ptr) {
    darray_header* header = (darray_header*)((u8*)arr - sizeof(darray_header));
    if (header->length >= header->capacity) {
        arr = darray_resize(arr, header->capacity * DARRAY_RESIZE_FACTOR);
        header = (darray_header*)((u8*)arr - sizeof(darray_header));
    }

    u64 addr = (u64)arr;
    addr += (header->length * header->stride);
    memory_copy((void*)addr, value_ptr, header->stride);
    header->length++;
    return arr;
}

void darray_pop(void* arr, void* dst) {
    darray_header* header = (darray_header*)((u8*)arr - sizeof(darray_header));

    if (header->length == 0) {
        MERROR("darray_pop - Empty array! Nothing was done");
        return;
    }

    if (dst) {
        u64 addr = (u64)arr;
        addr += ((header->length - 1) * header->stride);
        memory_copy(dst, (void*)addr, header->stride);
    }

    header->length--;
}

void* _darray_insert_at(void* arr, u64 index, void* value_ptr) {
    darray_header* header = (darray_header*)((u8*)arr - sizeof(darray_header));

    if (index >= header->length) {
        MERROR("darray_insert_at - Index outside the bounds of this array! Length: %llu, index: %llu", header->length, index);
        return arr;
    }

    if (header->length >= header->capacity) {
        arr = darray_resize(arr, header->capacity * DARRAY_RESIZE_FACTOR);
        header = (darray_header*)((u8*)arr - sizeof(darray_header));
    }

    u64 addr = (u64)arr;
    memory_copy(
        (void*)(addr + ((index + 1) * header->stride)),
        (void*)(addr + (index * header->stride)),
        header->stride * (header->length - index)
    );

    memory_copy((void*)(addr + (index * header->stride)), value_ptr, header->stride);

    header->length++;

    return arr;
}

void darray_pop_at(void* arr, u64 index, void* dst) {
    darray_header* header = (darray_header*)((u8*)arr - sizeof(darray_header));
    
    if (header->length == 0) {
        MERROR("darray_pop_at - Empty array! Nothing was done");
        return;
    }

    if (index >= header->length) {
        MERROR("darray_pop_at - Index outside of the bounds of this array! Length: %llu, index: %llu", header->length, index);
        return;
    }

    u64 addr = (u64)arr;
    if (dst) {
        memory_copy(dst, (void*)(addr + (index * header->stride)), header->stride);
    }

    if (index != header->length - 1) {
        memory_copy(
            (void*)(addr + (index * header->stride)),
            (void*)(addr + ((index + 1) * header->stride)),
            header->stride * (header->length - (index + 1))
        );
    }

    header->length--;
}

void* darray_duplicate(void* arr) {
    darray_header* src_header = (darray_header*)((u8*)arr - sizeof(darray_header));

    void* copy = _darray_create(src_header->capacity, src_header->stride, src_header->allocator);
    darray_header* dst_header = (darray_header*)((u8*)copy - sizeof(darray_header));
    dst_header->stride = src_header->stride;
    dst_header->length = src_header->length;
    dst_header->allocator = src_header->allocator;

    memory_copy(copy, arr, dst_header->capacity * dst_header->stride);

    return copy;
}

void darray_clear(void* arr) {
    darray_header* header = (darray_header*)((u8*)arr - sizeof(darray_header));
    // memory_zero ?
    header->length = 0;
}

u64 darray_capacity(void* arr) {
    darray_header* header = (darray_header*)((u8*)arr - sizeof(darray_header));
    return header->capacity;
}

u64 darray_length(void* arr) {
    darray_header* header = (darray_header*)((u8*)arr - sizeof(darray_header));
    return header->length;
}

u64 darray_stride(void* arr) {
    darray_header* header = (darray_header*)((u8*)arr - sizeof(darray_header));
    return header->stride;
}


void _kdarray_init(u32 length, u32 stride, u32 capacity, struct frame_allocator_int* allocator, u32* out_length, u32* out_stride, u32* out_capacity, void** block, struct frame_allocator_int** out_allocator) {
    *out_length = length;
    *out_stride = stride;
    *out_capacity = capacity;
    *out_allocator = allocator;
    if (allocator) {
        *block = allocator->allocate(capacity * stride);
    } else {
        *block = memory_allocate(capacity * stride, MEMORY_TAG_DARRAY);
    }
}

void _kdarray_free(u32* length, u32* capacity, u32* stride, void** block, struct frame_allocator_int** out_allocator) {
    if (*out_allocator) {
        (*out_allocator)->free(*block, (*capacity) * (*stride));
    } else {
        memory_free(*block, (*capacity) * (*stride), MEMORY_TAG_DARRAY);
    }
    *length = 0;
    *capacity = 0;
    *stride = 0;
    *block = 0;
    *out_allocator = 0;
}

void _kdarray_ensure_size(u32 required_length, u32 stride, u32* out_capacity, struct frame_allocator_int* allocator, void** block, void** base_block) {
    if (required_length > *out_capacity) {
        u32 new_capacity = MMAX(required_length, (*out_capacity) * DARRAY_RESIZE_FACTOR);
        if (allocator) {
            void* new_block = allocator->allocate(new_capacity * stride);
            memory_copy(new_block, *block, (*out_capacity) * stride);
            allocator->free(*block, (*out_capacity) * stride);
            *block = new_block;
        } else {
            *block = memory_reallocate(*block, (*out_capacity) * stride, new_capacity * stride, MEMORY_TAG_DARRAY);
        }
        *base_block = *block;
        *out_capacity = new_capacity;
    }
}

darray_iterator darray_iterator_begin(darray_base* arr) {
    darray_iterator it;
    it.arr = arr;
    it.pos = 0;
    it.dir = 1;
    it.end = darray_iterator_end;
    it.value = darray_iterator_value;
    it.next = darray_iterator_next;
    it.prev = darray_iterator_prev;
    return it;
}

darray_iterator darray_iterator_rbegin(darray_base* arr) {
    darray_iterator it;
    it.arr = arr;
    it.pos = arr->length - 1;
    it.dir = -1;
    it.end = darray_iterator_end;
    it.value = darray_iterator_value;
    it.next = darray_iterator_next;
    it.prev = darray_iterator_prev;
    return it;
}

b8 darray_iterator_end(const darray_iterator* it) {
    return it->dir == 1 ? it->pos >= (i32)it->arr->length : it->pos < 0;
}

void* darray_iterator_value(const darray_iterator* it) {
    return (void*)(((u64)it->arr->p_data) + (it->arr->stride * it->pos));
}

void darray_iterator_next(darray_iterator* it) {
    it->pos += it->dir;
}

void darray_iterator_prev(darray_iterator* it) {
    it->pos -= it->dir;
}