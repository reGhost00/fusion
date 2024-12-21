
#ifdef xxxxf
#include "core/_base.h"
#include "core/logger.h"
#include "misc.inner.h"
#include "warpper.h"
// #undef FU_ENABLE_MESSAGE_CALLABLE
#ifdef FU_ENABLE_MESSAGE_CALLABLE
#include <stdio.h>
static VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_cb(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT* cbd, void* usd)
{
    static char buff[120];
    memset(buff, 0, sizeof(buff));
    if (VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT & type)
        sprintf(buff, "%s[General]", buff);
    if (VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT & type)
        sprintf(buff, "%s[Validation]", buff);
    if (VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT & type)
        sprintf(buff, "%s[Performance]", buff);
    if (VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT & type)
        sprintf(buff, "%s[Device Address Binding]", buff);
    if (VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT == severity) {
        fu_error("%s%s\n", buff, cbd->pMessage);
        return VK_FALSE;
    }
    if (VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT == severity) {
        fu_warning("%s%s\n", buff, cbd->pMessage);
        return VK_FALSE;
    }
    if (VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT == severity) {
        fu_info("%s%s\n", buff, cbd->pMessage);
        return VK_FALSE;
    }
    fprintf(stdout, "%s%s\n", buff, cbd->pMessage);
    return VK_FALSE;
}
#endif

static struct _vk_fns_t {
    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT;
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;
    PFN_vkTransitionImageLayoutEXT vkTransitionImageLayoutEXT;
    PFN_vkCopyMemoryToImageEXT vkCopyMemoryToImageEXT;
} vkfns;

void tvk_fns_init_instance(VkInstance instance)
{
    vkfns.vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    vkfns.vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    vkfns.vkTransitionImageLayoutEXT = (PFN_vkTransitionImageLayoutEXT)vkGetInstanceProcAddr(instance, "vkTransitionImageLayoutEXT");
    vkfns.vkCopyMemoryToImageEXT = (PFN_vkCopyMemoryToImageEXT)vkGetInstanceProcAddr(instance, "vkCopyMemoryToImageEXT");
}

VkResult vkCreateDebugUtilsMessengerEXT(VkInstance inst, const VkDebugUtilsMessengerCreateInfoEXT* info, const VkAllocationCallbacks* cb, VkDebugUtilsMessengerEXT* debugger)
{
    return vkfns.vkCreateDebugUtilsMessengerEXT(inst, info, cb, debugger);
}

void vkDestroyDebugUtilsMessengerEXT(VkInstance inst, VkDebugUtilsMessengerEXT debugger, const VkAllocationCallbacks* cb)
{
    vkfns.vkDestroyDebugUtilsMessengerEXT(inst, debugger, cb);
}

VkResult vkTransitionImageLayoutEXT(VkDevice device, uint32_t transitionCount, const VkHostImageLayoutTransitionInfoEXT* pTransitions)
{
    return vkfns.vkTransitionImageLayoutEXT(device, transitionCount, pTransitions);
}
VkResult vkCopyMemoryToImageEXT(VkDevice device, const VkCopyMemoryToImageInfoEXT* pInfo)
{
    return vkfns.vkCopyMemoryToImageEXT(device, pInfo);
}

bool t_instance_init(const char* name, const uint32_t version, TInstance* instance)
{
    const VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = name,
        .applicationVersion = version,
        .pEngineName = "Fusion Engine",
        .engineVersion = VK_MAKE_VERSION(0, 0, 2),
        .apiVersion = VK_API_VERSION_1_3
    };
    const char* const defInstanceExtensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef VK_USE_PLATFORM_WIN32_KHR
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#elif defined(VK_USE_PLATFORM_XCB_KHR)
        VK_KHR_XCB_SURFACE_EXTENSION_NAME,
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
        VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
#endif
#ifdef FU_ENABLE_MESSAGE_CALLABLE
#ifdef FU_ENABLE_BEST_PRACTICES
        VK_EXT_LAYER_SETTINGS_EXTENSION_NAME,
#endif
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,

    };
    const char* const defLayers[] = {
        "VK_LAYER_KHRONOS_validation"
    };
#ifdef FU_ENABLE_BEST_PRACTICES
    const VkBool32 setting_validate_best_practices = VK_TRUE;
    const VkLayerSettingEXT layerSettings[] = {
        { "VK_LAYER_KHRONOS_validation", "validate_best_practices", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &setting_validate_best_practices }
    };

    const VkLayerSettingsCreateInfoEXT layerInfo = {
        .sType = VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT,
        .settingCount = 1,
        .pSettings = layerSettings
    };
    const VkDebugUtilsMessengerCreateInfoEXT debuggerInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext = &layerInfo,
#else
    const VkDebugUtilsMessengerCreateInfoEXT debuggerInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
#endif
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = vk_debug_cb
    };
    const VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = &debuggerInfo,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = FU_N_ELEMENTS(defLayers),
        .ppEnabledLayerNames = defLayers,
#else
    };
    const VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
#endif
        .enabledExtensionCount = FU_N_ELEMENTS(defInstanceExtensions),
        .ppEnabledExtensionNames = defInstanceExtensions
    };
    if (FU_UNLIKELY(vkCreateInstance(&createInfo, &defCustomAllocator, &instance->instance))) {
        fu_error("failed to create vulkan instance");
        return false;
    }

    tvk_fns_init_instance(instance->instance);
#ifdef FU_ENABLE_MESSAGE_CALLABLE
    if (FU_UNLIKELY(vkCreateDebugUtilsMessengerEXT(instance->instance, &debuggerInfo, &defCustomAllocator, &instance->debugger))) {
        fu_warning("failed to create vulkan debug messenger");
    }
#endif
    return true;
}

void t_instance_destroy(TInstance* instance)
{
#ifdef FU_ENABLE_MESSAGE_CALLABLE
    vkDestroyDebugUtilsMessengerEXT(instance->instance, instance->debugger, &defCustomAllocator);
#endif
    vkDestroyInstance(instance->instance, &defCustomAllocator);
}
#endif