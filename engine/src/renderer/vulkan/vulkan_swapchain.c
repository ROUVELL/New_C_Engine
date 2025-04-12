#include "vulkan_swapchain.h"

#include "vulkan_device.h"
#include "vulkan_image.h"
#include "vulkan_utils.h"

#include "platform/platform.h"
#include "memory/memory.h"
#include "renderer/renderer_types.h"



static b8 create(vulkan_context* ctx, window* window, renderer_config_flags flags, vulkan_swapchain* out_swapchain);
static void destroy(vulkan_context* ctx, vulkan_swapchain* swapchain);

b8 vulkan_swapchain_create(
    vulkan_context* ctx,
    window* window,
    renderer_config_flags flags,
    vulkan_swapchain* out_swapchain
) {
    return create(ctx, window, flags, out_swapchain);
}

b8 vulkan_swapchain_recreate(
    vulkan_context* ctx,
    window* window,
    vulkan_swapchain* swapchain
) {
    destroy(ctx, swapchain);
    return create(ctx, window, swapchain->flags, swapchain);
}

void vulkan_swapchain_destroy(vulkan_context* ctx, vulkan_swapchain* swapchain) {
    destroy(ctx, swapchain);
}

static b8 create(vulkan_context* ctx, window* window, renderer_config_flags flags, vulkan_swapchain* out_swapchain) {
    window_renderer_state* window_internal = window->renderer_state;
    window_renderer_backend_state* window_backend = window_internal->backend_state;

    VkExtent2D swapchain_extens = { window->width, window->height };

    vulkan_device_query_swapchain_support(
        ctx, ctx->device.physical,
        window_backend->surface,
        &ctx->device.swapchain_support
    );

    b8 found = false;
    for (u32 i = 0; i < ctx->device.swapchain_support.format_count; ++i) {
        VkSurfaceFormatKHR format = ctx->device.swapchain_support.formats[i];
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            out_swapchain->image_format = format;
            found = true;
            break;
        }
    }

    if (!found) {
        out_swapchain->image_format = ctx->device.swapchain_support.formats[0];
    }

    VkFormatProperties format_properties = {0};
    vkGetPhysicalDeviceFormatProperties(ctx->device.physical, out_swapchain->image_format.format, &format_properties);
    out_swapchain->supports_blit_dst = (format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT) != 0;
    out_swapchain->supports_blit_src = (format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT) != 0;
    MDEBUG("Swapchain image format %s be a blit destination.", out_swapchain->supports_blit_dst ? "CAN" : "CANNOT");
    MDEBUG("Swapchain image format %s be a blit source.", out_swapchain->supports_blit_src ? "CAN" : "CANNOT");

    // FIFO and MAILBOX supports vsync, IMMEDIATE does not
    out_swapchain->flags = flags;
    VkPresentModeKHR present_mode;
    if (flags & RENDERER_CONFIG_FLAG_VSYNC_ENABLED_BIT) {
        present_mode = VK_PRESENT_MODE_FIFO_KHR;
        if ((flags & RENDERER_CONFIG_FLAG_POWER_SAVING_BIT) == 0) {
            for (u32 i = 0; i < ctx->device.swapchain_support.present_mode_count; ++i) {
                VkPresentModeKHR mode = ctx->device.swapchain_support.present_modes[i];
                if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                    present_mode = mode;
                    break;
                }
            }
        }
    } else {
        present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    }

    vulkan_swapchain_support_info* swapchain_support = &ctx->device.swapchain_support;
    if (swapchain_support->format_count < 1 || swapchain_support->present_mode_count < 1) {
        if (swapchain_support->formats) {
            memory_free(swapchain_support->formats, sizeof(VkSurfaceFormatKHR) * swapchain_support->format_count, MEMORY_TAG_RENDERER);
        }
        if (swapchain_support->present_modes) {
            memory_free(swapchain_support->present_modes, sizeof(VkPresentModeKHR) * swapchain_support->present_mode_count, MEMORY_TAG_RENDERER);
        }
        MINFO("Required swapchain support not present, skipping device!");
        return false;
    }

    if (ctx->device.swapchain_support.capabilities.currentExtent.width != U32_MAX) {
        swapchain_extens = ctx->device.swapchain_support.capabilities.currentExtent;
    }

    VkExtent2D min = ctx->device.swapchain_support.capabilities.minImageExtent;
    VkExtent2D max = ctx->device.swapchain_support.capabilities.maxImageExtent;
    swapchain_extens.width = MCLAMP(swapchain_extens.width, min.width, max.width);
    swapchain_extens.height = MCLAMP(swapchain_extens.height, min.height, max.height);

    u32 image_count = ctx->device.swapchain_support.capabilities.minImageCount + 1;
    if (ctx->device.swapchain_support.capabilities.maxImageCount > 0 && image_count > ctx->device.swapchain_support.capabilities.maxImageCount) {
        image_count = ctx->device.swapchain_support.capabilities.minImageCount;
    }

    VkSwapchainCreateInfoKHR swapchain_info = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    swapchain_info.surface = window_backend->surface;
    swapchain_info.minImageCount = image_count;
    swapchain_info.imageFormat = out_swapchain->image_format.format;
    swapchain_info.imageColorSpace = out_swapchain->image_format.colorSpace;
    swapchain_info.imageExtent = swapchain_extens;
    swapchain_info.imageArrayLayers = 1;
    swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    if (ctx->device.graphics_queue_index != ctx->device.present_queue_index) {
        u32 queueFamilyIndices[2] = {
            (u32)ctx->device.graphics_queue_index,
            (u32)ctx->device.present_queue_index
        };

        swapchain_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_info.queueFamilyIndexCount = 2;
        swapchain_info.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchain_info.queueFamilyIndexCount = 0;
        swapchain_info.pQueueFamilyIndices = nullptr;
    }

    swapchain_info.preTransform = ctx->device.swapchain_support.capabilities.currentTransform;
    swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_info.presentMode = present_mode;
    swapchain_info.clipped = VK_TRUE;
    swapchain_info.oldSwapchain = nullptr;

    VkResult result = vkCreateSwapchainKHR(ctx->device.logical, &swapchain_info, nullptr, &out_swapchain->handle);
    if (!vulkan_result_is_success(result)) {
        MFATAL("Failed to crate Vulkan swapchain with the error: %s", vulkan_result_string(result, true));
        return false;
    }

    out_swapchain->image_count = 0;
    result = vkGetSwapchainImagesKHR(ctx->device.logical, out_swapchain->handle, &out_swapchain->image_count, nullptr);
    if (!vulkan_result_is_success(result)) {
        MFATAL("Failed to obtain image count from Vulkan swapchain with the error: %s", vulkan_result_string(result, true));
        return false;
    }
    if (!out_swapchain->images) {
        out_swapchain->images = (VkImage*)memory_allocate(sizeof(VkImage) * out_swapchain->image_count, MEMORY_TAG_RENDERER);
    }
    if (!out_swapchain->views) {
        out_swapchain->views = (VkImageView*)memory_allocate(sizeof(VkImageView) * out_swapchain->image_count, MEMORY_TAG_RENDERER);
    }

    result = vkGetSwapchainImagesKHR(ctx->device.logical, out_swapchain->handle, &out_swapchain->image_count, out_swapchain->images);
    if (!vulkan_result_is_success(result)) {
        MFATAL("Failed to obtain images from Vulkan swapchain with the error: %s", vulkan_result_string(result, true));
        return false;
    }

    for (u32 i = 0; i < out_swapchain->image_count; ++i) {
        VkImageViewCreateInfo view_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        view_info.image = out_swapchain->images[i];
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = out_swapchain->image_format.format;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = 1;

        VK_CHECK(vkCreateImageView(ctx->device.logical, &view_info, nullptr, &out_swapchain->views[i]));
    }

    if (!vulkan_device_detect_depth_format(ctx, &ctx->device)) {
        ctx->device.depth_format = VK_FORMAT_UNDEFINED;
        MFATAL("Failed to find a supported depth format!");
    }

    vulkan_image_create(
        ctx,
        swapchain_extens.width,
        swapchain_extens.height,
        ctx->device.depth_format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        true,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        1,
        &out_swapchain->depth_attachment
    );

    return true;
}

static void destroy(vulkan_context* ctx, vulkan_swapchain* swapchain) {
    vkDeviceWaitIdle(ctx->device.logical);

    vulkan_image_destroy(ctx, &swapchain->depth_attachment);

    for (u32 i = 0; i < swapchain->image_count; ++i) {
        vkDestroyImageView(ctx->device.logical, swapchain->views[i], nullptr);
    }

    vkDestroySwapchainKHR(ctx->device.logical, swapchain->handle, nullptr);
}
