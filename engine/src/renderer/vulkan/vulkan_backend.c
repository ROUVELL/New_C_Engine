#include "vulkan_backend.h"

#include "platform/platform.h"
#include "platform/vulkan_platform.h"
#include "vulkan_device.h"
#include "vulkan_swapchain.h"
#include "vulkan_types.h"
#include "renderer/renderer_types.h"

#include "containers/darray.h"
#include "memory/memory.h"
#include "core/logger.h"


static b8 initialized = false;
static vulkan_context context;

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void* user_data
);

i32 find_memory_index(u32 type_filter, u32 property_flags);

b8 vulkan_renderer_initialize(const char* app_name, u16 window_width, u16 window_height) {
    memory_zero(&context, sizeof(vulkan_context));

    context.find_memory_index = find_memory_index;

    VkApplicationInfo app_info = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
    app_info.pNext = nullptr;
    app_info.pApplicationName = app_name;
    app_info.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    app_info.pEngineName = "MEngine";
    app_info.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_4;

    VkInstanceCreateInfo instance_info = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    instance_info.pNext = nullptr;
    instance_info.flags = 0;
    instance_info.pApplicationInfo = &app_info;

    // Enabled extensions

    const char** required_extensions = darray_reserve(const char*, 3);
    darray_push(required_extensions, &VK_KHR_SURFACE_EXTENSION_NAME);
    platform_get_required_extension_names(&required_extensions);
#if ENGINE_DEBUG
    darray_push(required_extensions, &VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    MTRACE("Required extensions:");
    u64 len = darray_length(required_extensions);
    for (u64 i = 0; i < len; ++i) {
        MTRACE(required_extensions[i]);
    }
#endif

    instance_info.enabledExtensionCount = darray_length(required_extensions);
    instance_info.ppEnabledExtensionNames = required_extensions;

    const char** required_validation_layers = darray_create(const char*);

    // Validation layers
#if ENGINE_DEBUG
    darray_push(required_validation_layers, &"VK_LAYER_KHRONOS_validation");

    // NOTE: Check?
#endif

    instance_info.enabledLayerCount = darray_length(required_validation_layers);
    instance_info.ppEnabledLayerNames = required_validation_layers;

    VK_CHECK(vkCreateInstance(&instance_info, nullptr, &context.instance));
    
    darray_destroy(required_extensions);
    darray_destroy(required_validation_layers);

    // debugger
    context.debug_messenger = VK_NULL_HANDLE;

#if ENGINE_DEBUG
    VkDebugUtilsMessengerCreateInfoEXT debugger_info = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
    debugger_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
    debugger_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT;
    debugger_info.pfnUserCallback = vk_debug_callback;

    PFN_vkCreateDebugUtilsMessengerEXT pfn = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context.instance, "vkCreateDebugUtilsMessengerEXT");
    VK_CHECK(pfn(context.instance, &debugger_info, nullptr, &context.debug_messenger));
#endif

    vulkan_device_create(&context);

    initialized = true;
    return true;
}

void vulkan_renderer_shutdown(void) {
    if (initialized) {
        vulkan_device_destroy(&context);

        if (context.debug_messenger) {
            PFN_vkDestroyDebugUtilsMessengerEXT pfn = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context.instance, "vkDestroyDebugUtilsMessengerEXT");
            pfn(context.instance, context.debug_messenger, nullptr);
        }

        vkDestroyInstance(context.instance, nullptr);

        MTRACE("Vulkan renderer destroyed!");
    }
}

void vulkan_renderer_on_window_created(struct window* w) {
    MTRACE("vulkan_renderer_on_window_created called!");

    w->renderer_state->backend_state = memory_allocate(sizeof(window_renderer_backend_state), MEMORY_TAG_RENDERER);

    if (!vulkan_platform_create_vulkan_surface(&context, w)) {
        MFATAL("Failed to crate Vulkan surface during creation of window %s!", w->name);
        return;
    }

    MTRACE("Vulkan Surface crated successfully!");

    if (!vulkan_swapchain_create(&context, w, 0, &w->renderer_state->backend_state->swapchain)) {
        MFATAL("Failed to create Vulkan swapchain during creation of window %s. See logs for details!", w->name);
        return;
    }

    MTRACE("Vulkan swapchain created successfully!");

    if (!vulkan_device_detect_depth_format(&context, &context.device)) {
        context.device.depth_format = VK_FORMAT_UNDEFINED;
        MFATAL("Failed to find a supported depth format!");
        return;
    }
}

void vulkan_renderer_on_window_resized(struct window* w, u16 width, u16 height) {
    MTRACE("vulkan_renderer_on_window_resized called!");

    // if (w && w->renderer_state && w->renderer_state->backend_state) {
    //     vulkan_swapchain_recreate(&context, w, &w->renderer_state->backend_state->swapchain);
    // }
}

void vulkan_renderer_on_window_destroyed(struct window* w) {
    MTRACE("vulkan_renderer_on_window_destroyed called!");

    if (w && w->renderer_state && w->renderer_state->backend_state) {
        vulkan_swapchain_destroy(&context, &w->renderer_state->backend_state->swapchain);

        vkDestroySurfaceKHR(context.instance, w->renderer_state->backend_state->surface, nullptr);

        memory_free(w->renderer_state->backend_state, sizeof(window_renderer_backend_state), MEMORY_TAG_RENDERER);
        w->renderer_state->backend_state = nullptr;
    }
}


VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void* user_data
) {
    switch (severity) {
        default:
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            MERROR(data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            MWARN(data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            MINFO(data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            MTRACE(data->pMessage);
            break;
    }

    return VK_FALSE;
}

i32 find_memory_index(u32 type_filter, u32 property_flags) {
    VkPhysicalDeviceMemoryProperties properties;
    vkGetPhysicalDeviceMemoryProperties(context.device.physical, &properties);

    for (u32 i = 0; i < properties.memoryTypeCount; ++i) {
        if (type_filter & (1 << i) && (properties.memoryTypes[i].propertyFlags & property_flags) == property_flags) {
            return i;
        }
    }

    MWARN("Unable to find suitable memory type!");
    return -1;
}
