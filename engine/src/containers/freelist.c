#include "freelist.h"

#include "memory/memory.h"
#include "core/logger.h"

typedef struct freelist_node {
    u64 offset;
    u64 size;
    struct freelist_node* next;
} freelist_node;

typedef struct internal_state {
    u64 total_size;
    u64 max_entries;
    freelist_node* head;
    freelist_node* nodes;
} internal_state;

static freelist_node* get_node(freelist* list);
static void return_node(freelist_node* node);


void freelist_create(u64 total_size, u64* memory_requirement, void* memory, freelist* out_list) {
    u64 max_entries = (total_size / (sizeof(void*) * sizeof(freelist_node)));

    if (max_entries < 20) {
        max_entries = 20;
    }

    *memory_requirement = sizeof(internal_state) + (sizeof(freelist_node) * max_entries);
    if (!memory) {
        return;
    }

    out_list->memory = memory;

    memory_zero(out_list->memory, *memory_requirement);
    internal_state* state = out_list->memory;
    state->nodes = (void*)(out_list->memory + sizeof(internal_state));
    state->max_entries = max_entries;
    state->total_size = total_size;

    memory_zero(state->nodes, sizeof(freelist_node) * state->max_entries);

    state->head = &state->nodes[0];
    state->head->offset = 0;
    state->head->size = total_size;
    state->head->next = nullptr;
}

void freelist_destroy(freelist* list) {
    if (list && list->memory) {
        internal_state* state = list->memory;
        memory_zero(list->memory, sizeof(internal_state) + sizeof(freelist_node) * state->max_entries);
        list->memory = nullptr;
    }
}

b8 freelist_allocate_block(freelist* list, u64 size, u64* out_offset) {
    if (!list || !out_offset || !list->memory) {
        return false;
    }

    internal_state* state = list->memory;
    freelist_node* node = state->head;
    freelist_node* previous = nullptr;
    while (node) {
        if (node->size == size) {
            // Exact match. Just return the node
            *out_offset = node->offset;
            freelist_node* node_to_return = nullptr;
            if (previous) {
                previous->next = node->next;
                node_to_return = node;
            } else {
                // This node is the head of the list. Reassgn the head
                // and return the previous head node
                node_to_return = state->head;
                state->head = node->next;
            }
            return_node(node_to_return);
            return true;
        } else if (node->size > size) {
            // Node is larger. Deduct the memory from it and movev the offset by that amount
            *out_offset = node->offset;
            node->size -= size;
            node->offset += size;
            return true;
        }

        previous = node;
        node = node->next;
    }

    u64 free_space = freelist_free_space(list);
    MWARN("freelist_find_block, no block with enough free space found (requested: %lluB, available: %lluB)", size, free_space);
    return false;
}

b8 freelist_free_block(freelist* list, u64 size, u64 offset) {
    if (!list || !list->memory || !size) {
        return false;
    }

    internal_state* state = list->memory;
    freelist_node* node = state->head;
    freelist_node* previous = nullptr;

    if (!node) {
        // Check for the case where the entire thing is allocated
        // In this case a new node is needed at the head
        freelist_node* new_node = get_node(list);
        new_node->offset = offset;
        new_node->size = size;
        new_node->next = nullptr;
        state->head = new_node;
        return true;
    } else {
        while (node) {
            if (node->offset + node->size == offset) {
                // Can be appended to the rigth of this node
                node->size += size;

                // Check if this then connects the range between this and the next
                // node, and if so, combine them and return the second node
                if (node->next && node->next->offset == node->offset + node->size) {
                    node->size += node->next->size;
                    freelist_node* next = node->next;
                    node->next = node->next->next;
                    return_node(next);
                }
                return true;
            } else if (node->offset == offset) {
                // If there is an exact match, this means the exact block of memory
                // that is already free is being freed again
                MFATAL("Attempting to free already-freed block of memory at offset %llu", node->offset);
                return false;
            } else if (node->offset > offset) {
                // Iterated beyound the space to be freed. Need a new node
                freelist_node* new_node = get_node(list);
                new_node->offset = offset;
                new_node->size = size;

                // If there is a previous node, the new node shoud be inserted between this and it
                if (previous) {
                    previous->next = new_node;
                    new_node->next = node;
                } else {
                    // Otherwise, the new node becomes the head
                    new_node->next = node;
                    state->head = new_node;
                }

                // Double-check next node to see if it can be joined
                if (new_node->next && new_node->offset + new_node->size == new_node->next->offset) {
                    new_node->size += new_node->next->size;
                    freelist_node* rubbish = new_node->next;
                    new_node->next = rubbish->next;
                    return_node(rubbish);
                }

                // Double-check previous node to see if the new_node can be joined to it
                if (previous && previous->offset + previous->size == new_node->offset) {
                    previous->size += new_node->size;
                    freelist_node* rubbish = new_node;
                    previous->next = rubbish->next;
                    return_node(rubbish);
                }

                return true;
            }

            // If on the last node and the last node's offset + size < the free space,
            // a new node is required
            if (!node->next && node->offset + node->size < offset) {
                freelist_node* new_node = get_node(list);
                new_node->offset = offset;
                new_node->size = size;
                new_node->next = nullptr;
                node->next = new_node;
                return true;
            }

            previous = node;
            node = node->next;
        }
    }

    MWARN("Unable to find block to be freed! Corruption possible?");
    return false;
}

b8 freelist_resize(freelist* list, u64* memory_requirement, void* new_memory, u64 new_size, void** out_old_memory) {
    if (!list || !memory_requirement || ((internal_state*)list->memory)->total_size > new_size) {
        return false;
    }

    // Enough space to hold state, plus array for all nodes
    u64 max_entries = (new_size / sizeof(void*));

    if (max_entries < 20) {
        max_entries = 20;
    }

    *memory_requirement = sizeof(internal_state) * (sizeof(freelist_node) * max_entries);
    if (!new_memory) {
        return true;
    }

    *out_old_memory = list->memory;

    internal_state* old_state = (internal_state*)list->memory;
    u64 size_diff = new_size - old_state->total_size;

    list->memory = new_memory;

    memory_zero(list->memory, *memory_requirement);

    internal_state* state = (internal_state*)list->memory;
    state->nodes = (void*)(list->memory + sizeof(internal_state));
    state->max_entries = max_entries;
    state->total_size = new_size;

    memory_zero(state->nodes, sizeof(freelist_node) * state->max_entries);

    state->head = &state->nodes[0];

    // Copy over the nodes
    freelist_node* new_list_node = state->head;
    freelist_node* old_node = old_state->head;
    if (!old_node) {
        // If there is no head, then the entire list is allocated. In this case,
        // the head should be set to the difference of the space now available,
        // and at the end of the list
        state->head->offset = old_state->total_size;
        state->head->size = size_diff;
        state->head->next = nullptr;
    } else {
        while (old_node) {
            // Get a new node, copy the offset/size, and set next to it
            freelist_node* new_node = get_node(list);
            new_node->offset = old_node->offset;
            new_node->size = old_node->size;
            new_node->next = nullptr;
            new_list_node->next = new_node;
            // Move to the next entry
            new_list_node = new_list_node->next;

            if (old_node->next) {
                // If there is another node, move on
                old_node = old_node->next;
            } else {
                // Reached the end of the list
                // Check if it extends to the end of the block, If so,
                // just append to the size. Otherwise, create a new node and attach to it
                if (old_node->offset + old_node->size == old_state->total_size) {
                    new_node->size += size_diff;
                } else {
                    freelist_node* new_node_end = get_node(list);
                    new_node_end->offset = old_state->total_size;
                    new_node_end->size = size_diff;
                    new_node_end->next = nullptr;
                    new_node->next = new_node_end;
                }
                break;
            }
        }
    }

    return true;
}

void freelist_clear(freelist* list) {
    if (!list || !list->memory) {
        return;
    }

    internal_state* state = list->memory;
    memory_zero(state->nodes, sizeof(freelist_node) * state->max_entries);

    // state->head = &state->nodes[0];
    state->head->offset = 0;
    state->head->size = state->total_size;
    state->head->next = nullptr;
}

u64 freelist_free_space(freelist* list) {
    if (!list | !list->memory) {
        return 0;
    }

    u64 running_total = 0;
    internal_state* state = list->memory;
    freelist_node* node = state->head;
    while (node) {
        running_total += node->size;
        node = node->next;
    }

    return running_total;
}

static freelist_node* get_node(freelist* list) {
    internal_state* state = list->memory;
    for (u64 i = 1; i < state->max_entries; ++i) {
        if (state->nodes[i].size == 0) {
            state->nodes[i].next = nullptr;
            state->nodes[i].offset = 0;
            return &state->nodes[i];
        }
    }

    return nullptr;
}

static void return_node(freelist_node* node) {
    node->offset = 0;
    node->size = 0;
    node->next = nullptr;
}