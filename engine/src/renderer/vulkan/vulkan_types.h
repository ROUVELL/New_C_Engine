#pragma once

#include <vulkan/vulkan.h>

#include "renderer/renderer_types.h"
#include "core/asserts.h"
#include "defines.h"


#define VK_CHECK(expr) \
    { \
        MASSERT(expr == VK_SUCCESS); \
    }

typedef struct vulkan_swapchain_support_info {
    VkSurfaceCapabilitiesKHR capabilities;
    u32 format_count;
    VkSurfaceFormatKHR* formats;
    u32 present_mode_count;
    VkPresentModeKHR* present_modes;
} vulkan_swapchain_support_info;

typedef enum vulkan_device_support_flag_bits {
    VULKAN_DEVICE_SUPPORT_FLAG_NONE_BIT = 0x00,
    VULKAN_DEVICE_SUPPORT_FLAG_NATIVE_DYNAMIC_STATE_BIT = 0x01,
    VULKAN_DEVICE_SUPPORT_FLAG_DYNAMIC_STATE_BIT = 0x02,
    VULKAN_DEVICE_SUPPORT_FLAG_LINE_SMOOTH_RASTERISATION_BIT = 0x04
} vulkan_device_support_flag_bits;

typedef u32 vulkan_device_support_flags;

typedef struct vulkan_device {
    u32 api_major;
    u32 api_minor;
    u32 api_patch;

    VkPhysicalDevice physical;
    VkDevice logical;
    vulkan_swapchain_support_info swapchain_support;

    i32 graphics_queue_index;
    i32 present_queue_index;
    i32 transfer_queue_index;

    VkQueue graphics_queue;
    VkQueue present_queue;
    VkQueue transfer_queue;

    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceMemoryProperties memory;

    VkCommandPool graphics_command_pool;

    VkFormat depth_format;
    u8 depth_channels;

    b8 supports_device_local_host_visible;

    vulkan_device_support_flags support_flags;

} vulkan_device;

typedef struct vulkan_image {
    VkImage handle;
    VkImageView view;

    VkDeviceMemory memory;
    VkMemoryPropertyFlags memory_flags;
    VkMemoryRequirements memory_requirements;
    VkFormat format;

    u32 width;
    u32 height;
    u32 mip_levels;
} vulkan_image;

typedef struct vulkan_swapchain {
    VkSurfaceFormatKHR image_format;

    renderer_config_flags flags;

    VkSwapchainKHR handle;

    u32 image_count;
    VkImage* images;
    VkImageView* views;

    vulkan_image depth_attachment;

    b8 supports_blit_dst;
    b8 supports_blit_src;

    u32 image_index;
} vulkan_swapchain;

typedef enum vulkan_command_buffer_state {
    COMMAND_BUFFER_STATE_READY,
    COMMAND_BUFFER_STATE_RECORDING,
    COMMAND_BUFFER_STATE_IN_RENDER_PASS,
    COMMAND_BUFFER_STATE_RECORDING_ENDED,
    COMMAND_BUFFER_STATE_SUBMITTED,
    COMMAND_BUFFER_STATE_NOT_ALLOCATED
} vulkan_command_buffer_state;

typedef struct vulkan_command_buffer {
    VkCommandBuffer handle;
    vulkan_command_buffer_state state;
} vulkan_command_buffer;

typedef struct vulkan_context {
    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_messenger;

    vulkan_device device;

    i32 (*find_memory_index)(u32 type_filter, u32 property_flags);
} vulkan_context;

typedef struct window_renderer_backend_state {
    VkSurfaceKHR surface;
    vulkan_swapchain swapchain;
} window_renderer_backend_state;
