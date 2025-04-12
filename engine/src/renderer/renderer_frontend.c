#include "renderer_frontend.h"

#include "renderer_backend.h"
#include "core/logger.h"
#include "memory/memory.h"

typedef struct renderer_system_state {
    renderer_backend_type type;
    renderer_backend* backend;
} renderer_system_state;

static b8 initialized = false;
static renderer_system_state state;

b8 renderer_system_initialize(const char* app_name, u16 window_width, u16 window_height) {
    if (initialized) {
        return false;
    }

    state.backend = memory_allocate(sizeof(renderer_backend), MEMORY_TAG_RENDERER);

    if (!renderer_backend_create(RENDERER_BACKEND_TYPE_VULKAN, state.backend)) {
        MFATAL("Failed to create renderer backend!");
        return false;
    }

    state.type = RENDERER_BACKEND_TYPE_VULKAN;

    if (!state.backend->initialize(app_name, window_width, window_height)) {
        MFATAL("Failed to initialize renderer backend!");
        return false;
    }

    initialized = true;
    return true;
}

void renderer_system_shutdown() {
    if (initialized) {
        state.backend->shutdown();

        memory_free(state.backend, sizeof(renderer_backend), MEMORY_TAG_RENDERER);
        state.backend = nullptr;
        state.type = 0;

        initialized = false;
    }
}

void renderer_on_window_created(struct window* w) {
    state.backend->on_window_created(w);
}

void renderer_on_window_resized(struct window* w, u16 width, u16 height) {
    state.backend->on_window_resized(w, width, height);
}

void renderer_on_window_destroyed(struct window* w) {
    state.backend->on_window_destroyed(w);
}
