#include "vulkan_image.h"

#include "core/logger.h"
#include "memory/memory.h"
#include "vulkan_utils.h"


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
) {
    mip_levels = MMAX(mip_levels, 1);

    out_image->width = width;
    out_image->height = height;
    out_image->memory_flags = memory_flags;
    out_image->mip_levels = mip_levels;
    out_image->format = format;
    
    VkImageCreateInfo info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    info.imageType = VK_IMAGE_TYPE_2D;  // TODO: Configurable
    info.extent.width = width;
    info.extent.height = height;
    info.extent.depth = 1;
    info.mipLevels = mip_levels;
    info.arrayLayers = 1;
    info.format = format;
    info.tiling = tiling;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    info.usage = usage;
    info.samples = VK_SAMPLE_COUNT_1_BIT;  // TODO: Configurable
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    VK_CHECK(vkCreateImage(ctx->device.logical, &info, nullptr, &out_image->handle));

    vkGetImageMemoryRequirements(ctx->device.logical, out_image->handle, &out_image->memory_requirements);

    i32 memory_type = ctx->find_memory_index(out_image->memory_requirements.memoryTypeBits, memory_flags);
    if (memory_type == -1) {
        MERROR("Required memory type not found! Image not valid!");
    }

    VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    alloc_info.allocationSize = out_image->memory_requirements.size;
    alloc_info.memoryTypeIndex = memory_type;
    VkResult result = vkAllocateMemory(ctx->device.logical, &alloc_info, nullptr, &out_image->memory);
    if (!vulkan_result_is_success(result)) {
        MERROR("Failed to allocate memory for image with the following error: %s", vulkan_result_string(result, true));
        return;
    }

    VK_CHECK(vkBindImageMemory(ctx->device.logical, out_image->handle, out_image->memory, 0));

    b8 is_device_memory = (out_image->memory_flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    memory_allocate_report(out_image->memory_requirements.size, is_device_memory ? MEMORY_TAG_GPU_LOCAL : MEMORY_TAG_VULKAN);

    if (create_view) {
        out_image->view = VK_NULL_HANDLE;

        VkImageViewCreateInfo view_info = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        view_info.image = out_image->handle;
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;  // TODO: Configurable
        view_info.format = format;
        view_info.subresourceRange.aspectMask = view_aspect_flags;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = out_image->mip_levels;
        view_info.subresourceRange.layerCount = 1;
        view_info.subresourceRange.baseArrayLayer = 0;

        VK_CHECK(vkCreateImageView(ctx->device.logical, &view_info, nullptr, &out_image->view));
    }
}

void vulkan_image_destroy(vulkan_context* ctx, vulkan_image* image) {
    if (image->view) {
        vkDestroyImageView(ctx->device.logical, image->view, nullptr);
        image->view = nullptr;
    }

    if (image->memory) {
        vkFreeMemory(ctx->device.logical, image->memory, nullptr);
        image->memory = nullptr;
    }

    if (image->handle) {
        vkDestroyImage(ctx->device.logical, image->handle, nullptr);
        image->handle = nullptr;
    }

    b8 is_device_memory = (image->memory_flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    memory_free_report(image->memory_requirements.size, is_device_memory ? MEMORY_TAG_GPU_LOCAL : MEMORY_TAG_VULKAN);
    memory_zero(&image->memory_requirements, sizeof(VkMemoryRequirements));
}

void vulkan_image_transition_layout(
    vulkan_context* ctx,
    vulkan_command_buffer cmd_buffer,
    vulkan_image* image,
    VkFormat format,
    VkImageLayout old_layout,
    VkImageLayout new_layout
) {
    VkPipelineStageFlags src_stage;
    VkPipelineStageFlags dst_stage;
    
    VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = ctx->device.graphics_queue_index;
    barrier.dstQueueFamilyIndex = ctx->device.graphics_queue_index;
    barrier.image = image->handle;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = image->mip_levels;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;

    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else {
        MFATAL("Unsupported laout transition!");
        return;
    }

    vkCmdPipelineBarrier(
        cmd_buffer.handle,
        src_stage, dst_stage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );
}

b8 vulkan_image_mipmaps_generate(
    vulkan_context* ctx,
    vulkan_image* image,
    vulkan_command_buffer* cmd_buffer
) {
    if (image->mip_levels <= 1) {
        MWARN("Attempted to generate mips for an image that isn't configured for them!");
        return false;
    }

    VkFormatProperties format_properties;
    vkGetPhysicalDeviceFormatProperties(ctx->device.physical, image->format, &format_properties);

    if (!(format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        MWARN("Texture image format does not support linear blitting! Mipmaps cannot be created!");
        return false;
    }

    VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    barrier.image = image->handle;
    barrier.srcQueueFamilyIndex = ctx->device.graphics_queue_index;
    barrier.dstQueueFamilyIndex = ctx->device.graphics_queue_index;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;  // Generate for all layers

    i32 mip_width = (i32)image->width;
    i32 mip_height = (i32)image->height;

    for (u32 i = 1; i < image->mip_levels; ++i) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(
            cmd_buffer->handle,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        VkImageBlit blit = { 0 };
        blit.srcOffsets[0] = (VkOffset3D){0, 0, 0};
        blit.srcOffsets[1] = (VkOffset3D){mip_width, mip_height, 1};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = (VkOffset3D){0, 0, 0};
        blit.dstOffsets[1] = (VkOffset3D){mip_width > 1 ? mip_width / 2 : 1, mip_height > 1 ? mip_height / 2 : 1, 1};
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(
            cmd_buffer->handle,
            image->handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            image->handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR
        );

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
            cmd_buffer->handle,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        if (mip_width > 1) {
            mip_width /= 2;
        }

        if (mip_height > 1) {
            mip_height /= 2;
        }
    }

    barrier.subresourceRange.baseMipLevel = image->mip_levels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(
        cmd_buffer->handle,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    return true;
}

void vulkan_image_copy_from_buffer(
    vulkan_context* ctx,
    vulkan_image* image,
    VkBuffer buffer,
    u64 offset,
    vulkan_command_buffer* cmd_buffer
) {
    VkBufferImageCopy region;
    memory_zero(&region, sizeof(VkBufferImageCopy));
    region.bufferOffset = offset;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset.x = 0;
    region.imageOffset.y = 0;
    region.imageOffset.z = 0;
    region.imageExtent.width = image->width;
    region.imageExtent.height = image->height;
    region.imageExtent.depth = 1;

    vkCmdCopyBufferToImage(
        cmd_buffer->handle,
        buffer, image->handle,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &region
    );
}

void vulkan_image_copy_to_buffer(
    vulkan_context* ctx,
    vulkan_image* image,
    VkBuffer buffer,
    vulkan_command_buffer* cmd_buffer
) {
    VkBufferImageCopy region;
    memory_zero(&region, sizeof(VkBufferImageCopy));
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset.x = 0;
    region.imageOffset.y = 0;
    region.imageOffset.z = 0;
    region.imageExtent.width = image->width;
    region.imageExtent.height = image->height;
    region.imageExtent.depth = 1;

    vkCmdCopyImageToBuffer(
        cmd_buffer->handle,
        image->handle,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        buffer,
        1, &region
    );
}

void vulkan_image_copy_region_to_buffer(
    vulkan_context* ctx,
    vulkan_image* image,
    VkBuffer buffer,
    u32 x,
    u32 y,
    u32 width,
    u32 height,
    vulkan_command_buffer* cmd_buffer
) {
    VkBufferImageCopy region;
    memory_zero(&region, sizeof(VkBufferImageCopy));
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset.x = x;
    region.imageOffset.y = y;
    region.imageOffset.z = 0;
    region.imageExtent.width = width;
    region.imageExtent.height = height;
    region.imageExtent.depth = 1;

    vkCmdCopyImageToBuffer(
        cmd_buffer->handle,
        image->handle,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        buffer,
        1, &region
    );
}