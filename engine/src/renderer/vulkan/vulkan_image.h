#pragma once

#include "vulkan_types.h"


void vulkan_image_create(
    vulkan_context* ctx,
    // texture_type type,
    u32 width,
    u32 height,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags memory_flags,
    b8 create_view,
    VkImageAspectFlags view_aspect_flags,
    u32 mip_levels,
    vulkan_image* out_image
);

void vulkan_image_destroy(vulkan_context* ctx, vulkan_image* image);

void vulkan_image_transition_layout(
    vulkan_context* ctx,
    vulkan_command_buffer cmd_buffer,
    vulkan_image* image,
    VkFormat format,
    VkImageLayout old_layout,
    VkImageLayout new_layout
);

b8 vulkan_image_mipmaps_generate(
    vulkan_context* ctx,
    vulkan_image* image,
    vulkan_command_buffer* cmd_buffer
);

void vulkan_image_copy_from_buffer(
    vulkan_context* ctx,
    vulkan_image* image,
    VkBuffer buffer,
    u64 offset,
    vulkan_command_buffer* cmd_buffer
);

void vulkan_image_copy_to_buffer(
    vulkan_context* ctx,
    vulkan_image* image,
    VkBuffer buffer,
    vulkan_command_buffer* cmd_buffer
);

void vulkan_image_copy_region_to_buffer(
    vulkan_context* ctx,
    vulkan_image* image,
    VkBuffer buffer,
    u32 x,
    u32 y,
    u32 width,
    u32 height,
    vulkan_command_buffer* cmd_buffer
);