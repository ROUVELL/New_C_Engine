#include "u64_bst.h"

#include "memory/memory.h"


static bst_node* node_create(u64 key, bst_node_value value) {
    bst_node* node = memory_allocate(sizeof(bst_node), MEMORY_TAG_BST);
    node->key = key;
    node->value = value;
    node->left = nullptr;
    node->right = nullptr;
    return node;
}

static bst_node* find_min(bst_node* root) {
    if (!root) {
        return nullptr;
    } else if (root->left) {
        return find_min(root->left);
    }
    return root;
}


bst_node* u64_bst_insert(bst_node* root, u64 key, bst_node_value value) {
    if (!root) {
        return node_create(key, value);
    }

    if (key < root->key) {
        root->left = u64_bst_insert(root->left, key, value);
    } else if (key > root->key) {
        root->right = u64_bst_insert(root->right, key, value);
    }
    return root;
}

bst_node* u64_bst_delete(bst_node* root, u64 key) {
    if (!root) {
        return nullptr;
    }

    if (key > root->key) {
        root->right = u64_bst_delete(root->right, key);
    } else if (key < root->key) {
        root->left = u64_bst_delete(root->left, key);
    } else {
        if (!root->left && !root->right) {
            memory_free(root, sizeof(bst_node), MEMORY_TAG_BST);
            return nullptr;
        } else if (!root || !root->right) {
            bst_node* tmp;
            if (!root->left) {
                tmp = root->right;
            } else {
                tmp = root->left;
            }
            memory_free(root, sizeof(bst_node), MEMORY_TAG_BST);
            return tmp;
        } else {
            bst_node* tmp = find_min(root->right);
            root->key = tmp->key;
            root->right = u64_bst_delete(root->right, tmp->key);
        }
    }
    return root;
}

const bst_node* u64_bst_find(const bst_node* root, u64 key) {
    if (root == nullptr || key == root->key) {
        return root;
    }

    if (root->key < key) {
        return u64_bst_find(root->right, key);
    }
    return u64_bst_find(root->left, key);
}

void u64_bst_clear(bst_node* root) {
    if (root) {
        if (root->left) {
            u64_bst_clear(root->left);
            root->left = nullptr;
        }
        if (root->right) {
            u64_bst_clear(root->right);
            root->right = nullptr;
        }
        memory_free(root, sizeof(bst_node), MEMORY_TAG_BST);
    }
}