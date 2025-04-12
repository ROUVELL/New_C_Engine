#include "event.h"

#include "containers/darray.h"
#include "memory/memory.h"
#include "core/logger.h"


typedef struct registered_event {
    void* listener;
    PFN_on_event callback;
} registered_event;

#define MAX_EVENT_CODES_COUNT (U16_MAX / 4)

typedef struct event_system_state {
    // darray
    registered_event* registered[MAX_EVENT_CODES_COUNT];
} event_system_state;

static struct event_system_state state;

b8 event_system_initialize() {
    memory_zero(&state, sizeof(event_system_state));
    return true;
}

void event_system_shutdown() {
    for (u16 i = 0; i < SYSTEM_EVENT_MAX_CODE; ++i) {
        if (state.registered[i] != nullptr) {
            darray_destroy(state.registered[i]);
            state.registered[i] = 0;
        }
    }
}

b8 event_register(u16 code, void* listener, PFN_on_event on_event) {
    if (state.registered[code] == nullptr) {
        state.registered[code] = darray_create(registered_event);
    }

    u64 registered_count = darray_length(state.registered[code]);
    for (u64 i = 0; i < registered_count; ++i) {
        if (state.registered[code][i].listener == listener) {
            MERROR("event_register - Don't register same listener to the same event code!");
            return false;
        }
    }

    registered_event event;
    event.listener = listener;
    event.callback = on_event;
    darray_push(state.registered[code], event);

    return true;
}

b8 event_unregister(u16 code, void* listener, PFN_on_event on_event) {
    if (state.registered[code] == nullptr) {
        MERROR("event_unregister - Do not have any listener for that event!");
        return false;
    }

    u64 registered_count = darray_length(state.registered[code]);
    for (u64 i = 0; i < registered_count; ++i) {
        registered_event e = state.registered[code][i];
        if (e.listener == listener && e.callback == on_event) {
            registered_event poped_event;
            darray_pop_at(state.registered[code], i, &poped_event);
            return true;
        }
    }

    return false;
}

b8 event_fire(u16 code, void* sender, event_context ctx) {
    if (state.registered[code] == nullptr) {
        return false;
    }

    u64 registered_count = darray_length(state.registered[code]);
    for (u64 i = 0; i < registered_count; ++i) {
        registered_event e = state.registered[code][i];
        if (e.callback(code, sender, e.listener, ctx)) {
            return true;
        }
    }

    return false;
}