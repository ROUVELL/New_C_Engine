#include "renderer_backend.h"

#include "memory/memory.h"

#include "vulkan/vulkan_backend.h"


b8 renderer_backend_create(renderer_backend_type type, renderer_backend* out_backend) {
    if (type == RENDERER_BACKEND_TYPE_VULKAN) {
        out_backend->initialize = vulkan_renderer_initialize;
        out_backend->shutdown = vulkan_renderer_shutdown;

        out_backend->on_window_created = vulkan_renderer_on_window_created;
        out_backend->on_window_resized = vulkan_renderer_on_window_resized;
        out_backend->on_window_destroyed = vulkan_renderer_on_window_destroyed;
        
        return true;
    }
    // } else if (type == RENDERER_BACKEND_TYPE_OPENGL) {
    //     return true;
    // }

    return false;
}

void renderer_backend_destroy(renderer_backend* backend) {
    memory_zero(backend, sizeof(renderer_backend));
}