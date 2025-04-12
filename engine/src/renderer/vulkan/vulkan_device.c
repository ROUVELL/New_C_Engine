#include "vulkan_device.h"

#include "platform/vulkan_platform.h"
#include "vulkan_utils.h"

#include "containers/darray.h"
#include "strings/string.h"
#include "memory/memory.h"
#include "core/logger.h"


typedef struct vulkan_physical_device_requirements {
    b8 graphics;
    b8 present;
    b8 compute;
    b8 transfer;
    // darray
    const char** device_extension_names;
    b8 sampler_anisotropy;
    b8 discrete_gpu;
} vulkan_physical_device_requirements;

typedef struct vulkan_physical_device_queue_family_info {
    i32 graphics_family_index;
    i32 present_family_index;
    i32 compute_family_index;
    i32 transfer_family_index;
} vulkan_physical_device_queue_family_info;

static b8 select_physical_device(vulkan_context* ctx);
static b8 physical_device_meets_requirements(
    vulkan_context* ctx,
    VkPhysicalDevice device,
    const VkPhysicalDeviceProperties* properties,
    const VkPhysicalDeviceFeatures* features,
    vulkan_physical_device_requirements* requirements,
    vulkan_physical_device_queue_family_info* out_queue_info
);

b8 vulkan_device_create(vulkan_context* ctx) {
    if (!select_physical_device(ctx)) {
        return false;
    }

    b8 present_shares_graphics_queue = ctx->device.graphics_queue_index == ctx->device.present_queue_index;
    b8 transfer_shares_graphics_queue = ctx->device.graphics_queue_index == ctx->device.transfer_queue_index;
    b8 present_must_share_graphics = false;

    u32 index_count = 1;
    if (!present_shares_graphics_queue) {
        index_count++;
    }
    if (!transfer_shares_graphics_queue) {
        index_count++;
    }

    i32 indices[32];
    u8 index = 0;
    indices[index++] = ctx->device.graphics_queue_index;
    if (!present_shares_graphics_queue) {
        indices[index++] = ctx->device.present_queue_index;
    }
    if (!transfer_shares_graphics_queue) {
        indices[index++] = ctx->device.transfer_queue_index;
    }

    VkDeviceQueueCreateInfo queue_create_infos[32];
    f32 queue_priorities[2] = { 0.9f, 1.0f };

    VkQueueFamilyProperties props[32];
    u32 prop_count;
    vkGetPhysicalDeviceQueueFamilyProperties(ctx->device.physical, &prop_count, nullptr);
    vkGetPhysicalDeviceQueueFamilyProperties(ctx->device.physical, &prop_count, props);

    for (u32 i = 0; i < index_count; ++i) {
        queue_create_infos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_infos[i].queueFamilyIndex = indices[i];
        queue_create_infos[i].queueCount = 1;

        if (present_shares_graphics_queue && indices[i] == ctx->device.present_queue_index) {
            if (props[ctx->device.present_queue_index].queueCount > 1) {
                queue_create_infos[i].queueCount = 2;
            } else {
                present_must_share_graphics = true;
            }
        }

        // TODO: Enable this for a future enhancement
        // if (indices[i] == ctx->device.graphics_queue_index) {
        //     queue_create_infos[i].queueCount = 2;
        // }

        queue_create_infos[i].flags = 0;
        queue_create_infos[i].pNext = nullptr;
        queue_create_infos[i].pQueuePriorities = queue_priorities;
    }

    b8 portability_required = false;
    u32 available_extension_count = 0;
    VkExtensionProperties* available_extensions = nullptr;
    VK_CHECK(vkEnumerateDeviceExtensionProperties(ctx->device.physical, nullptr, &available_extension_count, nullptr));
    if (available_extension_count > 0) {
        available_extensions = memory_allocate(sizeof(VkExtensionProperties) * available_extension_count, MEMORY_TAG_RENDERER);
        VK_CHECK(vkEnumerateDeviceExtensionProperties(ctx->device.physical, nullptr, &available_extension_count, available_extensions));
        for (u32 i = 0; i < available_extension_count; ++i) {
            if (cstr_equal(available_extensions[i].extensionName, "VK_KHR_portability_subset")) {
                MINFO("Adding required extension 'VK_KHR_portability_subset'");
                portability_required = true;
                break;
            }
        }
        memory_free(available_extensions, sizeof(VkExtensionProperties) * available_extension_count, MEMORY_TAG_RENDERER);
    }

    const char* extension_names[6] = {0};
    u32 ext_idx = 0;
    extension_names[ext_idx] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    ext_idx++;

    // Dynamic indexing. NOTE: not needed for 1.2+
    // extension_names[ext_idx] = VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME;
    // ext_idx++;

    if (portability_required) {
        extension_names[ext_idx] = "VK_KHR_portability_subset";
        ext_idx++;
    }

    if (((ctx->device.support_flags & VULKAN_DEVICE_SUPPORT_FLAG_NATIVE_DYNAMIC_STATE_BIT) == 0) &&
        (ctx->device.support_flags & VULKAN_DEVICE_SUPPORT_FLAG_DYNAMIC_STATE_BIT)) {
            extension_names[ext_idx] = VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME;
            ext_idx++;
            extension_names[ext_idx] = VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME;
            ext_idx++;
        }
    
    if (ctx->device.support_flags & VULKAN_DEVICE_SUPPORT_FLAG_LINE_SMOOTH_RASTERISATION_BIT) {
        extension_names[ext_idx] = VK_EXT_LINE_RASTERIZATION_EXTENSION_NAME;
        ext_idx++;
    }

    VkPhysicalDeviceFeatures2 device_features2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR };

    device_features2.features.samplerAnisotropy = ctx->device.features.samplerAnisotropy;
    device_features2.features.fillModeNonSolid = ctx->device.features.fillModeNonSolid;
    device_features2.features.shaderClipDistance = ctx->device.features.shaderClipDistance;
    if (!device_features2.features.shaderClipDistance) {
        MERROR("shaderClipDistance not support by Vulkan device '%s'!", ctx->device.properties.deviceName);
    }

    // Dynamic rendering.
    VkPhysicalDeviceDynamicRenderingFeatures dynamic_rendering_ext = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES};
    dynamic_rendering_ext.dynamicRendering = VK_TRUE;
    device_features2.pNext = &dynamic_rendering_ext;

    // VK_EXT_extended_dynamic_state
    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extended_dynamic_state = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT};
    extended_dynamic_state.extendedDynamicState = VK_TRUE;
    dynamic_rendering_ext.pNext = &extended_dynamic_state;

    // VK_EXT_descriptor_indexing
    VkPhysicalDeviceDescriptorIndexingFeatures descriptor_indexing_features = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT};
    // Partial binding is required for descriptor aliasing.
    descriptor_indexing_features.descriptorBindingPartiallyBound = VK_FALSE; // Don't use this.
    extended_dynamic_state.pNext = &descriptor_indexing_features;

    // Smooth line rasterisation, if supported.
    VkPhysicalDeviceLineRasterizationFeaturesEXT line_rasterization_ext = {0};
    if (ctx->device.support_flags & VULKAN_DEVICE_SUPPORT_FLAG_LINE_SMOOTH_RASTERISATION_BIT) {
        line_rasterization_ext.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_FEATURES_EXT;
        line_rasterization_ext.smoothLines = VK_TRUE;
        descriptor_indexing_features.pNext = &line_rasterization_ext;
    }

    VkDeviceCreateInfo device_create_info = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    device_create_info.queueCreateInfoCount = index_count;
    device_create_info.pQueueCreateInfos = queue_create_infos;
    device_create_info.pEnabledFeatures = nullptr;  // &device_features
    device_create_info.enabledExtensionCount = ext_idx;
    device_create_info.ppEnabledExtensionNames = extension_names;

    // Deprecated and ignored, so pass nothing
    device_create_info.enabledLayerCount = 0;
    device_create_info.ppEnabledLayerNames = nullptr;
    device_create_info.pNext = &descriptor_indexing_features;

    VK_CHECK(vkCreateDevice(ctx->device.physical, &device_create_info, nullptr, &ctx->device.logical));

    vkGetDeviceQueue(ctx->device.logical, ctx->device.graphics_queue_index, 0, &ctx->device.graphics_queue);
    vkGetDeviceQueue(
        ctx->device.logical,
        ctx->device.present_queue_index,
        present_must_share_graphics ? 0 : (ctx->device.graphics_queue_index == ctx->device.present_queue_index) ? 1 : 0,
        &ctx->device.present_queue
    );
    vkGetDeviceQueue(ctx->device.logical, ctx->device.transfer_queue_index, 0, &ctx->device.transfer_queue);

    VkCommandPoolCreateInfo pool_info = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    pool_info.queueFamilyIndex = ctx->device.graphics_queue_index;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK(vkCreateCommandPool(ctx->device.logical, &pool_info, nullptr, &ctx->device.graphics_command_pool));

    MTRACE("Vulkan device created!");

    return true;
}

void vulkan_device_destroy(vulkan_context* ctx) {
    ctx->device.graphics_queue = VK_NULL_HANDLE;
    ctx->device.present_queue = VK_NULL_HANDLE;
    ctx->device.transfer_queue = VK_NULL_HANDLE;

    vkDestroyCommandPool(ctx->device.logical, ctx->device.graphics_command_pool, nullptr);

    vkDestroyDevice(ctx->device.logical, nullptr);

    ctx->device.graphics_queue_index = -1;
    ctx->device.present_queue_index = -1;
    ctx->device.transfer_queue_index = -1;
}

void vulkan_device_query_swapchain_support(
    vulkan_context* ctx,
    VkPhysicalDevice physical_device,
    VkSurfaceKHR surface,
    vulkan_swapchain_support_info* out_support_info
) {
    VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &out_support_info->capabilities);
    if (!vulkan_result_is_success(result)) {
        MFATAL("vkGetPhysicalDeviceSurfaceCapabilitiesKHR failed with message: %s", vulkan_result_string(result, true));
    }

    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &out_support_info->format_count, nullptr));

    if (out_support_info->format_count > 0) {
        if (!out_support_info->formats) {
            out_support_info->formats = memory_allocate(sizeof(VkSurfaceFormatKHR) * out_support_info->format_count, MEMORY_TAG_RENDERER);
        }
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &out_support_info->format_count, out_support_info->formats));
    }

    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &out_support_info->present_mode_count, nullptr));
    
    if (out_support_info->present_mode_count > 0) {
        if (!out_support_info->present_modes) {
            out_support_info->present_modes = memory_allocate(sizeof(VkPresentModeKHR) * out_support_info->present_mode_count, MEMORY_TAG_RENDERER);
        }
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &out_support_info->present_mode_count, out_support_info->present_modes));

    }
}

b8 vulkan_device_detect_depth_format(vulkan_context* ctx, vulkan_device* device) {
    const u32 candidate_count = 2;
    VkFormat candidates[2] = {
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT
    };

    u8 sizes[2] = {
        4,
        3
    };

    u32 flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    for (u32 i = 0; i < candidate_count; ++i) {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(device->physical, candidates[i], &properties);

        if ((properties.linearTilingFeatures & flags) == flags) {
            device->depth_format = candidates[i];
            device->depth_channels = sizes[i];
            return true;
        } else if ((properties.optimalTilingFeatures & flags) == flags) {
            device->depth_format = candidates[i];
            device->depth_channels = sizes[i];
            return true;
        }
    }

    return false;
}

static b8 select_physical_device(vulkan_context* ctx) {
    u32 physical_device_count = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(ctx->instance, &physical_device_count, nullptr));
    if (physical_device_count == 0) {
        MFATAL("Vulkan Device: No devices which support Vulkan were found!");
        return false;
    }

    vulkan_physical_device_requirements requirements = {};
    requirements.graphics = true;
    requirements.present = true;
    requirements.transfer = true;
    // requirements.compute = true;
    requirements.sampler_anisotropy = true;
    requirements.discrete_gpu = true;

    requirements.device_extension_names = darray_create(const char*);
    darray_push(requirements.device_extension_names, &VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    VkPhysicalDevice physical_devices[32];
    VK_CHECK(vkEnumeratePhysicalDevices(ctx->instance, &physical_device_count, physical_devices));
    for (u32 i = 0; i < physical_device_count; ++i) {
        VkPhysicalDeviceProperties2 properties2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
        VkPhysicalDeviceDriverProperties driverProperties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES };
        properties2.pNext = &driverProperties;
        vkGetPhysicalDeviceProperties2(physical_devices[i], &properties2);
        VkPhysicalDeviceProperties properties = properties2.properties;

        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(physical_devices[i], &features);

        VkPhysicalDeviceFeatures2 features2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
        VkPhysicalDeviceExtendedDynamicStateFeaturesEXT dynamic_state_next = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT };
        features2.pNext = &dynamic_state_next;
        VkPhysicalDeviceLineRasterizationFeaturesEXT smooth_line_next = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_FEATURES_EXT };
        dynamic_state_next.pNext = &smooth_line_next;
        vkGetPhysicalDeviceFeatures2(physical_devices[i], &features2);

        VkPhysicalDeviceMemoryProperties memory;
        vkGetPhysicalDeviceMemoryProperties(physical_devices[i], &memory);

        MINFO("Evaluating device: '%s', index %u", properties.deviceName, i);

        // Check support local/host visible combo
        b8 supports_device_local_host_visible = false;
        for (u32 i = 0; i < memory.memoryTypeCount; ++i) {
            if ((memory.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) &&
                (memory.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
                    supports_device_local_host_visible = true;
                    break;
                }
        }

        vulkan_physical_device_queue_family_info queue_info = {};
        b8 result = physical_device_meets_requirements(
            ctx,
            physical_devices[i],
            &properties,
            &features,
            &requirements,
            &queue_info
        );

        if (result) {
            MINFO("Selected device: '%s'", properties.deviceName);

            switch (properties.deviceType) {
                default:
                case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                    MINFO("GPU type is Unknown.");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                    MINFO("GPU type is Integrated.");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                    MINFO("GPU type is Descrete.");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                    MINFO("GPU type is Virtual.");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_CPU:
                    MINFO("GPU type is CPU.");
                    break;
                }
            
            MINFO("GPU Driver version: %s", driverProperties.driverInfo);

            ctx->device.api_major = VK_API_VERSION_MAJOR(properties.apiVersion);
            ctx->device.api_minor = VK_API_VERSION_MINOR(properties.apiVersion);
            ctx->device.api_patch = VK_API_VERSION_PATCH(properties.apiVersion);

            MINFO("Vulkan API version: %d.%d.%d",
                ctx->device.api_major,
                ctx->device.api_minor,
                ctx->device.api_patch);

            for (u32 j = 0; j < memory.memoryHeapCount; ++j) {
                f32 memory_size_gib = (((f32)memory.memoryHeaps[j].size) / 1024.0f / 1024.0f / 1024.0f);
                if (memory.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                    MINFO("Local GPU memory: %.2f GiB", memory_size_gib);
                } else {
                    MINFO("Shared System memory: %.2f GiB", memory_size_gib);
                }
            }

            ctx->device.physical = physical_devices[i];
            ctx->device.graphics_queue_index = queue_info.graphics_family_index;
            ctx->device.present_queue_index = queue_info.present_family_index;
            ctx->device.transfer_queue_index = queue_info.transfer_family_index;

            ctx->device.properties = properties;
            ctx->device.features = features;
            ctx->device.memory = memory;
            ctx->device.supports_device_local_host_visible = supports_device_local_host_visible;

            if (ctx->device.api_major > 1 || ctx->device.api_minor > 2) {
                ctx->device.support_flags |= VULKAN_DEVICE_SUPPORT_FLAG_NATIVE_DYNAMIC_STATE_BIT;
            }

            if (dynamic_state_next.extendedDynamicState) {
                ctx->device.support_flags |= VULKAN_DEVICE_SUPPORT_FLAG_DYNAMIC_STATE_BIT;
            }

            if (smooth_line_next.smoothLines) {
                ctx->device.support_flags |= VULKAN_DEVICE_SUPPORT_FLAG_LINE_SMOOTH_RASTERISATION_BIT;
            }
            break;
        }
    }

    darray_destroy(requirements.device_extension_names);

    if (!ctx->device.physical) {
        MERROR("No physical devices were found which meet the requirements!");
        return false;
    }

    return true;
}

static b8 physical_device_meets_requirements(
    vulkan_context* ctx,
    VkPhysicalDevice device,
    const VkPhysicalDeviceProperties* properties,
    const VkPhysicalDeviceFeatures* features,
    vulkan_physical_device_requirements* requirements,
    vulkan_physical_device_queue_family_info* out_queue_info
) {
    out_queue_info->graphics_family_index = -1;
    out_queue_info->present_family_index = -1;
    out_queue_info->compute_family_index = -1;
    out_queue_info->transfer_family_index = -1;

    // Discrete GPU?
    if (requirements->discrete_gpu) {
        if (properties->deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            return false;
        }
    }

    u32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);
    VkQueueFamilyProperties queue_families[32];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);

    u8 min_transfer_score = 255;
    for (u32 i = 0; i < queue_family_count; ++i) {
        u8 current_transfer_score = 0;

        // Graphics queue?
        if (out_queue_info->graphics_family_index == -1 && queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            out_queue_info->graphics_family_index = i;
            ++current_transfer_score;

            // If also a present queue, this prioritizes grouping of the 2
            b8 supports_present = vulkan_platform_presentation_support(device, i);
            if (supports_present) {
                out_queue_info->present_family_index = i;
                ++current_transfer_score;
            }
        }

        // Compute queue?
        if (queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            out_queue_info->compute_family_index = i;
            ++current_transfer_score;
        }

        // Transfer queue?
        if (queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
            if (current_transfer_score <= min_transfer_score) {
                min_transfer_score = current_transfer_score;
                out_queue_info->transfer_family_index = i;
            }
        }
    }

    if (out_queue_info->present_family_index == -1) {
        for (u32 i = 0; i < queue_family_count; ++i) {
            b8 supports_present = vulkan_platform_presentation_support(device, i);
            if (supports_present) {
                out_queue_info->present_family_index = i;

                if (out_queue_info->present_family_index != out_queue_info->graphics_family_index) {
                    MWARN("Vulkan Device: Differnt queue index used for present vs graphics!");
                }
                break;
            }
        }
    }

    if (
        (!requirements->graphics || (requirements->graphics && out_queue_info->graphics_family_index != -1)) &&
        (!requirements->present || (requirements->present && out_queue_info->present_family_index != -1)) &&
        (!requirements->compute || (requirements->compute && out_queue_info->compute_family_index != -1)) &&
        (!requirements->transfer || (requirements->transfer && out_queue_info->transfer_family_index != -1))
    ) {
        if (requirements->device_extension_names) {
            u32 available_extension_count = 0;
            VkExtensionProperties* avaliable_extensions = nullptr;
            VK_CHECK(vkEnumerateDeviceExtensionProperties(device, nullptr, &available_extension_count, nullptr));
            if (available_extension_count > 0) {
                avaliable_extensions = memory_allocate(sizeof(VkExtensionProperties) * available_extension_count, MEMORY_TAG_RENDERER);
                VK_CHECK(vkEnumerateDeviceExtensionProperties(device, nullptr, &available_extension_count, avaliable_extensions));

                u32 required_extensions_count = darray_length(requirements->device_extension_names);
                for (u32 i = 0; i < required_extensions_count; ++i) {
                    b8 found = false;
                    for (u32 j = 0; j < available_extension_count; ++j) {
                        if (cstr_equal(requirements->device_extension_names[i], avaliable_extensions[j].extensionName)) {
                            found = true;
                            break;
                        }
                    }

                    if (!found) {
                        MINFO("Vulkan Device: Required extension not found: '%s', skipping device!", requirements->device_extension_names[i]);
                        memory_free(avaliable_extensions, sizeof(VkExtensionProperties) * available_extension_count, MEMORY_TAG_RENDERER);
                        return false;
                    }
                }

                memory_free(avaliable_extensions, sizeof(VkExtensionProperties) * available_extension_count, MEMORY_TAG_RENDERER);
            }
        }

        // Sampler anisotropy
        if (requirements->sampler_anisotropy && !features->samplerAnisotropy) {
            MINFO("Device does not support samplerAnysotropy, skipping!");
            return false;
        }

        return true;
    }

    return false;
}