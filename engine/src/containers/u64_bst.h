#pragma once

#include "defines.h"

typedef union bst_node_value {
    void* ptr;
    const char* str;
    u64 u64;
    i64 i64;
    u32 u32;
    i32 i32;
    u16 u16;
    i16 i16;
    u8 u8;
    i8 i8;
    b8 b8;
    f32 f32;
} bst_node_value;

typedef struct bst_node {
    u64 key;
    bst_node_value value;
    struct bst_node* left;
    struct bst_node* right;
} bst_node;

MAPI bst_node* u64_bst_insert(bst_node* root, u64 key, bst_node_value value);

MAPI bst_node* u64_bst_delete(bst_node* root, u64 key);

MAPI const bst_node* u64_bst_find(const bst_node* root, u64 key);

MAPI void u64_bst_clear(bst_node* root);