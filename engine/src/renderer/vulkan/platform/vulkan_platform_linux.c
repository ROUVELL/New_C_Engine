#include "vulkan_platform.h"

#ifdef PLATFORM_LINUX

#define VK_USE_PLATFORM_XCB_KHR
#include <vulkan/vulkan.h>
#include <xcb/xcb.h>
#include <vulkan/vulkan_xcb.h>

#include "containers/darray.h"
#include "core/logger.h"
#include "memory/memory.h"
#include "platform/platform.h"

#include "renderer/vulkan/vulkan_types.h"


typedef struct linux_handle_info {
    xcb_connection_t* connection;
    xcb_screen_t* screen;
} linux_handle_info;

typedef struct window_platform_state {
    xcb_window_t window;
} window_platform_state;


void platform_get_required_extension_names(const char*** names_darray_ptr) {
    darray_push(*names_darray_ptr, &"VK_KHR_xcb_surface");  // VK_KHR_xlib_surface
}

b8 vulkan_platform_presentation_support(VkPhysicalDevice device, u32 queue_family_index) {
    u64 size = 0;
    platform_get_handle_info(&size, nullptr);
    void* memory = memory_allocate(size, MEMORY_TAG_RENDERER);
    platform_get_handle_info(&size, memory);

    linux_handle_info* handle = (linux_handle_info*)memory;
    if (!handle) {
        memory_free(memory, size, MEMORY_TAG_RENDERER);
        return false;
    }

    VkBool32 result = vkGetPhysicalDeviceXcbPresentationSupportKHR(device, queue_family_index, handle->connection, handle->screen->root_visual);
    memory_free(memory, size, MEMORY_TAG_RENDERER);

    return (b8)result;
}

b8 vulkan_platform_create_vulkan_surface(vulkan_context* ctx, struct window* w) {
    u64 size = 0;
    platform_get_handle_info(&size, nullptr);
    void* memory = memory_allocate(size, MEMORY_TAG_RENDERER);
    platform_get_handle_info(&size, memory);

    linux_handle_info* handle = (linux_handle_info*)memory;
    if (!handle) {
        memory_free(memory, size, MEMORY_TAG_RENDERER);
        return false;
    }

    VkXcbSurfaceCreateInfoKHR info = { VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR };
    info.connection = handle->connection;
    info.window = w->platform_state->window;

    if (vkCreateXcbSurfaceKHR(ctx->instance, &info, nullptr, &w->renderer_state->backend_state->surface) != VK_SUCCESS) {
        MFATAL("Failed to create xcb vulkan surface!");
        memory_free(memory, size, MEMORY_TAG_RENDERER);
        return false;
    }

    memory_free(memory, size, MEMORY_TAG_RENDERER);

    return true;
}

#endif