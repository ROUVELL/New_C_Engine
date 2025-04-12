#include "vulkan_platform.h"

#ifdef PLATFORM_WINDOWS

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <vulkan/vulkan_win32.h>

#include "platform/platform.h"
#include "containers/darray.h"
#include "renderer/renderer_types.h"
#include "memory/memory.h"
#include "core/logger.h"

typedef struct win32_handle_info {
    HINSTANCE h_instance;
} win32_handle_info;

typedef struct window_platform_state {
    HWND hwnd;
} window_platform_state;


void platform_get_required_extension_names(const char*** names_darray_ptr) {
    darray_push(*names_darray_ptr, &"VK_KHR_win32_surface");
}

b8 vulkan_platform_presentation_support(VkPhysicalDevice device, u32 queue_family_index) {
    return (b8)vkGetPhysicalDeviceWin32PresentationSupportKHR(device, queue_family_index);
}

b8 vulkan_platform_create_vulkan_surface(vulkan_context* ctx, struct window* w) {
    u64 size = 0;
    platform_get_handle_info(&size, nullptr);
    void* memory = memory_allocate(size, MEMORY_TAG_RENDERER);
    platform_get_handle_info(&size, memory);

    win32_handle_info* handle = (win32_handle_info*)memory;
    if (!handle) {
        memory_free(memory, size, MEMORY_TAG_RENDERER);
        return false;
    }

    VkWin32SurfaceCreateInfoKHR info = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
    info.hinstance = handle->h_instance;
    info.hwnd = w->platform_state->hwnd;

    if (vkCreateWin32SurfaceKHR(ctx->instance, &info, nullptr, &w->renderer_state->backend_state->surface) != VK_SUCCESS) {
        MFATAL("Failed to create win32 vulkan surface!");
        memory_free(memory, size, MEMORY_TAG_RENDERER);
        return false;
    }

    memory_free(memory, size, MEMORY_TAG_RENDERER);

    return true;
}

#endif