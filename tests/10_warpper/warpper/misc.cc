
#include <cstdio>
#define VMA_IMPLEMENTATION
#include "libs/vk_mem_alloc.h"

#include "core/_base.h"
#include "core/logger.h"
#include "core/memory.h"

#include "misc.inner.h"
#include "warpper.h"
// #include <stb_image.h>
// #include <vector>

// // 加载图像并转换格式
// bool loadImageAndConvertFormat(const char* filename, std::vector<uint8_t>& outImage, int& width, int& height) {
//     unsigned char* data = stbi_load(filename, &width, &height, nullptr, 4); // 4: RGBA
//     if (!data) {
//         return false;
//     }

//     // 创建转换后的图像缓冲区
//     outImage.resize(width * height * 4); // 每个像素 4 字节
//     for (int i = 0; i < width * height; ++i) {
//         // 原始格式是 RGBA，转换为 BGRA
//         outImage[i * 4 + 0] = data[i * 4 + 2]; // B
//         outImage[i * 4 + 1] = data[i * 4 + 1]; // G
//         outImage[i * 4 + 2] = data[i * 4 + 0]; // R
//         outImage[i * 4 + 3] = data[i * 4 + 3]; // A
//     }

//     stbi_image_free(data);
//     return true;
// }
//
//  VMA
static struct _TAllocator {
    VmaAllocator defAllocator;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
} defAlloc;

void vk_allocator_destroy()
{
    vmaDestroyAllocator(defAlloc.defAllocator);
}

bool vk_allocator_init(VkInstance instance, VkDevice device, VkPhysicalDevice physicalDevice)
{
    VmaAllocatorCreateInfo allocInfo = {
        .flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT,
        .physicalDevice = physicalDevice,
        .device = device,
        .pAllocationCallbacks = &defCustomAllocator,
        .instance = instance,
        .vulkanApiVersion = VK_API_VERSION_1_3
    };
    if (vmaCreateAllocator(&allocInfo, &defAlloc.defAllocator))
        return false;
    defAlloc.device = device;
    defAlloc.physicalDevice = physicalDevice;
    return true;
}

//
//  fence

TFence* vk_fence_new(VkDevice device, uint32_t count, bool signaled)
{
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
    TFence* rev = (TFence*)fu_malloc(sizeof(TFence));
    rev->fences = fu_steal_pointer(&fences);
    rev->device = device;
    rev->count = count;
    return rev;
}

void vk_fence_free(TFence* fence)
{
    if (FU_UNLIKELY(!fence))
        return;
    for (uint32_t i = 0; i < fence->count; i++)
        vkDestroyFence(fence->device, fence->fences[i], &defCustomAllocator);
    fu_free(fence->fences);
    fu_free(fence);
}

//
//  command buffer

void vk_command_buffer_free(TCommandBuffer* cmdf)
{
    vkFreeCommandBuffers(cmdf->device, cmdf->pool, 1, &cmdf->buffer);
    fu_free(cmdf);
}

TCommandBuffer* vk_command_buffer_submit_free(TCommandBuffer* cmdf, VkQueue queue, VkFence fence)
{
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

TCommandBuffer* vk_command_buffer_new_once(VkDevice device, VkCommandPool pool)
{
    VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    TCommandBuffer* cmdf = (TCommandBuffer*)fu_malloc(sizeof(TCommandBuffer));
    if (vkAllocateCommandBuffers(device, &allocInfo, &cmdf->buffer)) {
        fu_error("Failed to allocate command buffers!");
        fu_free(cmdf);
        return NULL;
    }

    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    if (vkBeginCommandBuffer(cmdf->buffer, &beginInfo)) {
        fu_error("Failed to begin recording command buffer!");
        vkFreeCommandBuffers(device, pool, 1, &cmdf->buffer);
        fu_free(cmdf);
        return NULL;
    }
    cmdf->device = device;
    cmdf->pool = pool;
    return cmdf;
}

void vk_uniform_buffer_destroy(TUniformBuffer* buff)
{
    vmaDestroyBuffer(defAlloc.defAllocator, buff->buffer, buff->alloc);
}

void vk_uniform_buffer_free(TUniformBuffer* buff)
{
    vmaDestroyBuffer(defAlloc.defAllocator, buff->buffer, buff->alloc);
    fu_free(buff);
}

bool vk_uniform_buffer_init(VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags, TUniformBuffer* tar)
{
    VkBufferCreateInfo bufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };
    VmaAllocationCreateInfo allocInfo = {
        .flags = flags,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };

    if (vmaCreateBuffer(defAlloc.defAllocator, &bufferInfo, &allocInfo, &tar->buffer, &tar->alloc, &tar->allocInfo)) {
        fu_error("Failed to create vertex buffer");
        return false;
    }
    return true;
}

TUniformBuffer* vk_uniform_buffer_new(VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags)
{
    TUniformBuffer* buffer = (TUniformBuffer*)fu_malloc(sizeof(TUniformBuffer));
    if (!vk_uniform_buffer_init(size, usage, flags, buffer)) {
        fu_free(buffer);
        return NULL;
    }
    return buffer;
}

TCommandBuffer* vk_uniform_buffer_copy(VkCommandPool commandPool, VkQueue queue, TUniformBuffer* dst, TUniformBuffer* src, VkFence fence)
{
    TCommandBuffer* cmdf = vk_command_buffer_new_once(defAlloc.device, commandPool);
    if (!cmdf)
        return NULL;
    VkBufferCopy copyRegion = { .size = src->allocInfo.size };
    vkCmdCopyBuffer(cmdf->buffer, src->buffer, dst->buffer, 1, &copyRegion);
    return vk_command_buffer_submit_free(cmdf, queue, fence);
}

TCommandBuffer* vk_uniform_buffer_copy_to_image(VkCommandPool commandPool, VkQueue queue, TImageBuffer* dst, TUniformBuffer* src, VkFence fence)
{
    TCommandBuffer* cmdf = vk_command_buffer_new_once(defAlloc.device, commandPool);
    if (!cmdf)
        return NULL;
    VkImageSubresourceLayers subresource = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .layerCount = 1
    };
    VkExtent3D extent = { .width = dst->extent.width, .height = dst->extent.height, .depth = 1 };
    VkBufferImageCopy copyRegion = {
        .imageSubresource = subresource,
        .imageExtent = extent
    };
    vkCmdCopyBufferToImage(cmdf->buffer, src->buffer, dst->buff.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
    return vk_command_buffer_submit_free(cmdf, queue, fence);
}

//
//  image

void vk_image_buffer_destroy(TImageBuffer* buff)
{
    vmaDestroyImage(defAlloc.defAllocator, buff->buff.image, buff->alloc);
    vkDestroyImageView(defAlloc.device, buff->buff.view, &defCustomAllocator);
}

bool vk_image_view_init(VkDevice device, VkFormat format, VkImage image, VkImageAspectFlags aspect, uint32_t levels, VkImageView* view)
{
    VkImageSubresourceRange subresourceRange = {
        .aspectMask = aspect,
        .levelCount = levels,
        .layerCount = 1
    };
    VkImageViewCreateInfo viewInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .subresourceRange = subresourceRange
    };
    if (vkCreateImageView(device, &viewInfo, &defCustomAllocator, view)) {
        fu_error("Failed to create image view\n");
        return false;
    }
    return true;
}

bool vk_image_buffer_init(VkImageUsageFlags usage, VmaAllocationCreateFlags flags, VkFormat format, VkImageAspectFlags aspect, uint32_t levels, VkSampleCountFlagBits samples, TImageBuffer* tar)
{
    VkImageCreateInfo imageInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = {
            .width = tar->extent.width,
            .height = tar->extent.height,
            .depth = 1 },
        .mipLevels = levels,
        .arrayLayers = 1,
        .samples = samples,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    VmaAllocationCreateInfo allocInfo = {
        .flags = flags,
        .usage = VMA_MEMORY_USAGE_AUTO,
        .priority = 1.0f
    };

    if (vmaCreateImage(defAlloc.defAllocator, &imageInfo, &allocInfo, &tar->buff.image, &tar->alloc, &tar->allocInfo)) {
        fu_error("Failed to create image buffer\n");
        return false;
    }
    if (!vk_image_view_init(defAlloc.device, imageInfo.format, tar->buff.image, aspect, levels, &tar->buff.view)) {
        fu_error("Failed to create image view\n");
        vk_image_buffer_destroy(tar);
        return false;
    }

    tar->layout = VK_IMAGE_LAYOUT_UNDEFINED;
    tar->format = format;
    tar->levels = levels;
    return true;
}

bool vk_image_buffer_init_from_data(const uint8_t* data, const uint32_t width, const uint32_t height, uint32_t levels, VkSampleCountFlagBits samples, TImageBuffer* tar)
{
    tar->extent.width = width;
    tar->extent.height = height;
    if (FU_UNLIKELY(!vk_image_buffer_init(VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_HOST_TRANSFER_BIT_EXT, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, levels, samples, tar)))
        return false;

    VkHostImageLayoutTransitionInfoEXT transitionInfo = {
        .sType = VK_STRUCTURE_TYPE_HOST_IMAGE_LAYOUT_TRANSITION_INFO_EXT,
        .image = tar->buff.image,
        .oldLayout = tar->layout,
        .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = levels,
            .layerCount = 1 }
    };

    if (FU_UNLIKELY(vkTransitionImageLayoutEXT(defAlloc.device, 1, &transitionInfo))) {
        fu_error("Failed to transition image layout!\n");
        vk_image_buffer_destroy(tar);
        return false;
    }

    VkMemoryToImageCopyEXT mipCopy = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_TO_IMAGE_COPY_EXT,
        .pHostPointer = data,
        .imageSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .layerCount = 1 },
        .imageExtent = { .width = tar->extent.width, .height = tar->extent.height, .depth = 1 },
    };
    VkCopyMemoryToImageInfoEXT copyInfo = {
        .sType = VK_STRUCTURE_TYPE_COPY_MEMORY_TO_IMAGE_INFO_EXT,
        .dstImage = tar->buff.image,
        .dstImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .regionCount = 1,
        .pRegions = &mipCopy
    };

    if (FU_UNLIKELY(vkCopyMemoryToImageEXT(defAlloc.device, &copyInfo))) {
        fu_error("Failed to copy memory to image\n");
        vk_image_buffer_destroy(tar);
        return false;
    }
    return true;
}

TCommandBuffer* vk_image_buffer_transition_layout(TImageBuffer* image, VkCommandPool pool, VkQueue queue, VkImageLayout layout, VkFence fence)
{
    TCommandBuffer* cmdf = vk_command_buffer_new_once(defAlloc.device, pool);
    if (!cmdf) {
        fu_error("Failed to allocate command buffers!");
        return NULL;
    }

    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = image->layout,
        .newLayout = layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image->buff.image,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = image->levels,
            .layerCount = 1 }
    };

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (VK_IMAGE_LAYOUT_UNDEFINED == image->layout && VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL == layout) {
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL == image->layout && VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL == layout) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        fu_error("Unsupported layout transition!");
        return NULL;
    }

    image->layout = layout;
    vkCmdPipelineBarrier(cmdf->buffer, sourceStage, destinationStage, 0, 0, NULL, 0, NULL, 1, &barrier);
    return vk_command_buffer_submit_free(cmdf, queue, fence);
}

TCommandBuffer* vk_image_buffer_generate_mipmaps(TImageBuffer* image, VkCommandPool pool, VkQueue queue, VkFence fence)
{
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(defAlloc.physicalDevice, image->format, &formatProperties);
    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        fu_error("texture image format does not support linear blitting!");
        return NULL;
    }

    TCommandBuffer* cmdf = vk_command_buffer_new_once(defAlloc.device, pool);
    if (!cmdf) {
        fu_error("Failed to allocate command buffers!");
        return NULL;
    }

    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image->buff.image,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1 }
    };

    VkImageBlit blit = {
        .srcSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .layerCount = 1 },
        // .srcOffsets = { [1] = { (int)image->extent.width, (int)image->extent.height, 1 } },
        .dstSubresource = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .layerCount = 1 },
        // .dstOffsets = { [1] = { 0, 0, 1 } }
    };

    blit.srcOffsets[1] = { (int)image->extent.width, (int)image->extent.height, 1 };
    blit.dstOffsets[1] = { 0, 0, 1 };
    for (uint32_t i = 1; i < image->levels; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        blit.srcSubresource.mipLevel = i - 1;
        blit.srcOffsets[1].x = image->extent.width >> barrier.subresourceRange.baseMipLevel;
        blit.srcOffsets[1].y = image->extent.height >> barrier.subresourceRange.baseMipLevel;

        blit.dstSubresource.mipLevel = i;
        blit.dstOffsets[1].x = image->extent.width >> i;
        blit.dstOffsets[1].y = image->extent.height >> i;

        vkCmdPipelineBarrier(cmdf->buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);
        vkCmdBlitImage(cmdf->buffer, image->buff.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image->buff.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cmdf->buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);
    }
    barrier.subresourceRange.baseMipLevel = image->levels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(cmdf->buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

    return vk_command_buffer_submit_free(cmdf, queue, fence);
    // tvk_image_buffer_transition_layout(&T_DESC_IMAGE(app), T_CMD_POOL(app), T_QUEUE(app), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, T_BUFF_INIT_FENCEV(app, E_BUFF_INIT_CMD_IMG_TO_READONLY));
}

bool vk_sampler_init(VkFilter filter, VkSamplerMipmapMode mipmap, VkSamplerAddressMode addr, uint32_t maxLod, VkSampler* sampler)
{
    VkPhysicalDeviceProperties properties = {};
    vkGetPhysicalDeviceProperties(defAlloc.physicalDevice, &properties);

    VkSamplerCreateInfo samplerInfo = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = filter,
        .minFilter = filter,
        .mipmapMode = mipmap,
        .addressModeU = addr,
        .addressModeV = addr,
        .addressModeW = addr,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = properties.limits.maxSamplerAnisotropy,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .maxLod = (float)maxLod,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK
    };

    if (FU_UNLIKELY(vkCreateSampler(defAlloc.device, &samplerInfo, &defCustomAllocator, sampler))) {
        fu_error("Failed to create image sampler\n");
        return false;
    }
    return true;
}
