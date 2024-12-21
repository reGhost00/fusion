#include <stdarg.h>

#include "core/_base.h"
#include "core/logger.h"
#include "core/memory.h"
#include "misc.inner.h"
#include "warpper.h"

void vk_descriptor_args_free(TDescriptorArgs* args)
{
    if (FU_UNLIKELY(!args))
        return;
    fu_free(args->bindings);
    fu_free(args->sizes);
    fu_free(args->writes);
    fu_free(args);
}

/**
 * @brief 创建描述符创建参数
 * 描述符创建参数保存了着色器中描述符数量，类型，大小和作用范围
 * 默认影响范围：VK_SHADER_STAGE_ALL_GRAPHICS
 * @param descriptorCount VkDescriptorPoolSize.descriptorCount
 * @param bindingCount
 * @param ... VkDescriptorType
 * @return TDescriptorArgs*
 */
TDescriptorArgs* vk_descriptor_args_new(uint32_t descriptorCount, uint32_t bindingCount, ...)
{
    fu_return_val_if_fail(descriptorCount, NULL);
    va_list ap;
    va_start(ap, count);

    TDescriptorArgs* args = fu_malloc(sizeof(TDescriptorArgs));
    args->bindings = fu_malloc0(sizeof(VkDescriptorType) * bindingCount);
    args->sizes = fu_malloc0(sizeof(VkDescriptorPoolSize) * bindingCount);
    args->writes = fu_malloc0(sizeof(VkWriteDescriptorSet) * bindingCount);
    args->bindingCount = bindingCount;
    args->descriptorCount = descriptorCount;

    for (uint32_t i = 0; i < bindingCount; i++) {
        args->bindings[i].binding = i;
        args->bindings[i].descriptorType = va_arg(ap, VkDescriptorType);
        args->bindings[i].descriptorCount = 1;
        args->bindings[i].stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;

        args->sizes[i].type = args->bindings[i].descriptorType;
        args->sizes[i].descriptorCount = descriptorCount;

        args->writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        args->writes[i].dstBinding = i;
        args->writes[i].descriptorCount = 1;
        args->writes[i].descriptorType = args->bindings[i].descriptorType;
    }
    va_end(ap);

    return args;
}

/**
 * @brief 描述符创建参数默认所有描述符作用范围：VK_SHADER_STAGE_ALL_GRAPHICS
 * 使用此函数更新作用范围
 * @param args
 * @param ... VkShaderStageFlags
 */
void vk_descriptor_args_update_stage_flags(TDescriptorArgs* args, ...)
{
    fu_return_if_fail(args);

    va_list ap;
    va_start(ap, args);
    for (uint32_t i = 0; i < args->bindingCount; i++) {
        args->bindings[i].stageFlags = va_arg(ap, VkShaderStageFlags);
    }
    va_end(ap);
}

/**
 * @brief update VkWriteDescriptorSet
 * @param args
 * @param ... TWriteDescriptorSetInfo
 */
void vk_descriptor_args_update_info(TDescriptorArgs* args, ...)
{
    fu_return_if_fail(args);
    va_list ap;
    va_start(ap, args);
    for (uint32_t i = 0; i < args->bindingCount; i++) {
        if (VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER != args->bindings[i].descriptorType) {
            args->writes[i].pImageInfo = va_arg(ap, TWriteDescriptorSetInfo).imageInfo;
            continue;
        }
        args->writes[i].pBufferInfo = va_arg(ap, TWriteDescriptorSetInfo).bufferInfo;
    }
    va_end(ap);
}

//
//  vulkan descriptor

void t_descriptor_destroy(VkDevice device, TDescriptor* descriptor)
{
    vkDestroyDescriptorSetLayout(device, descriptor->layout, &defCustomAllocator);
    vkDestroyDescriptorPool(device, descriptor->pool, &defCustomAllocator);
}

bool t_descriptor_init(VkDevice device, TDescriptorArgs* args, TDescriptor* descriptor, TWriteDescriptorSetUpdate fnUpdate, void* usd)
{
    fu_return_val_if_fail(device && args, false);

    VkDescriptorSetLayoutCreateInfo layoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = args->bindingCount,
        .pBindings = args->bindings,
    };

    if (FU_UNLIKELY(vkCreateDescriptorSetLayout(device, &layoutInfo, &defCustomAllocator, &descriptor->layout))) {
        fu_error("Failed to create descriptor set layout\n");
        return false;
    }

    VkDescriptorPoolCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .poolSizeCount = args->bindingCount,
        .pPoolSizes = args->sizes,
        .maxSets = args->descriptorCount
    };
    if (FU_UNLIKELY(vkCreateDescriptorPool(device, &info, &defCustomAllocator, &descriptor->pool))) {
        fu_error("Failed to create descriptor pool\n");
        return false;
    }

    VkDescriptorSetLayout* layouts = fu_malloc(sizeof(VkDescriptorSetLayout) * args->descriptorCount);
    VkDescriptorSetAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = descriptor->pool,
        .descriptorSetCount = args->descriptorCount,
        .pSetLayouts = layouts
    };
    for (uint32_t i = 0; i < args->descriptorCount; i++)
        layouts[i] = descriptor->layout;
    descriptor->sets = fu_malloc(sizeof(VkDescriptorSet) * args->descriptorCount);
    if (FU_UNLIKELY(vkAllocateDescriptorSets(device, &allocInfo, descriptor->sets))) {
        fu_error("Failed to allocate descriptor sets\n");
        fu_free(layouts);
        return false;
    }
    for (uint32_t i = 0; i < args->descriptorCount; i++) {
        for (uint32_t j = 0; j < args->bindingCount; j++)
            args->writes[j].dstSet = descriptor->sets[i];
        if (fnUpdate)
            fnUpdate(args->writes, i, usd);
        vkUpdateDescriptorSets(device, args->bindingCount, args->writes, 0, NULL);
    }
    fu_free(layouts);
    return true;
}