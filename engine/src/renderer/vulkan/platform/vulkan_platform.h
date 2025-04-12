#pragma once

#include "../vulkan_types.h"

struct window;

void platform_get_required_extension_names(const char*** names_darray_ptr);

b8 vulkan_platform_presentation_support(VkPhysicalDevice device, u32 queue_family_index);

b8 vulkan_platform_create_vulkan_surface(vulkan_context* ctx, struct window* w);