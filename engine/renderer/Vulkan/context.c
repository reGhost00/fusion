#ifdef FU_RENDERER_TYPE_VK
#include <stdio.h>

#include "core/logger.h"
#include "core/memory.h"

#include "platform/glfw.inner.h"
#include "platform/sdl.inner.h"

#include "../context.h"

#include "misc.inner.h"

#define T_DEVICE(ctx) (ctx->surface->device.device)

#define T_FRAME_BUFF(ctx) (ctx->frame.buffers)
#define T_FRAME_CNT(ctx) (ctx->surface->swapchain.imageCount)

#define T_CMD_POOL(ctx) (ctx->frame.commandPool)
#define T_CMD_BUFF(ctx) (ctx->buffer.commands)

#define T_DESC_LAYOUT(ctx) (ctx->descriptor.layout)
#define T_DESC_POOL(ctx) (ctx->descriptor.pool)
#define T_DESC_SETS(ctx) (ctx->descriptor.sets)

#define T_PIPELINE_LAYOUT(ctx) (ctx->pipeline.layout)
#define T_RENDER_PASS(ctx) (ctx->pipeline.renderPass)
#define T_CURR_FRAME(ctx) (ctx->frame.currFrame)

#define T_SYNC_IDEL_SEMA(ctx) (ctx->sync.available)
#define T_SYNC_DONE_SEMA(ctx) (ctx->sync.finished)
#define T_SYNC_DONE_FENC(ctx) (ctx->sync.inFlight)

#define aligned_size(value, alignment) ((value + alignment - 1) & ~(alignment - 1))

FU_DEFINE_TYPE(FUContext, fu_context, FU_TYPE_OBJECT)

static void fu_pipeline_init_args_free(TPipelineInitArgs* args)
{
    if (!args)
        return;
    fu_shader_free(args->shaders->vertex);
    fu_shader_free(args->shaders->fragment);
    fu_free(args->shaders);
    fu_free(args);
}

static TPipelineInitArgs* fu_pipeline_init_args_new(FUShaderGroup* shaders, const FUContextArgs* args)
{
    TPipelineInitArgs* rev = fu_malloc0(sizeof(TPipelineInitArgs));
    rev->shaders = fu_malloc(sizeof(FUShaderGroup));
    rev->shaders->vertex = fu_steal_pointer(&shaders->vertex);
    rev->shaders->fragment = fu_steal_pointer(&shaders->fragment);
    if (args)
        memcpy(&rev->ctx, args, sizeof(FUContextArgs));

    return rev;
}

//
//  pipeline
//  pipeline stage 1
static bool fu_context_init_frameBuffers(FUContext* ctx, VkCommandBuffer* commandBuffers)
{
    VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = T_CMD_POOL(ctx),
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = ctx->surface->swapchain.imageCount
    };
    return !vkAllocateCommandBuffers(T_DEVICE(ctx), &allocInfo, commandBuffers);
}

/** initialize frameBuffers & renderPass */
static bool fu_context_init_frames(FUContext* ctx)
{
    fu_print_func_name();
    fu_return_val_if_fail(ctx && ctx->initArgs, false);
    // fu_surface_t* sf = ctx->surface;
    VkExtent3D extent = { .depth = 1 };
    memcpy(&extent, &ctx->surface->surface.extent, sizeof(VkExtent2D));
    //
    //  render pass & framebuffers attachments

    VkImageView framebufferAttachments[3];
    VkAttachmentDescription renderPassAttachments[3];
    VkClearValue clearColors[3];
    VkClearValue colorClearValue = { { { 0.0f, 0.0f, 0.0f, 1.0f } } };
    VkClearValue depthClearValue = { .depthStencil = { 1.0f } };
    VkFormat surfaceFormat = ctx->surface->surface.format.format;
    TAttachmentArgs* attr = NULL;
    int frameAttachmentIndex = 0, colorAttachmentIndex = -1, depthAttachmentIndex = -1;

    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1
    };

    if (ctx->initArgs->ctx.multiSample) {
        attr = t_attachment_args_prepare(attr, surfaceFormat, VK_SAMPLE_COUNT_4_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        colorAttachmentIndex = attr->attachmentRef.attachment;
        renderPassAttachments[colorAttachmentIndex] = attr->attachment;
        subpass.pColorAttachments = &attr->attachmentRef;
        if (ctx->initArgs->ctx.depthTest) {
            ctx->buffer.color = vk_image_buffer_new(&extent, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, surfaceFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1, VK_SAMPLE_COUNT_4_BIT);
            ctx->buffer.depth = vk_image_buffer_new(&extent, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_IMAGE_ASPECT_DEPTH_BIT, 1, VK_SAMPLE_COUNT_4_BIT);
            attr = t_attachment_args_prepare(attr, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_SAMPLE_COUNT_4_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
            depthAttachmentIndex = attr->attachmentRef.attachment;
            framebufferAttachments[depthAttachmentIndex] = ctx->buffer.depth->image.view;
            renderPassAttachments[depthAttachmentIndex] = attr->attachment;
            clearColors[depthAttachmentIndex] = depthClearValue;
            subpass.pDepthStencilAttachment = &attr->attachmentRef;
        } else
            ctx->buffer.color = vk_image_buffer_new(&extent, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, surfaceFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1, VK_SAMPLE_COUNT_1_BIT);
        attr = t_attachment_args_prepare(attr, surfaceFormat, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        frameAttachmentIndex = attr->attachmentRef.attachment;
        framebufferAttachments[colorAttachmentIndex] = ctx->buffer.color->image.view;
        renderPassAttachments[frameAttachmentIndex] = attr->attachment;
        clearColors[colorAttachmentIndex] = colorClearValue;
        clearColors[frameAttachmentIndex] = colorClearValue;
        subpass.pResolveAttachments = &attr->attachmentRef;
    } else if (ctx->initArgs->ctx.depthTest) {
        attr = t_attachment_args_prepare(attr, surfaceFormat, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        renderPassAttachments[frameAttachmentIndex] = attr->attachment;
        subpass.pColorAttachments = &attr->attachmentRef;
        clearColors[frameAttachmentIndex] = colorClearValue;

        attr = t_attachment_args_prepare(attr, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_SAMPLE_COUNT_4_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
        ctx->buffer.depth = vk_image_buffer_new(&extent, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_IMAGE_ASPECT_DEPTH_BIT, 1, VK_SAMPLE_COUNT_1_BIT);
        depthAttachmentIndex = attr->attachmentRef.attachment;
        framebufferAttachments[depthAttachmentIndex] = ctx->buffer.depth->image.view;
        renderPassAttachments[depthAttachmentIndex] = attr->attachment;
        subpass.pDepthStencilAttachment = &attr->attachmentRef;
        clearColors[depthAttachmentIndex] = depthClearValue;
    } else {
        attr = t_attachment_args_prepare(attr, surfaceFormat, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        renderPassAttachments[frameAttachmentIndex] = attr->attachment;
        subpass.pColorAttachments = &attr->attachmentRef;
        clearColors[frameAttachmentIndex] = colorClearValue;
    }

    //
    //  render pass

    VkSubpassDependency dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
    };

    if (ctx->initArgs->ctx.depthTest) {
        dependency.srcStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }

    VkFramebufferCreateInfo framebufferInfo = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .attachmentCount = 1 + attr->attachmentRef.attachment,
        .pAttachments = framebufferAttachments,
        .width = extent.width,
        .height = extent.height,
        .layers = 1
    };

    if (FU_UNLIKELY(!t_pipeline_init_render_pass(&ctx->pipeline, T_DEVICE(ctx), framebufferInfo.attachmentCount, renderPassAttachments, 1, &subpass, 1, &dependency))) {
        fu_error("Failed to create render pass\n");
        t_attachment_args_free_all(attr);
        return false;
    }
    //
    // framebuffer
    VkFramebuffer* framebuffers = fu_malloc(sizeof(VkFramebuffer) * T_FRAME_CNT(ctx));
    framebufferInfo.renderPass = ctx->pipeline.renderPass;
    for (uint32_t i = 0; i < T_FRAME_CNT(ctx); i++) {
        framebufferAttachments[frameAttachmentIndex] = ctx->surface->swapchain.images[i].view;
        if (FU_UNLIKELY(vkCreateFramebuffer(T_DEVICE(ctx), &framebufferInfo, &defCustomAllocator, framebuffers + i))) {
            fu_error("Failed to create framebuffer[%u]\n", i);
            t_attachment_args_free_all(attr);
            fu_free(framebuffers);
            return false;
        }
    }

    ctx->frame.attachmentCount = framebufferInfo.attachmentCount;
    ctx->frame.clearValues = fu_memdup(clearColors, sizeof(VkClearValue) * ctx->frame.attachmentCount);
    T_FRAME_BUFF(ctx) = fu_steal_pointer(&framebuffers);
    t_attachment_args_free_all(attr);
    return true;
}
/** initialize command pool & buffers */
static bool fu_context_init_buffers(FUContext* ctx)
{
    fu_print_func_name();
    fu_return_val_if_fail(ctx && ctx->initArgs, false);

    VkCommandPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = ctx->surface->device.queueFamilyIndex,
    };

    if (FU_UNLIKELY(vkCreateCommandPool(T_DEVICE(ctx), &poolInfo, &defCustomAllocator, &T_CMD_POOL(ctx)))) {
        fu_error("Failed to create command pool\n");
        return false;
    }

    VkCommandBuffer* commandBuffers = fu_malloc(sizeof(VkCommandBuffer) * T_FRAME_CNT(ctx));
    if (FU_LIKELY(fu_context_init_frameBuffers(ctx, commandBuffers))) {
        T_CMD_BUFF(ctx) = fu_steal_pointer(&commandBuffers);
        return true;
    }
    fu_error("Failed to allocate command buffers\n");
    fu_free(commandBuffers);
    return false;
}

static bool fu_context_init_input_stage(FUContext* context)
{
    fu_print_func_name();
    fu_return_val_if_fail(context && context->initArgs, false);
    fu_shader_t* sd = (fu_shader_t*)context->initArgs->shaders->vertex;
    const VkVertexInputBindingDescription binding = {
        .stride = sd->stride,
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    context->pipelineIndices.vertexInputInterfaceIndex = t_pipeline_library_new_vertex_input_interface(&context->surface->library, context->surface->device.device, 1, &binding, sd->attributeCount, sd->attributes);
    return 0 <= context->pipelineIndices.vertexInputInterfaceIndex;
}

//  stage 1 end
//  stage 2

static bool fu_context_init_descriptor_set_layout(FUContext* ctx)
{
    fu_return_val_if_fail(ctx->initArgs, false);
    if (!ctx->initArgs->desc)
        return true;
    TDescriptorArgs2* desc = ctx->initArgs->desc;
    uint32_t bindingCount = 1 + desc->binding.binding;
    VkDescriptorSetLayoutBinding* bindings = fu_malloc(sizeof(VkDescriptorSetLayoutBinding) * bindingCount);
    do {
        bindings[desc->binding.binding] = desc->binding;
        desc = desc->next;
    } while (desc);
    VkDescriptorSetLayoutCreateInfo layoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = bindingCount,
        .pBindings = bindings
    };
    bool rev = !vkCreateDescriptorSetLayout(T_DEVICE(ctx), &layoutInfo, &defCustomAllocator, &T_DESC_LAYOUT(ctx));
    fu_free(bindings);
    return rev;
}

static bool fu_context_init_descriptor_pool(FUContext* ctx)
{
    fu_return_val_if_fail(ctx->initArgs, false);
    if (!ctx->initArgs->desc)
        return true;
    TDescriptorArgs2* desc = ctx->initArgs->desc;
    uint32_t sizeCount = 1 + desc->binding.binding;
    VkDescriptorPoolSize* sizes = fu_malloc(sizeof(VkDescriptorPoolSize) * sizeCount);
    do {
        sizes[desc->binding.binding] = desc->size;
        desc = desc->next;
    } while (desc);
    VkDescriptorPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .poolSizeCount = sizeCount,
        .pPoolSizes = sizes,
        .maxSets = ctx->surface->swapchain.imageCount
    };
    bool rev = !vkCreateDescriptorPool(T_DEVICE(ctx), &poolInfo, &defCustomAllocator, &T_DESC_POOL(ctx));
    fu_free(sizes);
    return true;
}
/**
 * @brief create and update descriptor sets
 * assume descriptor is uniform buffer and combined sampler
 * @param ctx
 * @return initialize
 */
static bool fu_context_init_descriptor_sets(FUContext* ctx)
{
    fu_return_val_if_fail(ctx->initArgs, false);
    if (!ctx->initArgs->desc)
        return true;
    VkDescriptorSetLayout* layouts = fu_malloc(sizeof(VkDescriptorSetLayout) * T_FRAME_CNT(ctx));
    for (uint32_t i = 0; i < T_FRAME_CNT(ctx); i++)
        layouts[i] = T_DESC_LAYOUT(ctx);
    VkDescriptorSetAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = T_DESC_POOL(ctx),
        .descriptorSetCount = T_FRAME_CNT(ctx),
        .pSetLayouts = layouts
    };

    T_DESC_SETS(ctx) = fu_malloc0(sizeof(VkDescriptorSet) * T_FRAME_CNT(ctx));
    if (FU_UNLIKELY(vkAllocateDescriptorSets(T_DEVICE(ctx), &allocInfo, T_DESC_SETS(ctx)))) {
        fu_error("Failed to allocate descriptor sets\n");
        fu_free(layouts);
        return false;
    }

    TDescriptorArgs2* desc = ctx->initArgs->desc;
    VkWriteDescriptorSet* writes = fu_malloc0(sizeof(VkWriteDescriptorSet) * ctx->descriptor.count);
    VkDescriptorBufferInfo** buffers = fu_malloc0(sizeof(VkDescriptorBufferInfo*) * ctx->descriptor.count);
    ctx->descriptor.offsets = fu_malloc(sizeof(VkDeviceSize) * ctx->descriptor.count);
    ctx->descriptor.ranges = fu_malloc(sizeof(VkDeviceSize) * ctx->descriptor.count);
    ctx->descriptor.size = desc->bufferInfo.offset + desc->bufferInfo.range;
    if (FU_UNLIKELY(!vk_uniform_buffer_init(ctx->descriptor.size * T_FRAME_CNT(ctx), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT, &ctx->buffer.uniforms))) {
        fu_error("Failed to init desc buffer\n");
        fu_free(layouts);
        fu_free(writes);
        fu_free(buffers);
        return false;
    }

    while (desc && VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER != desc->write.descriptorType) {
        desc->imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        desc->imageInfo.imageView = ctx->buffer.images[desc->binding.binding - ctx->buffer.uniformCount].image.view;
        desc->imageInfo.sampler = ctx->buffer.sampler;

        writes[desc->binding.binding] = desc->write;
        writes[desc->binding.binding].pImageInfo = &desc->imageInfo;

        buffers[desc->binding.binding] = &desc->bufferInfo;
        desc = desc->next;
    }

    while (desc && VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER != desc->write.descriptorType) {
        ctx->descriptor.offsets[desc->binding.binding] = desc->bufferInfo.offset;
        ctx->descriptor.ranges[desc->binding.binding] = desc->bufferInfo.range;
        writes[desc->binding.binding] = desc->write;

        buffers[desc->binding.binding] = &desc->bufferInfo;
        buffers[desc->binding.binding]->buffer = ctx->buffer.uniforms.buffer;

        desc = desc->next;
    }
    for (uint32_t i = 0; i < T_FRAME_CNT(ctx); i++) {
        for (uint32_t j = 0; j < ctx->descriptor.count; j++) {
            writes[j].dstSet = T_DESC_SETS(ctx)[i];
            buffers[j]->offset = ctx->descriptor.size * i + ctx->descriptor.offsets[j];
        }
        vkUpdateDescriptorSets(T_DEVICE(ctx), ctx->descriptor.count, writes, 0, NULL);
    }

    fu_free(layouts);
    fu_free(writes);
    fu_free(buffers);
    return true;
}

static void fu_context_destroy_descriptor(FUContext* ctx)
{
    if (!ctx->descriptor.count)
        return;
    vkDestroyDescriptorPool(T_DEVICE(ctx), T_DESC_POOL(ctx), &defCustomAllocator);
    vkDestroyDescriptorSetLayout(T_DEVICE(ctx), T_DESC_LAYOUT(ctx), &defCustomAllocator);
    fu_free(T_DESC_SETS(ctx));
    fu_free(ctx->descriptor.offsets);
    fu_free(ctx->descriptor.ranges);
}

static bool fu_context_init_descriptor(FUContext* ctx)
{
    if (!ctx->initArgs->desc)
        return true;
    bool rev = fu_context_init_descriptor_set_layout(ctx);
    rev &= fu_context_init_descriptor_pool(ctx);
    rev &= fu_context_init_descriptor_sets(ctx);
    return rev;
}

static bool fu_context_init_rasterization_stage(FUContext* ctx)
{
    fu_print_func_name();
    fu_return_val_if_fail(ctx && ctx->initArgs, false);
    fu_shader_t* sd = (fu_shader_t*)ctx->initArgs->shaders->vertex;

    // stage
    VkShaderModuleCreateInfo shaderInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = sd->codeSize,
        .pCode = sd->code
    };
    VkPipelineShaderStageCreateInfo shaderStageInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = &shaderInfo,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .pName = "main"
    };

    ctx->pipelineIndices.preRasterizationShaderIndex = t_pipeline_library_new_layout_and_pre_rasterization_shaders(&ctx->surface->library, &ctx->pipeline, T_DEVICE(ctx), 1, &T_DESC_LAYOUT(ctx), 1, &shaderStageInfo);
    return 0 <= ctx->pipelineIndices.preRasterizationShaderIndex;
}

static bool fu_context_init_fragment_stage(FUContext* ctx)
{
    fu_print_func_name();
    fu_return_val_if_fail(ctx && ctx->initArgs, false);
    fu_shader_t* sd = (fu_shader_t*)ctx->initArgs->shaders->fragment;

    // stage
    const VkShaderModuleCreateInfo shaderInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = sd->codeSize,
        .pCode = sd->code
    };

    const VkPipelineShaderStageCreateInfo shaderStageInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = &shaderInfo,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pName = "main"
    };

    ctx->pipelineIndices.fragmentShaderIndex = t_pipeline_library_new_fragment_shader(&ctx->surface->library, T_DEVICE(ctx), T_PIPELINE_LAYOUT(ctx), T_RENDER_PASS(ctx), 1, &shaderStageInfo, ctx->initArgs->ctx.depthTest);
    return 0 <= ctx->pipelineIndices.fragmentShaderIndex;
}

static bool fu_context_init_pipeline(FUContext* ctx)
{
    fu_print_func_name();
    fu_return_val_if_fail(ctx && ctx->initArgs, false);
    //
    // color blend state
    const VkPipelineColorBlendAttachmentState colorBlendAttachment = {
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };

    if (ctx->initArgs->ctx.multiSample)
        ctx->pipelineIndices.fragmentOutputInterfaceIndex = t_pipeline_library_new_fragment_output_interface(&ctx->surface->library, T_DEVICE(ctx), T_PIPELINE_LAYOUT(ctx), T_RENDER_PASS(ctx), VK_SAMPLE_COUNT_4_BIT, 1, &colorBlendAttachment);
    else
        ctx->pipelineIndices.fragmentOutputInterfaceIndex = t_pipeline_library_new_fragment_output_interface(&ctx->surface->library, T_DEVICE(ctx), T_PIPELINE_LAYOUT(ctx), T_RENDER_PASS(ctx), VK_SAMPLE_COUNT_1_BIT, 1, &colorBlendAttachment);
    if (FU_UNLIKELY(0 > ctx->pipelineIndices.fragmentOutputInterfaceIndex)) {
        fu_error("Failed to create fragment output interface\n");
        return false;
    }

    return t_pipeline_init(&ctx->surface->library, &ctx->pipelineIndices, T_DEVICE(ctx), &ctx->pipeline);
}

static bool fu_context_init_synchronization_objects(FUContext* ctx)
{
    fu_print_func_name();
    fu_return_val_if_fail(ctx && ctx->initArgs, false);
    VkSemaphoreCreateInfo semaphoreInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };
    VkFenceCreateInfo fenceInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    VkSemaphore* imageAvailable = fu_malloc(sizeof(VkSemaphore) * T_FRAME_CNT(ctx));
    VkSemaphore* renderFinished = fu_malloc(sizeof(VkSemaphore) * T_FRAME_CNT(ctx));
    VkFence* fences = fu_malloc(sizeof(VkFence) * T_FRAME_CNT(ctx));

    for (uint32_t i = 0; i < T_FRAME_CNT(ctx); i++) {
        if (FU_UNLIKELY(vkCreateSemaphore(T_DEVICE(ctx), &semaphoreInfo, &defCustomAllocator, &imageAvailable[i]))) {
            fu_error("Failed to create image available semaphore[%u]\n", i);
            goto out;
        }
        if (FU_UNLIKELY(vkCreateSemaphore(T_DEVICE(ctx), &semaphoreInfo, &defCustomAllocator, &renderFinished[i]))) {
            fu_error("Failed to create render finished semaphore[%u]\n", i);
            goto out;
        }
        if (FU_UNLIKELY(vkCreateFence(T_DEVICE(ctx), &fenceInfo, &defCustomAllocator, &fences[i]))) {
            fu_error("Failed to create fence[%u]\n", i);
            goto out;
        }
    }

    T_SYNC_IDEL_SEMA(ctx) = fu_steal_pointer(&imageAvailable);
    T_SYNC_DONE_SEMA(ctx) = fu_steal_pointer(&renderFinished);
    T_SYNC_DONE_FENC(ctx) = fu_steal_pointer(&fences);
    return true;
out:
    fu_free(imageAvailable);
    fu_free(renderFinished);
    fu_free(fences);
    return false;
}

static bool fu_context_record_command(FUContext* ctx, VkCommandBuffer cmdf, uint32_t imageIndex)
{
    TSurface* sf = &ctx->surface->surface;
    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };

    if (vkBeginCommandBuffer(cmdf, &beginInfo)) {
        fu_error("Failed to begin recording command buffer\n");
        return false;
    }

    VkRenderPassBeginInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = ctx->pipeline.renderPass,
        .framebuffer = T_FRAME_BUFF(ctx)[imageIndex],
        .renderArea = {
            .extent = sf->extent },
        .clearValueCount = ctx->frame.attachmentCount,
        .pClearValues = ctx->frame.clearValues
    };

    vkCmdBeginRenderPass(cmdf, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmdf, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->pipeline.pipeline);

    VkViewport viewport = {
        .width = (float)sf->extent.width,
        .height = (float)sf->extent.height,
        .maxDepth = 1.0f
    };
    VkRect2D scissor = {
        .extent = sf->extent
    };
    VkBuffer vertexBuffer[] = { ctx->buffer.vertices.buffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdSetViewport(cmdf, 0, 1, &viewport);
    vkCmdSetScissor(cmdf, 0, 1, &scissor);

    vkCmdBindVertexBuffers(cmdf, 0, 1, &ctx->buffer.vertices.buffer, offsets);
    vkCmdBindIndexBuffer(cmdf, ctx->buffer.vertices.buffer, ctx->buffer.vertexSize, VK_INDEX_TYPE_UINT32);
    if (ctx->descriptor.count) {
        vkCmdBindDescriptorSets(cmdf, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->pipeline.layout, 0, 1, &T_DESC_SETS(ctx)[imageIndex], 0, NULL);
    }
    //
    vkCmdDrawIndexed(cmdf, ctx->buffer.indexCount, 1, 0, 0, 0);

    vkCmdEndRenderPass(cmdf);
    if (vkEndCommandBuffer(cmdf)) {
        fu_error("Failed to record command buffer\n");
        return false;
    }
    return true;
}

//
//  context

static void fu_context_destroy_buffer(FUContext* ctx)
{
    if (ctx->buffer.uniformCount)
        vk_uniform_buffer_destroy(&ctx->buffer.uniforms);
    if (ctx->buffer.imageCount) {
        for (uint32_t i = 0; i < ctx->buffer.imageCount; i++)
            vk_image_buffer_destroy(&ctx->buffer.images[i]);
        vkDestroySampler(T_DEVICE(ctx), ctx->buffer.sampler, &defCustomAllocator);
        fu_free(ctx->buffer.images);
    }
    vk_uniform_buffer_destroy(&ctx->buffer.vertices);
    fu_free(T_CMD_BUFF(ctx));
}

static void fu_context_destroy_frames(FUContext* ctx)
{
    for (uint32_t i = 0; i < T_FRAME_CNT(ctx); i++)
        vkDestroyFramebuffer(T_DEVICE(ctx), T_FRAME_BUFF(ctx)[i], &defCustomAllocator);
    vkDestroyCommandPool(T_DEVICE(ctx), T_CMD_POOL(ctx), &defCustomAllocator);
    fu_free(T_FRAME_BUFF(ctx));
}

static void fu_context_destroy_synchronization_objects(FUContext* ctx)
{
    for (uint32_t i = 0; i < T_FRAME_CNT(ctx); i++) {
        vkDestroySemaphore(T_DEVICE(ctx), T_SYNC_DONE_SEMA(ctx)[i], &defCustomAllocator);
        vkDestroySemaphore(T_DEVICE(ctx), T_SYNC_IDEL_SEMA(ctx)[i], &defCustomAllocator);
        vkDestroyFence(T_DEVICE(ctx), T_SYNC_DONE_FENC(ctx)[i], &defCustomAllocator);
    }
    fu_free(T_SYNC_DONE_SEMA(ctx));
    fu_free(T_SYNC_IDEL_SEMA(ctx));
    fu_free(T_SYNC_DONE_FENC(ctx));
}

static void fu_context_dispose(FUObject* obj)
{
    //  例外：图形管线由用户分步创建
    fu_print_func_name();
    FUContext* ctx = (FUContext*)obj;
    VkDevice dev = ctx->surface->device.device;
    vkDeviceWaitIdle(dev);
    fu_pipeline_init_args_free(ctx->initArgs);
    t_pipeline_destroy(&ctx->pipeline, T_DEVICE(ctx));
    fu_context_destroy_frames(ctx);
    fu_context_destroy_buffer(ctx);
    fu_context_destroy_descriptor(ctx);
    fu_context_destroy_synchronization_objects(ctx);
}

static void fu_context_class_init(FUObjectClass* oc)
{
    oc->dispose = fu_context_dispose;
}
/**
 * @brief Create new graphics pipeline (path of pipeline)
 *  this will take shader pointer if success
 * @param win
 * @param args
 * @return FUContext*
 */
FUContext* fu_context_new_take(const FUWindow* win, FUShaderGroup* shaders, const FUContextArgs* args)
{
    FUContext* ctx = (FUContext*)fu_object_new(FU_TYPE_CONTEXT);
    ctx->initArgs = fu_pipeline_init_args_new(shaders, args);
    ctx->surface = (fu_surface_t*)&win->surface;
    if (FU_UNLIKELY(!fu_context_init_frames(ctx))) {
        fu_error("Failed to create render pass\n");
        goto out;
    }

    if (FU_UNLIKELY(!fu_context_init_buffers(ctx))) {
        fu_error("Failed to create buffers\n");
        goto out;
    }

    if (FU_UNLIKELY(!fu_context_init_input_stage(ctx))) {
        fu_error("Failed to init pipeline: input stage\n");
        goto out;
    }

    return ctx;
out:
    fu_object_unref(ctx);
    return NULL;
}

/**
 * @brief Set uniform descriptor of pipeline
 *  uniform buffer will be created pre frame
 * @param ctx
 * @param size
 * @return index of the descriptor (times frame count)
 */
int fu_context_add_uniform_descriptor(FUContext* ctx, const uint32_t size, VkShaderStageFlags stage)
{
    fu_return_val_if_fail(ctx->initArgs, -1);

    uint32_t alignedSize = aligned_size(size, ctx->surface->device.physicalDevice.properties.limits.minUniformBufferOffsetAlignment);
    int rev = ctx->buffer.uniformCount++;
    ctx->buffer.uniformSize += alignedSize;
    ctx->descriptor.count += 1;
    ctx->initArgs->desc = t_descriptor_args_prepare(ctx->initArgs->desc, alignedSize, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, stage);

    return rev;
}

int fu_context_add_sampler_descriptor(FUContext* ctx, const uint32_t width, const uint32_t height, const uint8_t* data)
{
    fu_return_val_if_fail(ctx->initArgs, -1);
    fu_return_val_if_fail(width && height && data, -1);
    VkExtent3D extent = { .width = width, .height = height, .depth = 1 };
    VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
    uint32_t levels = 1;
    int rev = ctx->buffer.imageCount++;
    ctx->descriptor.count += 1;
    ctx->initArgs->desc = t_descriptor_args_prepare(ctx->initArgs->desc, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    ctx->buffer.images = fu_realloc(ctx->buffer.images, sizeof(TImageBuffer) * ctx->buffer.imageCount);
    if (ctx->initArgs->ctx.multiSample)
        samples = VK_SAMPLE_COUNT_4_BIT;
    if (ctx->initArgs->ctx.mipmap)
        usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    if (FU_LIKELY(vk_image_buffer_init(&extent, usage, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, ctx->surface->surface.format.format, VK_IMAGE_ASPECT_COLOR_BIT, levels, samples, &ctx->buffer.images[rev]))) {
        VkDeviceSize size = width * height * 4;
        TUniformBuffer* staging = vk_uniform_buffer_new(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
        if (!staging)
            return -1;
        memcpy(staging->allocInfo.pMappedData, data, size);
        vk_image_buffer_transition_layout(&ctx->buffer.images[rev], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, T_CMD_POOL(ctx), ctx->surface->device.generalQueue, NULL);
        vk_uniform_buffer_copy_to_image(staging, &ctx->buffer.images[rev], T_CMD_POOL(ctx), ctx->surface->device.generalQueue, NULL);
        vk_image_buffer_transition_layout(&ctx->buffer.images[rev], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, T_CMD_POOL(ctx), ctx->surface->device.generalQueue, NULL);
        vk_uniform_buffer_free(staging);
        if (!rev) {
            if (FU_UNLIKELY(!vk_sampler_init(VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, 1, &ctx->buffer.sampler))) {
                fu_error("Failed to create sampler\n");
                return -1;
            }
        }
    }
    return rev;
}

bool fu_context_update_input_data2(FUContext* ctx, uint64_t vertexSize, const float* vertices, uint64_t indexSize, const uint32_t* indices)
{
    fu_return_val_if_fail(vertexSize && vertices && indexSize && indices, false);

    ctx->buffer.vertexSize = aligned_size(vertexSize, ctx->surface->device.physicalDevice.properties.limits.minUniformBufferOffsetAlignment);
    ctx->buffer.indexSize = aligned_size(indexSize, ctx->surface->device.physicalDevice.properties.limits.minUniformBufferOffsetAlignment);
    VkDeviceSize size = ctx->buffer.vertexSize + ctx->buffer.indexSize;
    if (ctx->buffer.vertices.allocInfo.size < size) {
        if (ctx->buffer.vertices.allocInfo.size)
            vk_uniform_buffer_destroy(&ctx->buffer.vertices);
        if (FU_UNLIKELY(!vk_uniform_buffer_init(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, &ctx->buffer.vertices))) {
            fu_error("Failed to init vertex buffer");
            return false;
        }
    }

    TUniformBuffer* staging = vk_uniform_buffer_new(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);

    memcpy(staging->allocInfo.pMappedData, vertices, vertexSize);
    memcpy(staging->allocInfo.pMappedData + ctx->buffer.vertexSize, indices, indexSize);

    vk_uniform_buffer_copy(staging, ctx->buffer.vertices.buffer, T_CMD_POOL(ctx), ctx->surface->device.generalQueue, NULL);
    ctx->buffer.indexCount = indexSize / sizeof(uint32_t);

    vk_uniform_buffer_free(staging);
    return true;
}

bool fu_context_present(FUContext* ctx)
{
    static uint32_t imageIndex;
    TSwapchain* sc = &ctx->surface->swapchain;
    vkWaitForFences(T_DEVICE(ctx), 1, &T_SYNC_DONE_FENC(ctx)[T_CURR_FRAME(ctx)], VK_TRUE, ~0U);
    VkResult rev = vkAcquireNextImageKHR(T_DEVICE(ctx), sc->swapchain, ~0U, T_SYNC_IDEL_SEMA(ctx)[T_CURR_FRAME(ctx)], VK_NULL_HANDLE, &imageIndex);
    if (rev) {
        if (VK_ERROR_OUT_OF_DATE_KHR == rev) {
            sc->outOfDate = true;
            return false;
        }
        if (VK_SUBOPTIMAL_KHR != rev) {
            fu_error("Failed to acquire swapchain image\n");
            return false;
        }
        fu_warning("swapchain suboptiomal\n");
    }

    VkCommandBuffer cmdf = T_CMD_BUFF(ctx)[T_CURR_FRAME(ctx)];

    vkResetFences(T_DEVICE(ctx), 1, &T_SYNC_DONE_FENC(ctx)[T_CURR_FRAME(ctx)]);
    fu_context_record_command(ctx, cmdf, imageIndex);

    VkSemaphore waitSemaphores[] = { T_SYNC_IDEL_SEMA(ctx)[T_CURR_FRAME(ctx)] };
    VkSemaphore signalSemaphores[] = { T_SYNC_DONE_SEMA(ctx)[T_CURR_FRAME(ctx)] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = waitSemaphores,
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmdf,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = signalSemaphores
    };
    if (vkQueueSubmit(ctx->surface->device.generalQueue, 1, &submitInfo, T_SYNC_DONE_FENC(ctx)[T_CURR_FRAME(ctx)])) {
        fu_error("Failed to submit command buffer\n");
        return false;
    }
    VkSwapchainKHR swapchains[] = { sc->swapchain };
    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signalSemaphores,
        .swapchainCount = 1,
        .pSwapchains = swapchains,
        .pImageIndices = &imageIndex
    };
    rev = vkQueuePresentKHR(ctx->surface->device.generalQueue, &presentInfo);
    if (rev) {
        if (VK_ERROR_OUT_OF_DATE_KHR == rev || VK_SUBOPTIMAL_KHR == rev)
            sc->outOfDate = true;
        else {
            fu_error("Failed to present command buffer\n");
        }
    }
    T_CURR_FRAME(ctx) = (T_CURR_FRAME(ctx) + 1) % sc->imageCount;
    if (!T_CURR_FRAME(ctx)) {
        vkDeviceWaitIdle(T_DEVICE(ctx));
        vkResetCommandPool(T_DEVICE(ctx), T_CMD_POOL(ctx), 0);
        return fu_context_init_frameBuffers(ctx, T_CMD_BUFF(ctx));
    }
    return true;
}
/**
 * @brief pipeline creation stage 2
 *
 * @param win
 * @param ctx
 * @return true
 * @return false
 */
bool fu_window_add_context(FUWindow* win, FUContext* ctx)
{
    fu_return_val_if_fail(ctx->initArgs, false);
    ctx->surface = (fu_surface_t*)&win->surface;
    if (FU_UNLIKELY(!fu_context_init_descriptor(ctx)))
        return false;
    if (FU_UNLIKELY(!fu_context_init_rasterization_stage(ctx)))
        return false;
    if (FU_UNLIKELY(!fu_context_init_fragment_stage(ctx)))
        return false;
    if (FU_UNLIKELY(!fu_context_init_pipeline(ctx)))
        return false;
    if (FU_LIKELY(fu_context_init_synchronization_objects(ctx))) {
        fu_ptr_array_push(win->contexts, fu_object_ref(ctx));
        fu_pipeline_init_args_free(ctx->initArgs);
        ctx->initArgs = NULL;
        return true;
    }
    return false;
}

/**
 * @brief for window resize
 *
 * @param ctx
 * @param surface
 * @return true
 * @return false
 */
bool fu_context_init_framebuffers(FUContext* ctx, fu_surface_t* surface)
{
    VkImageView attachments[3];
    uint32_t frameAttachmentIndex = 0;
    VkFramebufferCreateInfo framebufferInfo = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = ctx->pipeline.renderPass,
        .attachmentCount = 1,
        .pAttachments = attachments,
        .width = surface->surface.extent.width,
        .height = surface->surface.extent.height,
        .layers = 1
    };

    if (ctx->buffer.color) {
        if (ctx->buffer.depth) {
            attachments[1] = ctx->buffer.depth->image.view;
            framebufferInfo.attachmentCount += 1;
            frameAttachmentIndex += 1;
        }
        attachments[0] = ctx->buffer.color->image.view;
        framebufferInfo.attachmentCount += 1;
        frameAttachmentIndex += 1;
    } else if (ctx->buffer.depth) {
        attachments[1] = ctx->buffer.depth->image.view;
        framebufferInfo.attachmentCount = 2;
    }

    for (uint32_t i = 0; i < surface->swapchain.imageCount; i++) {
        attachments[frameAttachmentIndex] = surface->swapchain.images[i].view;
        if (FU_UNLIKELY(vkCreateFramebuffer(surface->device.device, &framebufferInfo, &defCustomAllocator, T_FRAME_BUFF(ctx) + i))) {
            fu_error("Failed to create framebuffer[%u]\n", i);
            return false;
        }
    }

    return true;
}

void fu_context_update_uniform_buffer(FUContext* ctx, const uint32_t index, void* data, const VkDeviceSize size)
{
    fu_return_if_fail(ctx);
    fu_return_if_fail(index < ctx->buffer.uniformCount);
    TUniformBuffer* buff = &ctx->buffer.uniforms;
    VkDeviceSize offset = ctx->descriptor.offsets[index];
    VkDeviceSize range = fu_min(size, ctx->descriptor.ranges[index]);
    uintptr_t p = ((uintptr_t)buff->allocInfo.pMappedData) + offset;
    memcpy((void*)(p + ctx->descriptor.size * T_CURR_FRAME(ctx)), data, range);
}

#endif