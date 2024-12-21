#include <stdarg.h>
#include <stdio.h>

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
    va_start(ap, bindingCount);

    TDescriptorArgs* args = fu_malloc(sizeof(TDescriptorArgs));
    args->bindings = fu_malloc0(sizeof(VkDescriptorSetLayoutBinding) * bindingCount);
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

bool t_pipeline_init_render_pass(TPipeline* pipeline, VkDevice device, uint32_t attachmentCount, const VkAttachmentDescription* attachments, uint32_t subpassCount, const VkSubpassDescription* subpasses, uint32_t dependencyCount, const VkSubpassDependency* dependencies)
{
    const VkRenderPassCreateInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = attachmentCount,
        .pAttachments = attachments,
        .subpassCount = subpassCount,
        .pSubpasses = subpasses,
        .dependencyCount = dependencyCount,
        .pDependencies = dependencies
    };
    if (FU_UNLIKELY(vkCreateRenderPass(device, &renderPassInfo, &defCustomAllocator, &pipeline->renderPass))) {
        fu_error("Failed to create render pass\n");
        return false;
    }
    return true;
}
#ifdef use_warp
/**
 * @brief Create input stage of pipeline
 * return a index of this pipeline in library
 * return -1 if failed
 */
int t_pipeline_new_vertex_input_interface(TPipeline* pipeline, VkDevice device, uint32_t bindingCount, const VkVertexInputBindingDescription* bindings, uint32_t attributeCount, const VkVertexInputAttributeDescription* attributes)
{
    const VkGraphicsPipelineLibraryCreateInfoEXT libraryInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_LIBRARY_CREATE_INFO_EXT,
        .flags = VK_GRAPHICS_PIPELINE_LIBRARY_VERTEX_INPUT_INTERFACE_BIT_EXT
    };

    const VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
    };

    const VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = bindingCount,
        .pVertexBindingDescriptions = bindings,
        .vertexAttributeDescriptionCount = attributeCount,
        .pVertexAttributeDescriptions = attributes
    };

    const VkGraphicsPipelineCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .flags = VK_PIPELINE_CREATE_LIBRARY_BIT_KHR | VK_PIPELINE_CREATE_RETAIN_LINK_TIME_OPTIMIZATION_INFO_BIT_EXT,
        .pNext = &libraryInfo,
        .pInputAssemblyState = &inputAssembly,
        .pVertexInputState = &vertexInputInfo
    };

    VkPipeline target;
    if (FU_UNLIKELY(vkCreateGraphicsPipelines(device, NULL, 1, &createInfo, &defCustomAllocator, &target))) {
        fu_error("Failed to create graphics pipeline @vertexInputStage\n");
        return -1;
    }

    pipeline->library->vertexInputInterfaces = fu_realloc(pipeline->library->vertexInputInterfaces, sizeof(VkPipeline) * (pipeline->library->vertexInputInterfaceCount + 1));
    pipeline->library->vertexInputInterfaces[pipeline->library->vertexInputInterfaceCount] = fu_steal_pointer(&target);
    return pipeline->library->vertexInputInterfaceCount++;
}

/**
 * @brief Create preRasterizationStage of pipeline
 * return a index of this pipeline in library
 * return -1 if failed
 */
int t_pipeline_new_pre_rasterization_shaders(TPipeline* pipeline, VkDevice device, uint32_t stageCount, const VkPipelineShaderStageCreateInfo* stages)
{
    const VkGraphicsPipelineLibraryCreateInfoEXT libraryInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_LIBRARY_CREATE_INFO_EXT,
        .flags = VK_GRAPHICS_PIPELINE_LIBRARY_PRE_RASTERIZATION_SHADERS_BIT_EXT
    };

    // viewport
    const VkPipelineViewportStateCreateInfo viewportState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1
    };

    // rasterizer state
    const VkPipelineRasterizationStateCreateInfo rasterizerState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .lineWidth = 1.0f
    };

    const VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    const VkPipelineDynamicStateCreateInfo dynamicState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = FU_N_ELEMENTS(dynamicStates),
        .pDynamicStates = dynamicStates
    };

    const VkGraphicsPipelineCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &libraryInfo,
        .flags = VK_PIPELINE_CREATE_LIBRARY_BIT_KHR | VK_PIPELINE_CREATE_RETAIN_LINK_TIME_OPTIMIZATION_INFO_BIT_EXT,
        .stageCount = stageCount,
        .pStages = stages,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizerState,
        .pDynamicState = &dynamicState,
        .layout = pipeline->layout,
        .renderPass = pipeline->renderPass
    };

    VkPipeline target;
    if (FU_UNLIKELY(vkCreateGraphicsPipelines(device, NULL, 1, &createInfo, &defCustomAllocator, &target))) {
        fu_error("Failed to create graphics pipeline @preRasterizationShaders\n");
        return -1;
    }

    pipeline->library->preRasterizationShaders = fu_realloc(pipeline->library->preRasterizationShaders, sizeof(VkPipeline) * (pipeline->library->preRasterizationShaderCount + 1));
    pipeline->library->preRasterizationShaders[pipeline->library->preRasterizationShaderCount] = fu_steal_pointer(&target);
    return pipeline->library->preRasterizationShaderCount++;
}

int t_pipeline_init_layout_and_pre_rasterization_shaders(TPipeline* pipeline, VkDevice device, uint32_t setLayoutCount, const VkDescriptorSetLayout* setLayouts, uint32_t stageCount, const VkPipelineShaderStageCreateInfo* stages)
{
    const VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = setLayoutCount,
        .pSetLayouts = setLayouts
    };
    if (FU_UNLIKELY(vkCreatePipelineLayout(device, &pipelineLayoutInfo, &defCustomAllocator, &pipeline->layout))) {
        fu_error("Failed to create pipeline layout @vertexShaderStage\n");
        return -1;
    }
    return t_pipeline_new_pre_rasterization_shaders(pipeline, device, stageCount, stages);
}

/**
 * @brief Create fragmentStage of pipeline
 * return a index of this pipeline in library
 * return -1 if failed
 */
int t_pipeline_new_fragment_shader(TPipeline* pipeline, VkDevice device, uint32_t stageCount, const VkPipelineShaderStageCreateInfo* stages, bool hasDepth)
{
    VkGraphicsPipelineLibraryCreateInfoEXT libraryInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_LIBRARY_CREATE_INFO_EXT,
        .flags = VK_GRAPHICS_PIPELINE_LIBRARY_FRAGMENT_SHADER_BIT_EXT
    };

    // depth stencil state
    VkPipelineDepthStencilStateCreateInfo depthStencilState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
    };

    VkGraphicsPipelineCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &libraryInfo,
        .flags = VK_PIPELINE_CREATE_LIBRARY_BIT_KHR | VK_PIPELINE_CREATE_RETAIN_LINK_TIME_OPTIMIZATION_INFO_BIT_EXT,
        .stageCount = stageCount,
        .pStages = stages,
        // .pDepthStencilState = &depthStencilState,
        .layout = pipeline->layout,
        .renderPass = pipeline->renderPass
    };

    if (hasDepth)
        createInfo.pDepthStencilState = &depthStencilState;

    VkPipeline target;
    if (FU_UNLIKELY(vkCreateGraphicsPipelines(device, NULL, 1, &createInfo, &defCustomAllocator, &target))) {
        fu_error("Failed to create graphics pipeline @fragmentStage\n");
        fu_free(target);
        return -1;
    }

    pipeline->library->fragmentShaders = fu_realloc(pipeline->library->fragmentShaders, sizeof(VkPipeline) * (pipeline->library->fragmentShaderCount + 1));
    pipeline->library->fragmentShaders[pipeline->library->fragmentShaderCount] = fu_steal_pointer(&target);
    return pipeline->library->fragmentShaderCount++;
}
/**
 * @brief Create fragmentOutputInterface of pipeline
 * return a index of this pipeline in library
 * return -1 if failed
 */
int t_pipeline_new_fragment_output_interface(TPipeline* pipeline, VkDevice device, VkSampleCountFlagBits samples, uint32_t attachmentCount, const VkPipelineColorBlendAttachmentState* attachments)
{
    const VkGraphicsPipelineLibraryCreateInfoEXT libraryInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_LIBRARY_CREATE_INFO_EXT,
        .flags = VK_GRAPHICS_PIPELINE_LIBRARY_FRAGMENT_OUTPUT_INTERFACE_BIT_EXT
    };

    // multisample state
    const VkPipelineMultisampleStateCreateInfo multisampleState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = samples
    };

    const VkPipelineColorBlendStateCreateInfo colorBlendState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = attachmentCount,
        .pAttachments = attachments
    };

    const VkGraphicsPipelineCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &libraryInfo,
        .flags = VK_PIPELINE_CREATE_LIBRARY_BIT_KHR | VK_PIPELINE_CREATE_RETAIN_LINK_TIME_OPTIMIZATION_INFO_BIT_EXT,
        .pMultisampleState = &multisampleState,
        .pColorBlendState = &colorBlendState,
        .layout = pipeline->layout,
        .renderPass = pipeline->renderPass
    };

    VkPipeline target;
    if (FU_UNLIKELY(vkCreateGraphicsPipelines(device, NULL, 1, &createInfo, &defCustomAllocator, &target))) {
        fu_error("Failed to create graphics pipeline @fragmentOutputStage\n");
        return -1;
    }

    pipeline->library->fragmentOutputInterfaces = fu_realloc(pipeline->library->fragmentOutputInterfaces, sizeof(VkPipeline) * (pipeline->library->fragmentOutputInterfaceCount + 1));
    pipeline->library->fragmentOutputInterfaces[pipeline->library->fragmentOutputInterfaceCount] = fu_steal_pointer(&target);
    return pipeline->library->fragmentOutputInterfaceCount++;
}

bool t_pipeline_init(TPipeline* pipeline, VkDevice device, const TPipelineIndices* indices)
{
    const VkPipeline libraries[] = {
        pipeline->library->vertexInputInterfaces[indices->vertexInputInterfaceIndex],
        pipeline->library->preRasterizationShaders[indices->preRasterizationShaderIndex],
        pipeline->library->fragmentOutputInterfaces[indices->fragmentOutputInterfaceIndex],
        pipeline->library->fragmentShaders[indices->fragmentShaderIndex]
    };

    const VkPipelineLibraryCreateInfoKHR linkingInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LIBRARY_CREATE_INFO_KHR,
        .libraryCount = FU_N_ELEMENTS(libraries),
        .pLibraries = libraries
    };

    const VkGraphicsPipelineCreateInfo pipelineInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &linkingInfo,
        .layout = pipeline->layout,
        .renderPass = pipeline->renderPass
    };

    if (FU_UNLIKELY(vkCreateGraphicsPipelines(device, NULL, 1, &pipelineInfo, &defCustomAllocator, &pipeline->pipeline))) {
        fu_error("Failed to create graphics pipeline @pipeline\n");
        return false;
    }
    return true;
}

void t_pipeline_destroy(TPipeline* pipeline, VkDevice device)
{
    vkDestroyRenderPass(device, pipeline->renderPass, &defCustomAllocator);
    vkDestroyPipelineLayout(device, pipeline->layout, &defCustomAllocator);
    vkDestroyPipeline(device, pipeline->pipeline, &defCustomAllocator);
}

void t_pipeline_destroy_all(TPipeline* pipeline, VkDevice device)
{
    t_pipeline_destroy(pipeline, device);
    for (uint32_t i = 0; i < pipeline->library->vertexInputInterfaceCount; i++)
        vkDestroyPipeline(device, pipeline->library->vertexInputInterfaces[i], &defCustomAllocator);
    for (uint32_t i = 0; i < pipeline->library->preRasterizationShaderCount; i++)
        vkDestroyPipeline(device, pipeline->library->preRasterizationShaders[i], &defCustomAllocator);
    for (uint32_t i = 0; i < pipeline->library->fragmentOutputInterfaceCount; i++)
        vkDestroyPipeline(device, pipeline->library->fragmentOutputInterfaces[i], &defCustomAllocator);
    for (uint32_t i = 0; i < pipeline->library->fragmentShaderCount; i++)
        vkDestroyPipeline(device, pipeline->library->fragmentShaders[i], &defCustomAllocator);

    fu_free(pipeline->library->vertexInputInterfaces);
    fu_free(pipeline->library->preRasterizationShaders);
    fu_free(pipeline->library->fragmentOutputInterfaces);
    fu_free(pipeline->library->fragmentShaders);
}
#endif