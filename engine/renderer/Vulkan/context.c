#ifdef FU_RENDERER_TYPE_VK
#include <stdio.h>

#include "core/logger.h"
#include "core/memory.h"

#include "platform/glfw.inner.h"
#include "platform/sdl.inner.h"

#include "../context.h"

#include "misc.inner.h"

// typedef struct _TPipelineInitArgs {
//     FUContextArgs ctx;
//     FUShaderGroup* shaders;
//     TDescriptorArgs2* desc;
// } TPipelineInitArgs;

// struct _FUContext {
//     FUObject parent;
//     TPipelineInitArgs* initArgs;
//     fu_surface_t* surface;
//     TPipeline pipeline;
//     TPipelineIndices pipelineIndices;
//     TUniformBuffer vertices, indices;
//     /** user data */
//     TUniformBuffer* uniformBuffers;
//     /** multi sampling */
//     TImageBuffer* colorBuffer;
//     /** depth test */
//     TImageBuffer* depthBuffer;
//     /** user texture */
//     TImageBuffer* textureBuffers;
//     VkFramebuffer* framebuffers;
//     VkCommandPool pool;
//     VkCommandBuffer* commandBuffers;
//     VkSemaphore *imageAvailableSemaphores, *renderFinishedSemaphores;
//     VkFence* inFlightFences;
//     VkClearValue* clearValues;
//     uint32_t uniformBufferCount, imageBufferCount, attachmentCount, indexCount;
// };
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
static bool fu_context_init_frameBuffers(FUContext* ctx, VkCommandBuffer* commandBuffers)
{
    VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = ctx->pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = ctx->surface->swapchain.imageCount
    };
    return !vkAllocateCommandBuffers(ctx->surface->device.device, &allocInfo, commandBuffers);
}

/** initialize frameBuffers & renderPass */
static bool fu_context_init_frames(FUContext* ctx)
{
    fu_print_func_name();
    fu_return_val_if_fail(ctx && ctx->initArgs, false);
    fu_surface_t* sf = ctx->surface;
    VkDevice dev = sf->device.device;
    VkExtent3D extent = { .width = sf->surface.extent.width, .height = sf->surface.extent.height, .depth = 1 };

    //
    //  render pass & framebuffers attachments

    VkImageView framebufferAttachments[3];
    VkAttachmentDescription renderPassAttachments[3];
    VkClearValue clearColors[3];
    VkClearValue colorClearValue = { { { 0.0f, 0.0f, 0.0f, 1.0f } } };
    VkClearValue depthClearValue = { .depthStencil = { 1.0f } };
    TAttachmentArgs* attr = NULL;
    int frameAttachmentIndex = 0, colorAttachmentIndex = -1, depthAttachmentIndex = -1;

    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1
    };

    if (ctx->initArgs->ctx.multiSample) {
        attr = t_attachment_args_prepare(attr, sf->surface.format.format, VK_SAMPLE_COUNT_4_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        colorAttachmentIndex = attr->attachmentRef.attachment;
        renderPassAttachments[colorAttachmentIndex] = attr->attachment;
        subpass.pColorAttachments = &attr->attachmentRef;
        if (ctx->initArgs->ctx.depthTest) {
            ctx->colorBuffer = vk_image_buffer_new(&extent, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, sf->surface.format.format, VK_IMAGE_ASPECT_COLOR_BIT, 1, VK_SAMPLE_COUNT_4_BIT);
            ctx->depthBuffer = vk_image_buffer_new(&extent, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_IMAGE_ASPECT_DEPTH_BIT, 1, VK_SAMPLE_COUNT_4_BIT);
            attr = t_attachment_args_prepare(attr, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_SAMPLE_COUNT_4_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
            depthAttachmentIndex = attr->attachmentRef.attachment;
            framebufferAttachments[depthAttachmentIndex] = ctx->depthBuffer->image.view;
            renderPassAttachments[depthAttachmentIndex] = attr->attachment;
            clearColors[depthAttachmentIndex] = depthClearValue;
            subpass.pDepthStencilAttachment = &attr->attachmentRef;
        } else
            ctx->colorBuffer = vk_image_buffer_new(&extent, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, sf->surface.format.format, VK_IMAGE_ASPECT_COLOR_BIT, 1, VK_SAMPLE_COUNT_1_BIT);
        attr = t_attachment_args_prepare(attr, sf->surface.format.format, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        frameAttachmentIndex = attr->attachmentRef.attachment;
        framebufferAttachments[colorAttachmentIndex] = ctx->colorBuffer->image.view;
        renderPassAttachments[frameAttachmentIndex] = attr->attachment;
        clearColors[colorAttachmentIndex] = colorClearValue;
        clearColors[frameAttachmentIndex] = colorClearValue;
        subpass.pResolveAttachments = &attr->attachmentRef;
    } else if (ctx->initArgs->ctx.depthTest) {
        attr = t_attachment_args_prepare(attr, sf->surface.format.format, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        renderPassAttachments[frameAttachmentIndex] = attr->attachment;
        subpass.pColorAttachments = &attr->attachmentRef;
        clearColors[frameAttachmentIndex] = colorClearValue;

        attr = t_attachment_args_prepare(attr, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_SAMPLE_COUNT_4_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
        ctx->depthBuffer = vk_image_buffer_new(&extent, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_IMAGE_ASPECT_DEPTH_BIT, 1, VK_SAMPLE_COUNT_1_BIT);
        depthAttachmentIndex = attr->attachmentRef.attachment;
        framebufferAttachments[depthAttachmentIndex] = ctx->depthBuffer->image.view;
        renderPassAttachments[depthAttachmentIndex] = attr->attachment;
        subpass.pDepthStencilAttachment = &attr->attachmentRef;
        clearColors[depthAttachmentIndex] = depthClearValue;
    } else {
        attr = t_attachment_args_prepare(attr, sf->surface.format.format, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
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

    if (FU_UNLIKELY(!t_pipeline_init_render_pass(&ctx->pipeline, sf->device.device, framebufferInfo.attachmentCount, renderPassAttachments, 1, &subpass, 1, &dependency))) {
        fu_error("Failed to create render pass\n");
        t_attachment_args_free_all(attr);
        return false;
    }
    //
    // framebuffer
    VkFramebuffer* framebuffers = fu_malloc(sizeof(VkFramebuffer) * sf->swapchain.imageCount);
    framebufferInfo.renderPass = ctx->pipeline.renderPass;
    for (uint32_t i = 0; i < sf->swapchain.imageCount; i++) {
        framebufferAttachments[frameAttachmentIndex] = sf->swapchain.images[i].view;
        if (FU_UNLIKELY(vkCreateFramebuffer(dev, &framebufferInfo, &defCustomAllocator, framebuffers + i))) {
            fu_error("Failed to create framebuffer[%u]\n", i);
            t_attachment_args_free_all(attr);
            fu_free(framebuffers);
            return false;
        }
    }

    ctx->attachmentCount = framebufferInfo.attachmentCount;
    ctx->clearValues = fu_memdup(clearColors, sizeof(VkClearValue) * ctx->attachmentCount);
    ctx->framebuffers = fu_steal_pointer(&framebuffers);
    t_attachment_args_free_all(attr);
    return true;
}
/** initialize command pool & buffers */
static bool fu_context_init_buffers(FUContext* ctx)
{
    fu_print_func_name();
    fu_return_val_if_fail(ctx && ctx->initArgs, false);
    fu_surface_t* sf = ctx->surface;
    VkDevice dev = sf->device.device;

    VkCommandPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = sf->device.queueFamilyIndex,
    };

    if (FU_UNLIKELY(vkCreateCommandPool(dev, &poolInfo, &defCustomAllocator, &ctx->pool))) {
        fu_error("Failed to create command pool\n");
        return false;
    }

    VkCommandBuffer* commandBuffers = fu_malloc(sizeof(VkCommandBuffer) * sf->swapchain.imageCount);
    if (FU_LIKELY(fu_context_init_frameBuffers(ctx, commandBuffers))) {
        ctx->commandBuffers = fu_steal_pointer(&commandBuffers);
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

    ctx->pipelineIndices.preRasterizationShaderIndex = t_pipeline_library_new_layout_and_pre_rasterization_shaders(&ctx->surface->library, &ctx->pipeline, ctx->surface->device.device, 0, NULL, 1, &shaderStageInfo);
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

    VkDevice device = ctx->surface->device.device;
    VkPipelineLayout layout = ctx->pipeline.layout;
    VkRenderPass renderPass = ctx->pipeline.renderPass;
    ctx->pipelineIndices.fragmentShaderIndex = t_pipeline_library_new_fragment_shader(&ctx->surface->library, device, layout, renderPass, 1, &shaderStageInfo, ctx->initArgs->ctx.depthTest);
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

    VkDevice device = ctx->surface->device.device;
    VkPipelineLayout layout = ctx->pipeline.layout;
    VkRenderPass renderPass = ctx->pipeline.renderPass;
    if (ctx->initArgs->ctx.multiSample)
        ctx->pipelineIndices.fragmentOutputInterfaceIndex = t_pipeline_library_new_fragment_output_interface(&ctx->surface->library, device, layout, renderPass, VK_SAMPLE_COUNT_4_BIT, 1, &colorBlendAttachment);
    else
        ctx->pipelineIndices.fragmentOutputInterfaceIndex = t_pipeline_library_new_fragment_output_interface(&ctx->surface->library, device, layout, renderPass, VK_SAMPLE_COUNT_1_BIT, 1, &colorBlendAttachment);
    if (FU_UNLIKELY(0 > ctx->pipelineIndices.fragmentOutputInterfaceIndex)) {
        fu_error("Failed to create fragment output interface\n");
        return false;
    }

    return t_pipeline_init(&ctx->surface->library, &ctx->pipelineIndices, device, &ctx->pipeline);
}

static bool fu_context_init_synchronization_objects(FUContext* ctx)
{
    fu_print_func_name();
    fu_return_val_if_fail(ctx && ctx->initArgs, false);
    VkDevice dev = ctx->surface->device.device;
    VkSemaphoreCreateInfo semaphoreInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };
    VkFenceCreateInfo fenceInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    VkSemaphore* imageAvailable = fu_malloc(sizeof(VkSemaphore) * ctx->surface->swapchain.imageCount);
    VkSemaphore* renderFinished = fu_malloc(sizeof(VkSemaphore) * ctx->surface->swapchain.imageCount);
    VkFence* fences = fu_malloc(sizeof(VkFence) * ctx->surface->swapchain.imageCount);

    for (uint32_t i = 0; i < ctx->surface->swapchain.imageCount; i++) {
        if (FU_UNLIKELY(vkCreateSemaphore(dev, &semaphoreInfo, &defCustomAllocator, &imageAvailable[i]))) {
            fu_error("Failed to create image available semaphore[%u]\n", i);
            goto out;
        }
        if (FU_UNLIKELY(vkCreateSemaphore(dev, &semaphoreInfo, &defCustomAllocator, &renderFinished[i]))) {
            fu_error("Failed to create render finished semaphore[%u]\n", i);
            goto out;
        }
        if (FU_UNLIKELY(vkCreateFence(dev, &fenceInfo, &defCustomAllocator, &fences[i]))) {
            fu_error("Failed to create fence[%u]\n", i);
            goto out;
        }
    }

    ctx->imageAvailableSemaphores = fu_steal_pointer(&imageAvailable);
    ctx->renderFinishedSemaphores = fu_steal_pointer(&renderFinished);
    ctx->inFlightFences = fu_steal_pointer(&fences);
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
        .framebuffer = ctx->framebuffers[imageIndex],
        .renderArea = {
            .extent = sf->extent },
        .clearValueCount = ctx->attachmentCount,
        .pClearValues = ctx->clearValues
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
    VkBuffer vertexBuffer[] = { ctx->vertices.buffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdSetViewport(cmdf, 0, 1, &viewport);
    vkCmdSetScissor(cmdf, 0, 1, &scissor);
    vkCmdBindVertexBuffers(cmdf, 0, 1, vertexBuffer, offsets);
    vkCmdBindIndexBuffer(cmdf, ctx->indices.buffer, 0, VK_INDEX_TYPE_UINT32);
    // vkCmdBindDescriptorSets(cmdf, VK_PIPELINE_BIND_POINT_GRAPHICS, app->pipeline.layout, 0, 1, &T_DESC_SETS(app)[imageIndex], 0, NULL);
    vkCmdDrawIndexed(cmdf, ctx->indexCount, 1, 0, 0, 0);

    vkCmdEndRenderPass(cmdf);
    if (vkEndCommandBuffer(cmdf)) {
        fu_error("Failed to record command buffer\n");
        return false;
    }
    return true;
}

//
//  context

static void fu_context_dispose(FUObject* obj)
{
    //  例外：图形管线由用户分步创建
    fu_print_func_name();
    FUContext* ctx = (FUContext*)obj;
    VkDevice dev = ctx->surface->device.device;
    vkDeviceWaitIdle(dev);
    fu_pipeline_init_args_free(ctx->initArgs);
    vkDestroyRenderPass(dev, ctx->pipeline.renderPass, &defCustomAllocator);
    for (uint32_t i = 0; i < ctx->surface->swapchain.imageCount; i++) {
        vkDestroyFramebuffer(dev, ctx->framebuffers[i], &defCustomAllocator);
        vkDestroySemaphore(dev, ctx->imageAvailableSemaphores[i], &defCustomAllocator);
        vkDestroySemaphore(dev, ctx->renderFinishedSemaphores[i], &defCustomAllocator);
        vkDestroyFence(dev, ctx->inFlightFences[i], &defCustomAllocator);
    }
    vkDestroyCommandPool(dev, ctx->pool, &defCustomAllocator);
    vkDestroyPipelineLayout(ctx->surface->device.device, ctx->pipeline.layout, &defCustomAllocator);
    vkDestroyPipeline(ctx->surface->device.device, ctx->pipeline.pipeline, &defCustomAllocator);
    vk_uniform_buffer_destroy(&ctx->vertices);
    vk_uniform_buffer_destroy(&ctx->indices);
    fu_free(ctx->framebuffers);
    fu_free(ctx->commandBuffers);
    fu_free(ctx->imageAvailableSemaphores);
    fu_free(ctx->renderFinishedSemaphores);
    fu_free(ctx->inFlightFences);
    fu_free(ctx->clearValues);
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
    int rev = ctx->uniformBufferCount++;
    ctx->uniformBuffers = fu_realloc(ctx->uniformBuffers, sizeof(TUniformBuffer) * ctx->surface->swapchain.imageCount * ctx->uniformBufferCount);
    for (uint32_t i = 0; i < ctx->surface->swapchain.imageCount; i++) {
        if (FU_LIKELY(vk_uniform_buffer_init(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT, &ctx->uniformBuffers[ctx->surface->swapchain.imageCount * rev + i])))
            continue;
        fu_error("Failed to init desc buffer: %u\n", i);
        return -1;
    }
    ctx->initArgs->desc = t_descriptor_args_prepare(ctx->initArgs->desc, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, stage);
    ctx->initArgs->desc->bufferIndex = rev;
    return rev;
}

bool fu_context_update_input_data(FUContext* ctx, uint32_t vertexSize, const float* vertices, uint32_t indexSize, const uint32_t* indices)
{
    fu_return_val_if_fail(vertexSize && vertices && indexSize && indices, false);
    if (ctx->vertices.allocInfo.size < vertexSize) {
        vk_uniform_buffer_destroy(&ctx->vertices);
        vk_uniform_buffer_init(vertexSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &ctx->vertices);
    }
    if (ctx->indices.allocInfo.size < indexSize) {
        vk_uniform_buffer_destroy(&ctx->indices);
        vk_uniform_buffer_init(indexSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &ctx->indices);
    }
    if (ctx->vertices.allocInfo.size < vertexSize || ctx->indices.allocInfo.size < indexSize)
        return false;

    TUniformBuffer* verticesStaging = vk_uniform_buffer_new(vertexSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
    TUniformBuffer* indicesStaging = vk_uniform_buffer_new(indexSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);

    memcpy(verticesStaging->allocInfo.pMappedData, vertices, vertexSize);
    memcpy(indicesStaging->allocInfo.pMappedData, indices, indexSize);

    vk_uniform_buffer_copy(verticesStaging, ctx->vertices.buffer, ctx->pool, ctx->surface->device.generalQueue, NULL);
    vk_uniform_buffer_copy(indicesStaging, ctx->indices.buffer, ctx->pool, ctx->surface->device.generalQueue, NULL);
    ctx->indexCount = indexSize / sizeof(uint32_t);

    vk_uniform_buffer_free(verticesStaging);
    vk_uniform_buffer_free(indicesStaging);
    return true;
}

bool fu_context_present(FUContext* ctx)
{
    static uint32_t imageIndex, frameIndex = 0;
    VkDevice dev = ctx->surface->device.device;
    TSwapchain* sc = &ctx->surface->swapchain;
    vkWaitForFences(dev, 1, &ctx->inFlightFences[frameIndex], VK_TRUE, ~0U);
    VkResult rev = vkAcquireNextImageKHR(dev, sc->swapchain, ~0U, ctx->imageAvailableSemaphores[frameIndex], VK_NULL_HANDLE, &imageIndex);
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

    VkCommandBuffer cmdf = ctx->commandBuffers[frameIndex];

    vkResetFences(dev, 1, &ctx->inFlightFences[frameIndex]);
    fu_context_record_command(ctx, cmdf, imageIndex);

    VkSemaphore waitSemaphores[] = { ctx->imageAvailableSemaphores[frameIndex] };
    VkSemaphore signalSemaphores[] = { ctx->renderFinishedSemaphores[frameIndex] };
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
    if (vkQueueSubmit(ctx->surface->device.generalQueue, 1, &submitInfo, ctx->inFlightFences[frameIndex])) {
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
    frameIndex = (frameIndex + 1) % sc->imageCount;
    if (!frameIndex) {
        vkDeviceWaitIdle(dev);
        vkResetCommandPool(dev, ctx->pool, 0);
        return fu_context_init_frameBuffers(ctx, ctx->commandBuffers);
    }
    return true;
}

bool fu_window_add_context(FUWindow* win, FUContext* ctx)
{
    fu_return_val_if_fail(ctx->initArgs, false);
    ctx->surface = (fu_surface_t*)&win->surface;
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

    if (ctx->colorBuffer) {
        if (ctx->depthBuffer) {
            attachments[1] = ctx->depthBuffer->image.view;
            framebufferInfo.attachmentCount += 1;
            frameAttachmentIndex += 1;
        }
        attachments[0] = ctx->colorBuffer->image.view;
        framebufferInfo.attachmentCount += 1;
        frameAttachmentIndex += 1;
    } else if (ctx->depthBuffer) {
        attachments[1] = ctx->depthBuffer->image.view;
        framebufferInfo.attachmentCount = 2;
    }

    for (uint32_t i = 0; i < surface->swapchain.imageCount; i++) {
        attachments[frameAttachmentIndex] = surface->swapchain.images[i].view;
        if (FU_UNLIKELY(vkCreateFramebuffer(surface->device.device, &framebufferInfo, &defCustomAllocator, ctx->framebuffers + i))) {
            fu_error("Failed to create framebuffer[%u]\n", i);
            return false;
        }
    }

    return true;
}

#endif