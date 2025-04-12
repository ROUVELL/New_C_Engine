#pragma once

#include "vulkan_types.h"

struct window;

b8 vulkan_swapchain_create(
    vulkan_context* ctx,
    struct window* window,
    renderer_config_flags flags,
    vulkan_swapchain* out_swapchain
);

b8 vulkan_swapchain_recreate(
    vulkan_context* ctx,
    struct window* window,
    vulkan_swapchain* swapchain
);

void vulkan_swapchain_destroy(vulkan_context* ctx, vulkan_swapchain* swapchain);