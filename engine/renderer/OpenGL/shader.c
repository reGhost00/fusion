#ifdef FU_RENDERER_TYPE_VKdss
#include "core/logger.h"
#include "core/memory.h"

#include "misc.inner.h"

static struct _vk_fns_t {
#ifdef FU_ENABLE_MESSAGE_CALLABLE
    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT;
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;
#endif
#ifdef FU_USE_HOST_COPY
    PFN_vkTransitionImageLayoutEXT vkTransitionImageLayoutEXT;
    PFN_vkCopyMemoryToImageEXT vkCopyMemoryToImageEXT;
#endif
} vkfns;

void tvk_fns_init_instance(VkInstance instance)
{
#ifdef FU_ENABLE_MESSAGE_CALLABLE
    vkfns.vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    vkfns.vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
#endif
#ifdef FU_USE_HOST_COPY
    vkfns.vkTransitionImageLayoutEXT = (PFN_vkTransitionImageLayoutEXT)vkGetInstanceProcAddr(instance, "vkTransitionImageLayoutEXT");
    vkfns.vkCopyMemoryToImageEXT = (PFN_vkCopyMemoryToImageEXT)vkGetInstanceProcAddr(instance, "vkCopyMemoryToImageEXT");
#endif
}

#ifdef FU_ENABLE_MESSAGE_CALLABLE
VkResult vkCreateDebugUtilsMessengerEXT(VkInstance inst, const VkDebugUtilsMessengerCreateInfoEXT* info, const VkAllocationCallbacks* cb, VkDebugUtilsMessengerEXT* debugger)
{
    return vkfns.vkCreateDebugUtilsMessengerEXT(inst, info, cb, debugger);
}

void vkDestroyDebugUtilsMessengerEXT(VkInstance inst, VkDebugUtilsMessengerEXT debugger, const VkAllocationCallbacks* cb)
{
    vkfns.vkDestroyDebugUtilsMessengerEXT(inst, debugger, cb);
}
#endif
#ifdef FU_USE_HOST_COPY
VkResult vkTransitionImageLayoutEXT(VkDevice device, uint32_t transitionCount, const VkHostImageLayoutTransitionInfoEXT* pTransitions)
{
    return vkfns.vkTransitionImageLayoutEXT(device, transitionCount, pTransitions);
}
VkResult vkCopyMemoryToImageEXT(VkDevice device, const VkCopyMemoryToImageInfoEXT* pInfo)
{
    return vkfns.vkCopyMemoryToImageEXT(device, pInfo);
}
#endif
// static
static struct _def_allocator {
    VmaAllocator defAllocator;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
} _defAlloc;
void tvk_buffer_allocator_destroy()
{
    vmaDestroyAllocator(_defAlloc.defAllocator);
}

bool tvk_buffer_allocator_init(TSurface* sf)
{
    VmaAllocatorCreateInfo allocInfo = {
        .flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT,
        .physicalDevice = T_PHYSICAL_DEVICE(sf),
        .device = T_DEVICE(sf),
        .pAllocationCallbacks = &defCustomAllocator,
        .instance = sf->instance,
        .vulkanApiVersion = VK_API_VERSION_1_3
    };
    if (vmaCreateAllocator(&allocInfo, &_defAlloc.defAllocator))
        return false;
    _defAlloc.device = T_DEVICE(sf);
    _defAlloc.physicalDevice = T_PHYSICAL_DEVICE(sf);
    return true;
}

bool tvk_image_view_init(VkDevice device, VkFormat format, VkImage image, VkImageView* view)
{
    VkImageViewCreateInfo viewInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .components = { VK_COMPONENT_SWIZZLE_IDENTITY },
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1 }
    };
    return !vkCreateImageView(device, &viewInfo, &defCustomAllocator, view);
}
//
//  同步对象
TVKFence* tvk_fence_new(VkDevice device, uint32_t count, bool signaled)
{
    fu_return_val_if_fail(device && count, NULL);
    VkFenceCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
    };
    if (signaled)
        info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    VkFence* fences = (VkFence*)fu_malloc(sizeof(VkFence) * count);
    for (uint32_t i = 0; i < count; i++) {
        if (FU_LIKELY(!vkCreateFence(device, &info, &defCustomAllocator, &fences[i])))
            continue;
        fu_error("Failed to create fence\n");
        fu_free(fences);
        return NULL;
    }
    TVKFence* rev = (TVKFence*)fu_malloc(sizeof(TVKFence));
    rev->fences = fu_steal_pointer(&fences);
    rev->device = device;
    rev->count = count;
    return rev;
}

void tvk_fence_free(TVKFence* fence)
{
    if (FU_UNLIKELY(!fence))
        return;
    for (uint32_t i = 0; i < fence->count; i++)
        vkDestroyFence(fence->device, fence->fences[i], &defCustomAllocator);
    fu_free(fence->fences);
    fu_free(fence);
}

/**
 * @brief 检查特定格式是否支持主机复制
 * VK_EXT_host_image_copy
 * @param dev
 * @param format
 * @return true
 * @return false
 */
bool tvk_check_format_support_transfer(VkPhysicalDevice dev, VkFormat format)
{
    VkFormatProperties3 formatProperties3 = {
        .sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_3
    };
    VkFormatProperties2 formatProperties2 = {
        .sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2,
        .pNext = &formatProperties3
    };
    vkGetPhysicalDeviceFormatProperties2(dev, format, &formatProperties2);
    return formatProperties3.optimalTilingFeatures & VK_FORMAT_FEATURE_2_HOST_IMAGE_TRANSFER_BIT_EXT;
}

//
//  命令
/**
 * @brief 创建一次性命令
 *
 * @param device
 * @param pool
 * @return TCommandBuffer*
 */
TCommandBuffer* tvk_command_buffer_new_once(VkDevice device, VkCommandPool pool)
{
    fu_print_func_name();
    fu_return_val_if_fail(device && pool, NULL);
    VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandPool = pool,
        .commandBufferCount = 1
    };

    TCommandBuffer* cmdf = (TCommandBuffer*)fu_malloc(sizeof(TCommandBuffer));
    if (FU_UNLIKELY(vkAllocateCommandBuffers(device, &allocInfo, &cmdf->buffer))) {
        fu_error("Failed to allocate command buffers\n");
        fu_free(cmdf);
        return NULL;
    }
    cmdf->device = device;
    cmdf->pool = pool;

    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    vkBeginCommandBuffer(cmdf->buffer, &beginInfo);

    return cmdf;
}
/**
 * @brief 提交一次性命令
 * 如果 fence 不为 NULL 提交完成后返回（异步）
 * 否则等待队列空闲，然后释放（同步）
 * @param cmdf
 * @param queue
 * @param fence 可选，如果提供表示异步
 */
TCommandBuffer* tvk_command_buffer_submit_free(TCommandBuffer* cmdf, VkQueue queue, VkFence fence)
{
    fu_return_val_if_fail(cmdf && queue, NULL);
    vkEndCommandBuffer(cmdf->buffer);

    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmdf->buffer
    };

    if (fence) {
        vkQueueSubmit(queue, 1, &submitInfo, fence);
        return cmdf;
    }
    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);
    vkFreeCommandBuffers(cmdf->device, cmdf->pool, 1, &cmdf->buffer);
    fu_free(cmdf);
    return NULL;
}

void tvk_command_buffer_free(TCommandBuffer* cmdf)
{
    if (FU_UNLIKELY(!cmdf))
        return;
    vkFreeCommandBuffers(cmdf->device, cmdf->pool, 1, &cmdf->buffer);
    fu_free(cmdf);
}
//
//  通用
//  统一缓冲

void t_uniform_destroy(TUniform* buffer)
{
    fu_return_if_fail(buffer);
    vmaDestroyBuffer(_defAlloc.defAllocator, buffer->buffer, buffer->alloc);
}

void t_image_destroy(TImage* buffer)
{
    fu_return_if_fail(buffer);
    vkDestroySampler(_defAlloc.device, buffer->sampler, &defCustomAllocator);
    vkDestroyImageView(_defAlloc.device, buffer->view, &defCustomAllocator);
    vmaDestroyImage(_defAlloc.defAllocator, buffer->image, buffer->alloc);
}

bool t_uniform_init(VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags, TUniform* buffer)
{
    VkBufferCreateInfo bufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };
    VmaAllocationCreateInfo allocInfo = {
        .usage = VMA_MEMORY_USAGE_AUTO,
        .flags = flags
    };
    if (vmaCreateBuffer(_defAlloc.defAllocator, &bufferInfo, &allocInfo, &buffer->buffer, &buffer->alloc, &buffer->allocInfo)) {
        fu_error("Failed to create buffer\n");
        return false;
    }
    return true;
}

bool t_uniform_init_with_data(VkDeviceSize size, const void* data, VkBufferUsageFlags usage, TUniform* buffer)
{
    if (!t_uniform_init(size, usage, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT, buffer))
        return false;
    if (data)
        memcpy(buffer->allocInfo.pMappedData, data, size);

    return true;
}

void t_uniform_free(TUniform* buffer)
{
    t_uniform_destroy(buffer);
    fu_free(buffer);
}
/**
 * @brief 创建暂存缓冲
 * 暂存专用缓冲，没有设备地址
 * @param size
 * @param data
 * @return TUniform*
 */
TUniform* t_uniform_new_staging(VkDeviceSize size, const void* data)
{
    TUniform* rev = (TUniform*)fu_malloc(sizeof(TUniform));
    VkBufferCreateInfo buffInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };
    VmaAllocationCreateInfo allocInfo = {
        .usage = VMA_MEMORY_USAGE_AUTO,
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT
    };
    if (FU_UNLIKELY(vmaCreateBuffer(_defAlloc.defAllocator, &buffInfo, &allocInfo, &rev->buffer, &rev->alloc, &rev->allocInfo))) {
        fu_error("Failed to create buffer\n");
        fu_free(rev);
        return NULL;
    }
    memcpy(rev->allocInfo.pMappedData, data, size);
    return rev;
}
/**
 * @brief 转换图像布局
 *  如果 fence 为 NULL ，等待命令完成并释放，返回 NULL
 *  如果 fence 不为 NULL，则返回命令对象
 * @param pool
 * @param queue
 * @param image
 * @param format
 * @param oldLayout
 * @param newLayout
 * @param fence
 * @return TCommandBuffer*
 */
static TCommandBuffer* _t_image_transition_layout(VkCommandPool pool, VkQueue queue, TImage* image, VkImageLayout layout, VkFence fence)
{
    TCommandBuffer* cmdf = tvk_command_buffer_new_once(_defAlloc.device, pool);
    VkImageSubresourceRange subresource_range = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .levelCount = 1,
        .layerCount = 1
    };
    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = image->layout,
        .newLayout = layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image->image,
        .subresourceRange = subresource_range
    };
    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (VK_IMAGE_LAYOUT_UNDEFINED == image->layout && VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL == layout) {
        // barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL == image->layout && VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL == layout) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        fu_critical("unsupported layout transition!");
        return NULL;
    }

    vkCmdPipelineBarrier(cmdf->buffer, sourceStage, destinationStage, 0, 0, NULL, 0, NULL, 1, &barrier);
    image->layout = layout;
    return tvk_command_buffer_submit_free(cmdf, queue, fence);
}

static TCommandBuffer* _t_image_copy(TUniform* src, TImage* dst, VkCommandPool pool, VkQueue queue, VkFence fence)
{
    TCommandBuffer* cmdf = tvk_command_buffer_new_once(_defAlloc.device, pool);
    VkBufferImageCopy region = {
        .imageSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .layerCount = 1 },
        .imageExtent = dst->extent
    };
    vkCmdCopyBufferToImage(cmdf->buffer, src->buffer, dst->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    return tvk_command_buffer_submit_free(cmdf, queue, fence);
}
/**
 * @brief 初始化纹理专用缓存
 * 初始布局 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
 * 纹理格式 VK_FORMAT_R8G8B8A8_SRGB，高优先级
 * @param size
 * @param usage
 * @param buffer
 * @return true
 * @return false
 */
bool t_image_init(VkFormat format, FUSize* size, VkImageUsageFlags usage, TImage* buffer)
{
    VkImageCreateInfo imageInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format, // VK_FORMAT_R8G8B8A8_SRGB,
        .extent = {
            .width = size->width,
            .height = size->height,
            .depth = 1 },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = usage,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };
    VmaAllocationCreateInfo allocInfo = {
        .usage = VMA_MEMORY_USAGE_AUTO,
        .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        .priority = 1.0f
    };
    if (FU_UNLIKELY(vmaCreateImage(_defAlloc.defAllocator, &imageInfo, &allocInfo, &buffer->image, &buffer->alloc, &buffer->allocInfo))) {
        fu_error("Failed to create image\n");
        return false;
    }
    memcpy(&buffer->extent, &imageInfo.extent, sizeof(VkExtent3D));
    buffer->layout = imageInfo.initialLayout;
    return true;
}

bool t_image_init_with_data(VkCommandPool pool, VkQueue queue, VkFormat format, FUSize* size, const void* data, VkImageUsageFlags usage, TImage* buffer)
{
#ifdef FU_USE_HOST_COPY
    if (FU_UNLIKELY(!t_image_init(format, size, usage | VK_IMAGE_USAGE_HOST_TRANSFER_BIT_EXT, buffer)))
        return false;

    VkHostImageLayoutTransitionInfoEXT transitionInfo = {
        .sType = VK_STRUCTURE_TYPE_HOST_IMAGE_LAYOUT_TRANSITION_INFO_EXT,
        .image = buffer->image,
        .oldLayout = buffer->layout,
        .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1 }
    };
    if (FU_UNLIKELY(vkTransitionImageLayoutEXT(_defAlloc.device, 1, &transitionInfo))) {
        fu_error("Failed to transition image layout\n");
        t_image_destroy(buffer);
        return false;
    }
    VkMemoryToImageCopyEXT mipCopy = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_TO_IMAGE_COPY_EXT,
        .pHostPointer = data,
        .imageSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .layerCount = 1 },
        .imageExtent = { .width = size->width, .height = size->height, .depth = 1 },
    };
    VkCopyMemoryToImageInfoEXT copyInfo = {
        .sType = VK_STRUCTURE_TYPE_COPY_MEMORY_TO_IMAGE_INFO_EXT,
        .dstImage = buffer->image,
        .dstImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .regionCount = 1,
        .pRegions = &mipCopy
    };

    if (FU_UNLIKELY(vkCopyMemoryToImageEXT(_defAlloc.device, &copyInfo))) {
        fu_error("Failed to copy memory to image\n");
        t_image_destroy(buffer);
        return false;
    }
    if (FU_UNLIKELY(!tvk_image_view_init(_defAlloc.device, format, buffer->image, &buffer->view))) {
        fu_error("Failed to create image view\n");
        t_image_destroy(buffer);
        return false;
    }
    return true;
#else
    if (FU_UNLIKELY(!t_image_init(size, usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT, buffer)))
        return false;

    VkDeviceSize imageSize = size->width * size->height * 4;
    TUniform* stage = t_uniform_new_staging(imageSize, data);
    if (FU_UNLIKELY(!stage))
        return false;
    TVKFence* fence = tvk_fence_new(_defAlloc.device, 3, false);
    TCommandBuffer* cmdf1 = _t_image_transition_layout(pool, queue, buffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, fence->fences[0]);
    TCommandBuffer* cmdf2 = _t_image_copy(stage, buffer, pool, queue, fence->fences[1]);
    TCommandBuffer* cmdf3 = _t_image_transition_layout(pool, queue, buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, fence->fences[2]);
    if (FU_UNLIKELY(!tvk_image_view_init(_defAlloc.device, VK_FORMAT_R8G8B8A8_SRGB, buffer->image, &buffer->view))) {
        fu_error("Failed to create image view\n");
        t_image_destroy(buffer);
        return false;
    }
    vkWaitForFences(_defAlloc.device, 3, fence->fences, true, ~0UL);
    tvk_command_buffer_free(cmdf3);
    tvk_command_buffer_free(cmdf2);
    tvk_command_buffer_free(cmdf1);
    tvk_fence_free(fence);
    t_uniform_free(stage);
#endif
    return true;
}

bool t_image_init_sampler(TImage* image, VkFilter filter, VkSamplerMipmapMode minimap, VkSamplerAddressMode addr)
{
    VkPhysicalDeviceProperties properties = {};
    vkGetPhysicalDeviceProperties(_defAlloc.physicalDevice, &properties);

    VkSamplerCreateInfo samplerInfo = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = filter,
        .minFilter = filter,
        .mipmapMode = minimap,
        .addressModeU = addr,
        .addressModeV = addr,
        .addressModeW = addr,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = properties.limits.maxSamplerAnisotropy,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK
    };
    if (FU_UNLIKELY(vkCreateSampler(_defAlloc.device, &samplerInfo, &defCustomAllocator, &image->sampler))) {
        fu_error("Failed to create image sampler\n");
        return false;
    }
    return true;
}

//
//  物理设备
//
/**
 * 枚举物理设备
 * 仅包含设备信息，不含支持队列信息
 * @param cnt 如果指针指向有值，不枚举数量
 * */
TPhysicalDevice* t_physical_device_enumerate(VkInstance inst, uint32_t* cnt)
{
    fu_return_val_if_fail(cnt, NULL);
    if (!*cnt) {
        vkEnumeratePhysicalDevices(inst, cnt, NULL);
        if (FU_UNLIKELY(!*cnt)) {
            fu_error("No physical devices found!\n");
            return NULL;
        }
    }

    VkPhysicalDeviceDescriptorBufferPropertiesEXT descriptorBufferProperties = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT,
    };
    VkPhysicalDeviceProperties2 deviceProperties2 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
        .pNext = &descriptorBufferProperties,
    };
    VkPhysicalDevice devices[*cnt];
    TPhysicalDevice* rev = (TPhysicalDevice*)fu_malloc0(*cnt * sizeof(TPhysicalDevice));
    vkEnumeratePhysicalDevices(inst, cnt, devices);
    for (uint32_t i = 0; i < *cnt; i++) {
        vkGetPhysicalDeviceProperties2(devices[i], &deviceProperties2);
        vkGetPhysicalDeviceFeatures(devices[i], &rev[i].features);
        memcpy(&rev[i].properties, &deviceProperties2.properties, sizeof(VkPhysicalDeviceProperties));
        rev[i].device = fu_steal_pointer(&devices[i]);
        rev[i].deviceCount = *cnt;
        rev[i].deviceIndex = i;
    }
    return rev;
}

/**
 * @brief 用于从一个已有的物理设备拷贝初始化
 * 用于嵌入保存物理设备
 * @param dst
 * @param src
 */
void t_physical_device_init_with(TPhysicalDevice* dst, const TPhysicalDevice* src)
{
    memcpy(dst, src, sizeof(TPhysicalDevice));
    dst->extensions = (VkExtensionProperties*)fu_memdup(src->extensions, dst->extensionCount * sizeof(VkExtensionProperties));
    dst->queueFamilyGraphics = (TQueueFamilyProperties*)fu_memdup(src->queueFamilyGraphics, dst->queueFamilyCount * sizeof(TQueueFamilyProperties));
    dst->queueFamilyCompute = (TQueueFamilyProperties*)fu_memdup(src->queueFamilyCompute, dst->queueFamilyCount * sizeof(TQueueFamilyProperties));
    dst->queueFamilyTransfer = (TQueueFamilyProperties*)fu_memdup(src->queueFamilyTransfer, dst->queueFamilyCount * sizeof(TQueueFamilyProperties));
    dst->queueFamilyProperties = (TQueueFamilyProperties*)fu_memdup(src->queueFamilyProperties, dst->queueFamilyCount * sizeof(TQueueFamilyProperties));
    dst->deviceCount = 1;
}

/** 释放物理设备信息 */
void t_physical_device_free(TPhysicalDevice* dev)
{
    if (!dev)
        return;
    for (uint32_t i = 0; i < dev->deviceCount; i++) {
        fu_free(dev[i].extensions);
        fu_free(dev[i].queueFamilyProperties);
        fu_free(dev[i].queueFamilyGraphics);
        fu_free(dev[i].queueFamilyCompute);
        fu_free(dev[i].queueFamilyTransfer);
    }

    fu_free(dev);
}

/** 释放嵌入的物理设备信息 */
void t_physical_device_destroy(TPhysicalDevice* dev)
{
    fu_free(dev->extensions);
    fu_free(dev->queueFamilyProperties);
    fu_free(dev->queueFamilyGraphics);
    fu_free(dev->queueFamilyCompute);
    fu_free(dev->queueFamilyTransfer);
}

/**
 * @brief 给物理设备打分
 * 仅考虑物理设备的特性以及限制，不考虑队列
 * @param devs
 * @return TPhysicalDevice*
 */
TPhysicalDevice* t_physical_device_rate(TPhysicalDevice* devs)
{
    if (!devs)
        return NULL;
    if (1 >= devs->deviceCount)
        return devs;
    VkBool32* fp;
    uint32_t bestScore = 0, bestDevice = 0, i, j;
    const uint32_t featureCount = sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32);
    for (i = 0; i < devs->deviceCount; i++) {
        fp = (VkBool32*)&devs[i].features;
        for (j = 0; j < featureCount; j++) {
            if (fp[j])
                devs[i].score += 1;
        }
        devs[i].score *= 100;
        devs[i].score += devs[i].properties.limits.maxBoundDescriptorSets;
        devs[i].score += devs[i].properties.limits.maxComputeSharedMemorySize;
        devs[i].score += devs[i].properties.limits.maxImageArrayLayers;
        devs[i].score += devs[i].properties.limits.maxImageDimension2D;
        devs[i].score += devs[i].properties.limits.maxImageDimensionCube;
        if (VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU == devs[i].properties.deviceType)
            devs[i].score += 1000;
        if (devs[i].score > bestScore) {
            bestScore = devs[i].score;
            bestDevice = i;
        }
    }
    return &devs[bestDevice];
}

/**
 * @brief 枚举物理设备的队列族
 * 枚举的队列族信息会被保存物理设备对象中
 * 释放物理设备时会自动处理，无需另外释放
 * @param dev
 * @param sf 目标表面
 * @return true
 * @return false
 */
bool t_physical_device_enumerate_queue_family(TPhysicalDevice* dev, VkSurfaceKHR sf)
{
    fu_return_val_if_fail(dev && sf, false);
    vkGetPhysicalDeviceQueueFamilyProperties(dev->device, &dev->queueFamilyCount, NULL);
    if (FU_UNLIKELY(!dev->queueFamilyCount)) {
        fu_error("No queue families found!\n");
        return false;
    }
    VkBool32 presentSupported;
    uint32_t ig = 0, ic = 0, it = 0;
    dev->queueFamilyGraphics = (TQueueFamilyProperties*)fu_malloc0(dev->queueFamilyCount * sizeof(TQueueFamilyProperties));
    dev->queueFamilyCompute = (TQueueFamilyProperties*)fu_malloc0(dev->queueFamilyCount * sizeof(TQueueFamilyProperties));
    dev->queueFamilyTransfer = (TQueueFamilyProperties*)fu_malloc0(dev->queueFamilyCount * sizeof(TQueueFamilyProperties));
    TQueueFamilyProperties* q = dev->queueFamilyProperties = (TQueueFamilyProperties*)fu_malloc(dev->queueFamilyCount * sizeof(TQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(dev->device, &dev->queueFamilyCount, &q->properties);
    for (uint32_t i = 0; i < dev->queueFamilyCount; i++) {
        if (q[i].properties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            dev->queueFamilyGraphics[ig++] = q[i];
        if (q[i].properties.queueFlags & VK_QUEUE_COMPUTE_BIT)
            dev->queueFamilyCompute[ic++] = q[i];
        if (q[i].properties.queueFlags & VK_QUEUE_TRANSFER_BIT)
            dev->queueFamilyTransfer[it++] = q[i];
        vkGetPhysicalDeviceSurfaceSupportKHR(dev->device, i, sf, &presentSupported);
        q[i].presentSupported = presentSupported;
        q[i].index = i;
    }
    return true;
}

/**
 * @brief 枚举并检查物理设备是否支持指定的扩展
 *
 * @param dev
 * @param name
 * @return true
 * @return false
 */
int t_physical_device_check_support_extension2(TPhysicalDevice* dev, const char* const* const names, const uint32_t cnt)
{
    fu_return_val_if_fail(dev && names && cnt, false);
    if (!dev->extensionCount) {
        vkEnumerateDeviceExtensionProperties(dev->device, NULL, &dev->extensionCount, NULL);
        if (FU_UNLIKELY(!dev->extensionCount))
            return false;
        dev->extensions = (VkExtensionProperties*)fu_malloc0(dev->extensionCount * sizeof(VkExtensionProperties));
        vkEnumerateDeviceExtensionProperties(dev->device, NULL, &dev->extensionCount, dev->extensions);
    }

    for (uint32_t i = 0, j; i < cnt; i++) {
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
// 交换链
//
/** 释放设备的交换链查询信息 */
void t_swapchain_details_free(TSwapchainDetails* details)
{
    if (!details)
        return;
    fu_free(details->supportedFormats);
    fu_free(details->supportedPresentModes);
    fu_free(details);
}
/**
 * @brief 查询设备支持的交换链详细信息
 *
 * @param dev
 * @param surface
 * @return _api_dec*
 */
TSwapchainDetails* t_swapchain_details_query(TPhysicalDevice* dev, VkSurfaceKHR surface)
{
    fu_return_val_if_fail(dev, NULL);
    TSwapchainDetails* details = (TSwapchainDetails*)fu_malloc0(sizeof(TSwapchainDetails));
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev->device, surface, &details->capabilities);
    vkGetPhysicalDeviceSurfaceFormatsKHR(dev->device, surface, &details->formatCount, NULL);
    vkGetPhysicalDeviceSurfacePresentModesKHR(dev->device, surface, &details->presentModeCount, NULL);
    if (FU_UNLIKELY(!(details->formatCount && details->presentModeCount))) {
        fu_error("Failed to query swapchain details!\n");
        t_swapchain_details_free(details);
        return NULL;
    }
    details->supportedFormats = (VkSurfaceFormatKHR*)fu_malloc0(details->formatCount * sizeof(VkSurfaceFormatKHR));
    details->supportedPresentModes = (VkPresentModeKHR*)fu_malloc0(details->presentModeCount * sizeof(VkPresentModeKHR));
    vkGetPhysicalDeviceSurfaceFormatsKHR(dev->device, surface, &details->formatCount, details->supportedFormats);
    vkGetPhysicalDeviceSurfacePresentModesKHR(dev->device, surface, &details->presentModeCount, details->supportedPresentModes);
    return details;
}

//
// 管线库
TPipelineLibrary* t_pipeline_library_new(VkDevice device)
{
    VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO
    };
    TPipelineLibrary* lib = (TPipelineLibrary*)fu_malloc0(sizeof(TPipelineLibrary));
    if (vkCreatePipelineCache(device, &pipelineCacheCreateInfo, &defCustomAllocator, &lib->cache)) {
        fu_warning("Failed to create pipeline cache!\n");
    }
    lib->device = device;
    return lib;
}

void t_pipeline_library_free(TPipelineLibrary* lib)
{
    vkDestroyPipelineCache(lib->device, lib->cache, &defCustomAllocator);
    vkDestroyPipeline(lib->device, lib->vertexInputInterface, &defCustomAllocator);
    vkDestroyPipeline(lib->device, lib->preRasterizationShaders, &defCustomAllocator);
    vkDestroyPipeline(lib->device, lib->fragmentOutputInterface, &defCustomAllocator);
    vkDestroyPipeline(lib->device, lib->fragmentShaders, &defCustomAllocator);
    fu_free(lib);
}

#endif