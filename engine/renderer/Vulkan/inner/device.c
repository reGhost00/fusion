#ifdef FU_RENDERER_TYPE_VK
#include <string.h>

#include "core/_base.h"
#include "core/logger.h"
#include "core/memory.h"

#include "wrapper.h"

//
//  vulkan instance
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
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
    };
    const char* const defLayers[] = {
        "VK_LAYER_KHRONOS_validation"
    };

    const VkDebugUtilsMessengerCreateInfoEXT debuggerInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
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

#ifdef FU_ENABLE_MESSAGE_CALLABLE
    if (FU_UNLIKELY(vkCreateInstance(&createInfo, &defCustomAllocator, &instance->instance))) {
        fu_error("failed to create vulkan instance");
        return false;
    }

    tvk_fns_init_instance(instance->instance);
    if (FU_UNLIKELY(vkCreateDebugUtilsMessengerEXT(instance->instance, &debuggerInfo, &defCustomAllocator, &instance->debugger))) {
        fu_warning("failed to create vulkan debug messenger");
    }
    return true;
}

void t_instance_destroy(TInstance* instance)
{
    vkDestroyDebugUtilsMessengerEXT(instance->instance, instance->debugger, &defCustomAllocator);
    vkDestroyInstance(instance->instance, &defCustomAllocator);
}
#else
    if (FU_UNLIKELY(vkCreateInstance(&createInfo, &defCustomAllocator, instance))) {
        fu_error("failed to create vulkan instance");
        return false;
    }

    tvk_fns_init_instance(*instance);
    return true;
}

void t_instance_destroy(TInstance instance)
{
    vkDestroyInstance(instance, &defCustomAllocator);
}
#endif
//
//  vulkan physicalDevice

/** 枚举队列族 */
static bool t_physical_device_enumerate_queue_family(TPhysicalDevice* dev, VkSurfaceKHR surface)
{
    vkGetPhysicalDeviceQueueFamilyProperties(dev->physicalDevice, &dev->queueFamilyCount, NULL);
    if (!dev->queueFamilyCount) {
        fu_error("No queue family found\n");
        return false;
    }
    dev->queueFamilies = (TQueueFamilyProperties*)fu_malloc0(sizeof(TQueueFamilyProperties) * dev->queueFamilyCount);
    dev->graphicsQueueFamilies = (TQueueFamilyProperties*)fu_malloc0(sizeof(TQueueFamilyProperties) * dev->queueFamilyCount);
    dev->computeQueueFamilies = (TQueueFamilyProperties*)fu_malloc0(sizeof(TQueueFamilyProperties) * dev->queueFamilyCount);
    dev->transferQueueFamilies = (TQueueFamilyProperties*)fu_malloc0(sizeof(TQueueFamilyProperties) * dev->queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(dev->physicalDevice, &dev->queueFamilyCount, &dev->queueFamilies->properties);
    VkBool32 presentSupported;
    for (uint32_t i = 0; i < dev->queueFamilyCount; i++) {
        if (dev->queueFamilies[i].properties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            dev->graphicsQueueFamilies[dev->graphicsQueueFamilyCount++] = dev->queueFamilies[i];
        if (dev->queueFamilies[i].properties.queueFlags & VK_QUEUE_COMPUTE_BIT)
            dev->computeQueueFamilies[dev->computeQueueFamilyCount++] = dev->queueFamilies[i];
        if (dev->queueFamilies[i].properties.queueFlags & VK_QUEUE_TRANSFER_BIT)
            dev->transferQueueFamilies[dev->transferQueueFamilyCount++] = dev->queueFamilies[i];
        vkGetPhysicalDeviceSurfaceSupportKHR(dev->physicalDevice, i, surface, &presentSupported);
        vkGetPhysicalDeviceProperties(dev->physicalDevice, &dev->properties);
        dev->queueFamilies[i].presentSupported = presentSupported;
        dev->queueFamilies[i].index = i;
    }

    return true;
}

/** 枚举物理设备以及其队列族 */
static TPhysicalDevice* t_physical_device_enumerate(VkInstance instance, VkSurfaceKHR surface, uint32_t* count)
{
    vkEnumeratePhysicalDevices(instance, count, NULL);
    if (!*count) {
        fu_error("No GPU with Vulkan support found\n");
        return NULL;
    }
    VkPhysicalDevice* devices = (VkPhysicalDevice*)fu_malloc0(*count * sizeof(VkPhysicalDevice));
    TPhysicalDevice* rev = (TPhysicalDevice*)fu_malloc0(*count * sizeof(TPhysicalDevice));
    vkEnumeratePhysicalDevices(instance, count, devices);
    for (uint32_t i = 0; i < *count; i++) {
        rev[i].physicalDevice = fu_steal_pointer(&devices[i]);
        vkGetPhysicalDeviceFeatures(rev[i].physicalDevice, &rev[i].features);
        vkGetPhysicalDeviceProperties(rev[i].physicalDevice, &rev[i].properties);
        if (t_physical_device_enumerate_queue_family(&rev[i], surface))
            continue;
        fu_error("failed to enumerate queue family\n");
        fu_free(devices);
        fu_free(rev);
        return NULL;
    }
    fu_free(devices);
    return rev;
}

/** 物理设备打分 */
TPhysicalDevice* t_physical_device_rate(TPhysicalDevice* devs, uint32_t count)
{
    if (!(devs && count))
        return NULL;
    VkBool32* fp;
    uint32_t currScore, bestScore = 0, bestDevice = 0, i, j;
    const uint32_t featureCount = sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32);
    for (i = 0; i < count; i++) {
        if (!(devs[i].graphicsQueueFamilyCount && devs[i].queueFamilies[0].presentSupported))
            continue;
        fp = (VkBool32*)&devs[i].features;
        if (VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU == devs[i].properties.deviceType)
            currScore = 1000;
        else
            currScore = 0;
        for (j = 0; j < featureCount; j++) {
            if (fp[j])
                currScore += 1;
        }
        currScore *= 100;
        currScore += devs[i].properties.limits.maxBoundDescriptorSets;
        currScore += devs[i].properties.limits.maxComputeSharedMemorySize;
        currScore += devs[i].properties.limits.maxImageArrayLayers;
        currScore += devs[i].properties.limits.maxImageDimension2D;
        currScore += devs[i].properties.limits.maxImageDimensionCube;

        if (currScore > bestScore) {
            bestScore = currScore;
            bestDevice = i;
        }
    }
    return &devs[bestDevice];
}

/**
 * 选择合适的队列
 * 虽然规范中并未定义, 但是查看数据库 https://vulkan.gpuinfo.org/
 * 假设必定有一个队列同时支持图形、计算、传输和呈现
 */
static int t_physical_device_select_queue(TPhysicalDevice* dev)
{
    TQueueFamilyProperties* queueFamilyProperties;
    for (uint32_t i = 0; i < dev->queueFamilyCount; i++) {
        queueFamilyProperties = &dev->queueFamilies[i];
        if (!queueFamilyProperties->presentSupported)
            continue;
        if (!(queueFamilyProperties->properties.queueFlags & VK_QUEUE_GRAPHICS_BIT))
            continue;
        if (!(queueFamilyProperties->properties.queueFlags & VK_QUEUE_COMPUTE_BIT))
            continue;
        return i;
    }
    return -1;
}

/** 枚举并检查物理设备支持的扩展 */
static int t_physical_device_check_support_extension(TPhysicalDevice* dev, const char* const* const names, uint32_t count)
{
    vkEnumerateDeviceExtensionProperties(dev->physicalDevice, NULL, &dev->extensionCount, NULL);
    if (!dev->extensionCount) {
        fu_error("failed to enumerate device extension properties\n");
        return 0;
    }
    dev->extensions = (VkExtensionProperties*)fu_malloc0_n(dev->extensionCount, sizeof(VkExtensionProperties));
    vkEnumerateDeviceExtensionProperties(dev->physicalDevice, NULL, &dev->extensionCount, dev->extensions);
    for (uint32_t i = 0, j; i < count; i++) {
        for (j = 0; j < dev->extensionCount; j++) {
            if (!strcmp(names[i], dev->extensions[j].extensionName))
                break;
        }
        if (j >= dev->extensionCount)
            return i;
    }
    return -1;
}
//
//
/** 释放队列 */
static void t_physical_device_free_queue_family(TPhysicalDevice* dev)
{
    if (!dev)
        return;
    fu_free(dev->queueFamilies);
    fu_free(dev->graphicsQueueFamilies);
    fu_free(dev->computeQueueFamilies);
    fu_free(dev->transferQueueFamilies);
}
/** 释放物理设备 */
static void t_physical_device_free(TPhysicalDevice* devices, uint32_t count)
{
    if (!(devices && count))
        return;
    for (uint32_t i = 0; i < count; i++) {
        t_physical_device_free_queue_family(&devices[i]);
    }
    fu_free(devices);
}

/** 物理设备复制构造 */
static void t_physical_device_init_with(TPhysicalDevice* target, TPhysicalDevice* source)
{
    memcpy(target, source, sizeof(TPhysicalDevice));
    target->queueFamilies = (TQueueFamilyProperties*)fu_memdup(source->queueFamilies, sizeof(TQueueFamilyProperties) * source->queueFamilyCount);
    target->graphicsQueueFamilies = (TQueueFamilyProperties*)fu_memdup(source->graphicsQueueFamilies, sizeof(TQueueFamilyProperties) * source->graphicsQueueFamilyCount);
    target->computeQueueFamilies = (TQueueFamilyProperties*)fu_memdup(source->computeQueueFamilies, sizeof(TQueueFamilyProperties) * source->computeQueueFamilyCount);
    target->transferQueueFamilies = (TQueueFamilyProperties*)fu_memdup(source->transferQueueFamilies, sizeof(TQueueFamilyProperties) * source->transferQueueFamilyCount);
}

void t_device_destroy(TDevice* device)
{
    vk_allocator_destroy();
    vkDestroyDevice(device->device, &defCustomAllocator);
    t_physical_device_free_queue_family(&device->physicalDevice);
    fu_free(device->physicalDevice.extensions);
}

bool t_device_init(VkInstance instance, VkSurfaceKHR surface, TDevice* device)
{
    const char* const defDeviceExtensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
        VK_EXT_GRAPHICS_PIPELINE_LIBRARY_EXTENSION_NAME,
        VK_EXT_HOST_IMAGE_COPY_EXTENSION_NAME,
        VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME,
    };
    //  1：枚举所有物理设备，查询其支持的操作和检查是否支持呈现
    //  2：为所有物理设备打分，选择分数最高的设备
    //  3：验证这个设备支持所有所需扩展
    //  4：假设这个设备必定有一个队列族同时支持图形、计算、传输和呈现，找到这个队列族

    uint32_t deviceCount = 0;
    TPhysicalDevice* devices = t_physical_device_enumerate(instance, surface, &deviceCount);
    if (!(devices && deviceCount))
        return false;
    TPhysicalDevice* selectedDevice = t_physical_device_rate(devices, deviceCount);
    int queueIndex = t_physical_device_check_support_extension(selectedDevice, defDeviceExtensions, FU_N_ELEMENTS(defDeviceExtensions));
    bool rev = false;
    if (0 <= queueIndex) {
        fu_error("Device does not support extension: %s\n", defDeviceExtensions[queueIndex]);
        goto out;
    }
    queueIndex = t_physical_device_select_queue(selectedDevice);
    if (0 > queueIndex) {
        fu_error("Failed to find suitable queue family\n");
        goto out;
    }

    const float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queueIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority
    };

    VkPhysicalDeviceMemoryPriorityFeaturesEXT memoryPriorityFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PRIORITY_FEATURES_EXT,
        .memoryPriority = VK_TRUE
    };

    VkPhysicalDeviceHostImageCopyFeaturesEXT hostImageCopyFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_IMAGE_COPY_FEATURES_EXT,
        .pNext = &memoryPriorityFeatures,
        .hostImageCopy = VK_TRUE
    };

    VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT graphicsPipelineLibraryFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GRAPHICS_PIPELINE_LIBRARY_FEATURES_EXT,
        .pNext = &hostImageCopyFeatures,
        .graphicsPipelineLibrary = VK_TRUE
    };

    VkPhysicalDeviceFeatures2 deviceFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &graphicsPipelineLibraryFeatures,
        .features = {
            .robustBufferAccess = VK_TRUE,
            .samplerAnisotropy = VK_TRUE }
    };

    VkDeviceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &deviceFeatures,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueCreateInfo,
        .enabledExtensionCount = FU_N_ELEMENTS(defDeviceExtensions),
        .ppEnabledExtensionNames = defDeviceExtensions
    };

#ifdef FU_ENABLE_MESSAGE_CALLABLE
    const char* const defLayers[] = {
        "VK_LAYER_KHRONOS_validation"
    };
    createInfo.enabledLayerCount = FU_N_ELEMENTS(defLayers);
    createInfo.ppEnabledLayerNames = defLayers;
#endif
    if (vkCreateDevice(selectedDevice->physicalDevice, &createInfo, &defCustomAllocator, &device->device)) {
        fu_error("Failed to create logical device\n");
        goto out;
    }

    vkGetDeviceQueue(device->device, queueIndex, 0, &device->generalQueue);
    t_physical_device_init_with(&device->physicalDevice, selectedDevice);
    rev = vk_allocator_init(instance, device->device, device->physicalDevice.physicalDevice);
    if (VK_SAMPLE_COUNT_4_BIT & selectedDevice->properties.limits.framebufferColorSampleCounts)
        device->sampleCount = VK_SAMPLE_COUNT_4_BIT;
    else if (VK_SAMPLE_COUNT_2_BIT & selectedDevice->properties.limits.framebufferColorSampleCounts)
        device->sampleCount = VK_SAMPLE_COUNT_2_BIT;
    else
        device->sampleCount = VK_SAMPLE_COUNT_1_BIT;
    device->queueFamilyIndex = queueIndex;
    fu_info("Device: %s\n", device->physicalDevice.properties.deviceName);
out:
    t_physical_device_free(devices, deviceCount);
    return rev;
}

#endif