#pragma once

#include "defines.h"

struct frame_allocator_int;

typedef struct darray_header {
    u64 capacity;
    u64 length;
    u64 stride;
    struct frame_allocator_int* allocator;
} darray_header;

MAPI void* _darray_create(u64 length, u64 stride, struct frame_allocator_int* allocator);

MAPI void* _darray_push(void* arr, const void* value_ptr);

MAPI void* _darray_insert_at(void* arr, u64 index, void* value_ptr);

MAPI void* darray_duplicate(void* arr);

#define DARRAY_DEFAULT_CAPACITY 1
#define DARRAY_RESIZE_FACTOR 2

#define darray_create(type) _darray_create(DARRAY_DEFAULT_CAPACITY, sizeof(type), nullptr)

#define darray_create_with_allocator(type, allocator) _darray_create(DARRAY_DEFAULT_CAPACITY, sizeof(type), allocator)

#define darray_reserve(type, capacity) _darray_create(capacity, sizeof(type), nullptr)

#define darray_reserve_with_allocator(type, capacity, allocator) _darray_create(capacity, sizeof(type), allocator)

MAPI void darray_destroy(void* arr);

#define darray_push(arr, value) \
    { \
        typeof(value) __tmp_value__ = value; \
        arr = _darray_push(arr, &__tmp_value__); \
    }

MAPI void darray_pop(void* arr, void* dst);

MAPI void darray_pop_at(void* arr, u64 index, void* dst);

#define darray_insert_at(arr, index, value) \
    { \
        typeof(value) __tmp_value__ = value; \
        arr = _darray_insert_at(arr, index, &__tmp_value__); \
    }

MAPI void* darray_resize(void* arr, u64 size);

MAPI void darray_clear(void* arr);

MAPI u64 darray_capacity(void* arr);

MAPI u64 darray_length(void* arr);

MAPI u64 darray_stride(void* arr);

// New darray

MAPI void _kdarray_init(u32 length, u32 stride, u32 capacity, struct frame_allocator_int* allocator, u32* out_length, u32* out_stride, u32* out_capacity, void** block, struct frame_allocator_int** out_allocator);
MAPI void _kdarray_free(u32* length, u32* capacity, u32* stride, void** block, struct frame_allocator_int** out_allocator);
MAPI void _kdarray_ensure_size(u32 required_length, u32 stride, u32* out_capacity, struct frame_allocator_int* allocator, void** block, void** base_block);

typedef struct darray_base {
    u32 length;
    u32 stride;
    u32 capacity;
    struct frame_allocator_int* allocator;
    void* p_data;
} darray_base;

typedef struct darray_iterator {
    darray_base* arr;
    i32 pos;
    i32 dir;
    b8 (*end)(const struct darray_iterator* it);
    void* (*value)(const struct darray_iterator* it);
    void (*next)(struct darray_iterator* it);
    void (*prev)(struct darray_iterator* it);
} darray_iterator;

MAPI darray_iterator darray_iterator_begin(darray_base* arr);
MAPI darray_iterator darray_iterator_rbegin(darray_base* arr);
MAPI b8 darray_iterator_end(const darray_iterator* it);
MAPI void* darray_iterator_value(const darray_iterator* it);
MAPI void darray_iterator_next(darray_iterator* it);
MAPI void darray_iterator_prev(darray_iterator* it);

#define DARRAY_TYPE_NAMED(type, name)                                                                                                                                       \
    typedef struct darray_##name {                                                                                                                                          \
        darray_base base;                                                                                                                                                   \
        type* data;                                                                                                                                                         \
        darray_iterator (*begin)(darray_base * arr);                                                                                                                        \
        darray_iterator (*rbegin)(darray_base * arr);                                                                                                                       \
    } darray_##name;                                                                                                                                                        \
                                                                                                                                                                            \
    MINLINE darray_##name darray_##name##_reserve_with_allocator(u32 capacity, struct frame_allocator_int* allocator) {                                                     \
        darray_##name arr;                                                                                                                                                  \
        _kdarray_init(0, sizeof(type), capacity, allocator, &arr.base.length, &arr.base.stride, &arr.base.capacity, (void**)&arr.data, &arr.base.allocator);                \
        arr.base.p_data = arr.data;                                                                                                                                         \
        arr.begin = darray_iterator_begin;                                                                                                                                  \
        arr.rbegin = darray_iterator_rbegin;                                                                                                                                \
        return arr;                                                                                                                                                         \
    }                                                                                                                                                                       \
                                                                                                                                                                            \
    MINLINE darray_##name darray_##name##_create_with_allocator(struct frame_allocator_int* allocator) {                                                                    \
        darray_##name arr;                                                                                                                                                  \
        _kdarray_init(0, sizeof(type), DARRAY_DEFAULT_CAPACITY, allocator, &arr.base.length, &arr.base.stride, &arr.base.capacity, (void**)&arr.data, &arr.base.allocator); \
        arr.base.p_data = arr.data;                                                                                                                                         \
        arr.begin = darray_iterator_begin;                                                                                                                                  \
        arr.rbegin = darray_iterator_rbegin;                                                                                                                                \
        return arr;                                                                                                                                                         \
    }                                                                                                                                                                       \
                                                                                                                                                                            \
    MINLINE darray_##name darray_##name##_reserve(u32 capacity) {                                                                                                           \
        darray_##name arr;                                                                                                                                                  \
        _kdarray_init(0, sizeof(type), capacity, 0, &arr.base.length, &arr.base.stride, &arr.base.capacity, (void**)&arr.data, &arr.base.allocator);                        \
        arr.base.p_data = arr.data;                                                                                                                                         \
        arr.begin = darray_iterator_begin;                                                                                                                                  \
        arr.rbegin = darray_iterator_rbegin;                                                                                                                                \
        return arr;                                                                                                                                                         \
    }                                                                                                                                                                       \
                                                                                                                                                                            \
    MINLINE darray_##name darray_##name##_create(void) {                                                                                                                    \
        darray_##name arr;                                                                                                                                                  \
        _kdarray_init(0, sizeof(type), DARRAY_DEFAULT_CAPACITY, 0, &arr.base.length, &arr.base.stride, &arr.base.capacity, (void**)&arr.data, &arr.base.allocator);         \
        arr.base.p_data = arr.data;                                                                                                                                         \
        arr.begin = darray_iterator_begin;                                                                                                                                  \
        arr.rbegin = darray_iterator_rbegin;                                                                                                                                \
        return arr;                                                                                                                                                         \
    }                                                                                                                                                                       \
                                                                                                                                                                            \
    MINLINE darray_##name* darray_##name##_push(darray_##name* arr, type data) {                                                                                            \
        _kdarray_ensure_size(arr->base.length + 1, arr->base.stride, &arr->base.capacity, arr->base.allocator, (void**)&arr->data, (void**)&arr->base.p_data);              \
        arr->data[arr->base.length] = data;                                                                                                                                 \
        arr->base.length++;                                                                                                                                                 \
        return arr;                                                                                                                                                         \
    }                                                                                                                                                                       \
                                                                                                                                                                            \
    MINLINE b8 darray_##name##_pop(darray_##name* arr, type* out_value) {                                                                                                   \
        if (arr->base.length < 1) {                                                                                                                                         \
            return false;                                                                                                                                                   \
        }                                                                                                                                                                   \
        *out_value = arr->data[arr->base.length - 1];                                                                                                                       \
        arr->base.length--;                                                                                                                                                 \
        return true;                                                                                                                                                        \
    }                                                                                                                                                                       \
                                                                                                                                                                            \
    MINLINE b8 darray_##name##_pop_at(darray_##name* arr, u32 index, type* out_value) {                                                                                     \
        if (index >= arr->base.length) {                                                                                                                                    \
            return false;                                                                                                                                                   \
        }                                                                                                                                                                   \
        *out_value = arr->data[index];                                                                                                                                      \
        for (u32 i = index; i < arr->base.length; ++i) {                                                                                                                    \
            arr->data[i] = arr->data[i + 1];                                                                                                                                \
        }                                                                                                                                                                   \
        arr->base.length--;                                                                                                                                                 \
        return true;                                                                                                                                                        \
    }                                                                                                                                                                       \
                                                                                                                                                                            \
    MINLINE b8 darray_##name##_insert_at(darray_##name* arr, u32 index, type data) {                                                                                        \
        if (index > arr->base.length) {                                                                                                                                     \
            return false;                                                                                                                                                   \
        }                                                                                                                                                                   \
        _kdarray_ensure_size(arr->base.length + 1, arr->base.stride, &arr->base.capacity, arr->base.allocator, (void**)&arr->data, (void**)&arr->base.p_data);              \
        arr->base.length++;                                                                                                                                                 \
        for (u32 i = arr->base.length; i > index; --i) {                                                                                                                    \
            arr->data[i] = arr->data[i - 1];                                                                                                                                \
        }                                                                                                                                                                   \
        arr->data[index] = data;                                                                                                                                            \
        return true;                                                                                                                                                        \
    }                                                                                                                                                                       \
                                                                                                                                                                            \
    MINLINE darray_##name* darray_##name##_clear(darray_##name* arr) {                                                                                                      \
        arr->base.length = 0;                                                                                                                                               \
        return arr;                                                                                                                                                         \
    }                                                                                                                                                                       \
                                                                                                                                                                            \
    MINLINE void darray_##name##_destroy(darray_##name* arr) {                                                                                                              \
        _kdarray_free(&arr->base.length, &arr->base.capacity, &arr->base.stride, (void**)&arr->data, &arr->base.allocator);                                                 \
        arr->begin = 0;                                                                                                                                                     \
        arr->rbegin = 0;                                                                                                                                                    \
    }

// Create an array type of the given type. For advanced types or pointers, use ARRAY_TYPE_NAMED directly.
#define DARRAY_TYPE(type) DARRAY_TYPE_NAMED(type, type)

// Create array types for well-known types

DARRAY_TYPE(b8);

DARRAY_TYPE(u8);
DARRAY_TYPE(u16);
DARRAY_TYPE(u32);
DARRAY_TYPE(u64);

DARRAY_TYPE(i8);
DARRAY_TYPE(i16);
DARRAY_TYPE(i32);
DARRAY_TYPE(i64);

DARRAY_TYPE(f32);
DARRAY_TYPE(f64);

// Create array types for well-known "advanced" types, such as strings.
DARRAY_TYPE_NAMED(const char*, string);