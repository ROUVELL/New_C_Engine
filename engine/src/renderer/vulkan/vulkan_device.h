#pragma once

#include "vulkan_types.h"

b8 vulkan_device_create(vulkan_context* ctx);

void vulkan_device_destroy(vulkan_context* ctx);

void vulkan_device_query_swapchain_support(
    vulkan_context* ctx,
    VkPhysicalDevice physical_device,
    VkSurfaceKHR surface,
    vulkan_swapchain_support_info* out_support_info
);

b8 vulkan_device_detect_depth_format(vulkan_context* ctx, vulkan_device* device);