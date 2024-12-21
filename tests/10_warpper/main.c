#include <stdio.h>
#include <stdlib.h>
#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#include <cglm/cglm.h>
// #define SDL_MAIN_HANDLED
// #include <SDL2/SDL.h>
// #include <SDL2/SDL_vulkan.h>
// #include <vulkan/vulkan.h>

#include "core/_base.h"
#include "core/memory.h"
#include "libs/stb_image.h"
#include "libs/vk_mem_alloc.h"
#include "warpper/warpper.h"
#include <mimalloc.h>

#define DEF_WIN_WIDTH 800
#define DEF_WIN_HEIGHT 600

#define fu_error(format, ...) fprintf(stderr, "\033[91;49m" format "\033[0m", ##__VA_ARGS__)
#define fu_warning(format, ...) fprintf(stderr, "\033[93;49m" format "\033[0m", ##__VA_ARGS__)
#define fu_info(format, ...) fprintf(stderr, "\033[92;49m" format "\033[0m", ##__VA_ARGS__)
#define fu_print_func_name(...) (printf("%s "__VA_ARGS__ \
                                        "\n",            \
    __func__))

typedef struct _TVertex {
    vec3 position;
    vec4 color;
    vec2 uv;
} TVertex;

typedef struct _TUniformBufferObject {
    alignas(16) mat4 model;
    alignas(16) mat4 view;
    alignas(16) mat4 proj;
} TUniformBufferObject;

static const TVertex vertices[] = {
    { { -0.5f, -0.5f }, { 1.0f, 1.0f, 0.0f, 1.0f }, { 1.0f } },
    { { -0.5f, 0.5f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { { 0.5f, 0.5f }, { 0.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
    { { 0.5f, -0.5f }, { 1.0f, 0.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },

    { { -0.5f, -0.5f, -0.5f }, { 1.0f, 1.0f, 0.0f, 1.0f }, { 1.0f } },
    { { -0.5f, 0.5f, -0.5f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { { 0.5f, 0.5f, -0.5f }, { 0.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
    { { 0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } }
};

static const uint32_t indices[] = {
    0, 1, 2, 0, 2, 3,
    4, 5, 6, 4, 6, 7
};
#define DEF_IMG "E:\\Pictures\\Koala.jpg"
// #define DEF_IMG "E:\\Pictures\\bz_ladfdfdg_0011.jpg"

typedef struct _TSync {
    VkSemaphore* imageAvailable;
    VkSemaphore* renderFinished;
    VkFence* inFlight;
} TSync;

typedef struct _TBuffInitArgs {
    TFence* fence;
    TUniformBuffer* vertex;
    TUniformBuffer* index;
    TCommandBuffer* vertTranCmd;
    TCommandBuffer* idxTranCmd;
    TCommandBuffer* depthTranCmd;
} TBuffInitArgs;

typedef struct _TDrawing {
    TBuffInitArgs* args;
    TUniformBuffer vertexBuffer;
    TUniformBuffer indexBuffer;
    VkCommandPool commandPool;
    VkCommandBuffer* commandBuffers;
    VkFramebuffer* framebuffers;
} TDrawing;

#define T_CMD_POOL(app) (app->drawing.commandPool)
#define T_CMD_BUFF(app) (app->drawing.commandBuffers)
#define T_VERTEX_BUFFER(app) (app->drawing.vertexBuffer)
#define T_INDEX_BUFFER(app) (app->drawing.indexBuffer)
#define T_FRAMEBUFFER(app) (app->drawing.framebuffers)
typedef struct _TDescription {
    TDescriptor descriptor;
    TUniformBuffer* uniformBuffer;
    TImageBuffer imageBuffer;
    TImageBuffer depthBuffer;
    VkSampler sampler;
    stbi_uc* pixels;
} TDescription;
#define T_DESC_POOL(app) (app->description.descriptor.pool)
#define T_DESC_LAYOUT(app) (app->description.descriptor.layout)
#define T_DESC_SETS(app) (app->description.descriptor.sets)
#define T_DESC_BUFF(app) (app->description.uniformBuffer)
#define T_DESC_IMAGE(app) (app->description.imageBuffer)
#define T_DESC_DEPTH(app) (app->description.depthBuffer)
#define T_DESC_SAMPLER(app) (app->description.sampler)

#define T_RENDER_PASS(app) (app->pipeline.renderPass)
#define T_PIPELINE_LAYOUT(app) (app->pipeline.layout)
#define T_PIPELINE(app) (app->pipeline.pipeline)

#define T_SWAPCHAIN(app) (app->swapchain.swapchain)
#define T_SWAPCHAIN_IMAGES(app) (app->swapchain.images)

#define T_PHYSICAL_DEVICE(app) (app->device.physicalDevice.physicalDevice)
#define T_DEVICE(app) (app->device.device)
#define T_QUEUE(app) (app->device.generalQueue)

#define T_SURFACE(app) (app->surface.surface)
#define T_WINDOW(app) (app->surface.window)

typedef struct _TApp {
    TSync sync;
    TDrawing drawing;
    TPipeline pipeline;
    TDescription description;
    TSwapchain swapchain;
    TDevice device;
    TSurface surface;
    TInstance instance;
} TApp;
//
//  Sync

static void t_app_destroy_sync(TApp* app)
{
    for (uint32_t i = 0; i < app->swapchain.imageCount; i++) {
        vkDestroySemaphore(T_DEVICE(app), app->sync.imageAvailable[i], &defCustomAllocator);
        vkDestroySemaphore(T_DEVICE(app), app->sync.renderFinished[i], &defCustomAllocator);
        vkDestroyFence(T_DEVICE(app), app->sync.inFlight[i], &defCustomAllocator);
    }
    fu_free(app->sync.imageAvailable);
    fu_free(app->sync.renderFinished);
    fu_free(app->sync.inFlight);
}

static bool t_app_init_sync(TApp* app)
{
    VkSemaphoreCreateInfo semaphoreInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };
    VkFenceCreateInfo fenceInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };
    bool rev = false;
    app->sync.imageAvailable = (VkSemaphore*)fu_malloc0_n(app->swapchain.imageCount, sizeof(VkSemaphore));
    app->sync.renderFinished = (VkSemaphore*)fu_malloc0_n(app->swapchain.imageCount, sizeof(VkSemaphore));
    app->sync.inFlight = (VkFence*)fu_malloc0_n(app->swapchain.imageCount, sizeof(VkFence));
    for (uint32_t i = 0; i < app->swapchain.imageCount; i++) {
        rev = !vkCreateSemaphore(T_DEVICE(app), &semaphoreInfo, &defCustomAllocator, &app->sync.imageAvailable[i]);
        rev = rev && !vkCreateSemaphore(T_DEVICE(app), &semaphoreInfo, &defCustomAllocator, &app->sync.renderFinished[i]);
        rev = rev && !vkCreateFence(T_DEVICE(app), &fenceInfo, &defCustomAllocator, &app->sync.inFlight[i]);
        if (!rev)
            return false;
    }

    return rev;
}

//
//  drawing stuff
static void t_app_destroy_buffers__init_args(TApp* app)
{
    if (!app->drawing.args)
        return;

    vk_uniform_buffer_free(app->drawing.args->vertex);
    vk_uniform_buffer_free(app->drawing.args->index);

    vk_command_buffer_free(app->drawing.args->vertTranCmd);
    vk_command_buffer_free(app->drawing.args->idxTranCmd);

    vk_fence_free(app->drawing.args->fence);
    fu_free(app->drawing.args);
    app->drawing.args = NULL;
}

static void t_app_init_buffers__init_args(TApp* app)
{
    app->drawing.args = fu_malloc0(sizeof(TBuffInitArgs));
    app->drawing.args->fence = vk_fence_new(T_DEVICE(app), 2, false);
}

static void t_app_destroy_buffers(TApp* app)
{
    t_app_destroy_buffers__init_args(app);
    for (uint32_t i = 0; i < app->swapchain.imageCount; i++)
        vkDestroyFramebuffer(T_DEVICE(app), T_FRAMEBUFFER(app)[i], &defCustomAllocator);
    vkDestroyCommandPool(T_DEVICE(app), T_CMD_POOL(app), &defCustomAllocator);
    vk_uniform_buffer_destroy(&T_VERTEX_BUFFER(app));
    vk_uniform_buffer_destroy(&T_INDEX_BUFFER(app));
    fu_free(T_FRAMEBUFFER(app));
    fu_free(T_CMD_BUFF(app));
}

static bool t_app_init_buffers__framebuffers(TApp* app)
{
    VkImageView attachments[] = { NULL, T_DESC_DEPTH(app).buff.view };
    VkFramebufferCreateInfo framebufferInfo = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = T_RENDER_PASS(app),
        .attachmentCount = FU_N_ELEMENTS(attachments),
        .pAttachments = attachments,
        .width = app->surface.extent.width,
        .height = app->surface.extent.height,
        .layers = 1
    };

    for (uint32_t i = 0; i < app->swapchain.imageCount; i++) {
        attachments[0] = T_SWAPCHAIN_IMAGES(app)[i].view;
        if (vkCreateFramebuffer(T_DEVICE(app), &framebufferInfo, &defCustomAllocator, &T_FRAMEBUFFER(app)[i])) {
            fu_error("Failed to create framebuffer\n");
            return false;
        }
    }

    return true;
}

static bool t_app_init_buffers__vertex(TApp* app)
{
    TUniformBuffer* staging = vk_uniform_buffer_new(sizeof(vertices), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
    if (!staging)
        return false;
    memcpy(staging->allocInfo.pMappedData, vertices, sizeof(vertices));

    if (!vk_uniform_buffer_init(sizeof(vertices), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &T_VERTEX_BUFFER(app))) {
        fu_error("Failed to create vertex buffer\n");
        vk_uniform_buffer_free(staging);
        return false;
    }
    app->drawing.args->vertTranCmd = vk_uniform_buffer_copy(T_CMD_POOL(app), T_QUEUE(app), &T_VERTEX_BUFFER(app), staging, app->drawing.args->fence->fences[0]);
    if (!app->drawing.args->vertTranCmd) {
        vk_uniform_buffer_free(staging);
        return false;
    }

    app->drawing.args->vertex = fu_steal_pointer(&staging);
    return true;
}

static bool t_app_init_buffers__index(TApp* app)
{
    TUniformBuffer* staging = vk_uniform_buffer_new(sizeof(indices), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
    if (!staging)
        return false;
    memcpy(staging->allocInfo.pMappedData, indices, sizeof(indices));

    if (!vk_uniform_buffer_init(sizeof(indices), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &T_INDEX_BUFFER(app))) {
        fu_error("Failed to create index buffer\n");
        vk_uniform_buffer_free(staging);
        return false;
    }
    app->drawing.args->idxTranCmd = vk_uniform_buffer_copy(T_CMD_POOL(app), T_QUEUE(app), &T_INDEX_BUFFER(app), staging, app->drawing.args->fence->fences[1]);
    if (!app->drawing.args->idxTranCmd) {
        vk_uniform_buffer_free(staging);
        return false;
    }

    app->drawing.args->index = fu_steal_pointer(&staging);
    return true;
}

static bool t_app_init_buffers(TApp* app)
{
    T_FRAMEBUFFER(app) = fu_malloc0_n(app->swapchain.imageCount, sizeof(VkFramebuffer));
    if (!t_app_init_buffers__framebuffers(app))
        return false;

    VkCommandPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = app->device.queueFamilyIndex,
    };
    if (vkCreateCommandPool(T_DEVICE(app), &poolInfo, &defCustomAllocator, &T_CMD_POOL(app))) {
        fu_error("Failed to create command pool\n");
        return false;
    }
    t_app_init_buffers__init_args(app);

    if (!t_app_init_buffers__vertex(app))
        return false;
    if (!t_app_init_buffers__index(app))
        return false;
    VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = T_CMD_POOL(app),
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = app->swapchain.imageCount
    };
    T_CMD_BUFF(app) = fu_malloc0_n(app->swapchain.imageCount, sizeof(VkCommandBuffer));
    if (vkAllocateCommandBuffers(T_DEVICE(app), &allocInfo, T_CMD_BUFF(app))) {
        fu_error("Failed to allocate command buffers\n");
        return false;
    }
    vkWaitForFences(T_DEVICE(app), 2, app->drawing.args->fence->fences, VK_TRUE, ~0U);
    t_app_destroy_buffers__init_args(app);
    return true;
}

static bool t_command_buffer_recording(TApp* app, VkCommandBuffer cmdf, uint32_t imageIndex)
{
    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pInheritanceInfo = NULL
    };
    if (vkBeginCommandBuffer(cmdf, &beginInfo)) {
        fu_error("Failed to begin recording command buffer\n");
        return false;
    }
    static const VkClearValue clearColor[] = {
        { { { 0.0f, 0.0f, 0.0f, 1.0f } } },
        { .depthStencil = { 1.0f, 0 } },
    };
    VkRenderPassBeginInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = T_RENDER_PASS(app),
        .framebuffer = T_FRAMEBUFFER(app)[imageIndex],
        .renderArea = {
            .extent = app->surface.extent },
        .clearValueCount = FU_N_ELEMENTS(clearColor),
        .pClearValues = clearColor
    };
    vkCmdBeginRenderPass(cmdf, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmdf, VK_PIPELINE_BIND_POINT_GRAPHICS, T_PIPELINE(app));

    VkViewport viewport = {
        .width = (float)app->surface.extent.width,
        .height = (float)app->surface.extent.height,
        .maxDepth = 1.0f
    };
    VkRect2D scissor = {
        .extent = app->surface.extent
    };
    VkBuffer vertexBuffer[] = { T_VERTEX_BUFFER(app).buffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdSetViewport(cmdf, 0, 1, &viewport);
    vkCmdSetScissor(cmdf, 0, 1, &scissor);
    vkCmdBindVertexBuffers(cmdf, 0, 1, vertexBuffer, offsets);
    vkCmdBindIndexBuffer(cmdf, T_INDEX_BUFFER(app).buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(cmdf, VK_PIPELINE_BIND_POINT_GRAPHICS, app->pipeline.layout, 0, 1, &T_DESC_SETS(app)[imageIndex], 0, NULL);
    vkCmdDrawIndexed(cmdf, FU_N_ELEMENTS(indices), 1, 0, 0, 0);

    vkCmdEndRenderPass(cmdf);
    if (vkEndCommandBuffer(cmdf)) {
        fu_error("Failed to record command buffer\n");
        return false;
    }
    return true;
}

static bool t_app_init_pipeline__render_pass2(TApp* app)
{
    const VkAttachmentDescription colorAttachment = {
        .format = app->surface.format.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    const VkAttachmentReference colorAttachmentRef = {
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    const VkAttachmentDescription depthAttachment = {
        .format = T_DESC_DEPTH(app).format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    const VkAttachmentReference depthAttachmentRef = {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    const VkAttachmentDescription attachments[] = { colorAttachment, depthAttachment };

    const VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef,
        .pDepthStencilAttachment = &depthAttachmentRef
    };

    const VkSubpassDependency dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
    };

    return t_pipeline_init_render_pass(&app->pipeline, T_DEVICE(app), FU_N_ELEMENTS(attachments), attachments, 1, &subpass, 1, &dependency);
}

static bool t_app_pipeline_library_init_input_stage2(TApp* app, TPipelineIndices* args)
{
    const VkVertexInputBindingDescription binding = {
        .stride = sizeof(TVertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    const VkVertexInputAttributeDescription attribute[] = {
        { .format = VK_FORMAT_R32G32B32_SFLOAT },
        { .location = 1, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = offsetof(TVertex, color) },
        { .location = 2, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(TVertex, uv) },
    };

    args->vertexInputInterfaceIndex = t_pipeline_new_vertex_input_interface(&app->pipeline, T_DEVICE(app), 1, &binding, FU_N_ELEMENTS(attribute), attribute);
    return 0 <= args->vertexInputInterfaceIndex;
}

static bool t_app_pipeline_library_init_shader_stage2(TApp* app, TPipelineIndices* args)
{
    const uint32_t shaderVertCode[] = {
        0x07230203, 0x00010600, 0x0008000b, 0x00000035, 0x00000000, 0x00020011, 0x00000001, 0x0006000b,
        0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001,
        0x000c000f, 0x00000000, 0x00000004, 0x6e69616d, 0x00000000, 0x0000000d, 0x00000013, 0x00000021,
        0x0000002b, 0x0000002d, 0x00000031, 0x00000033, 0x00030003, 0x00000002, 0x000001cc, 0x00040005,
        0x00000004, 0x6e69616d, 0x00000000, 0x00060005, 0x0000000b, 0x505f6c67, 0x65567265, 0x78657472,
        0x00000000, 0x00060006, 0x0000000b, 0x00000000, 0x505f6c67, 0x7469736f, 0x006e6f69, 0x00070006,
        0x0000000b, 0x00000001, 0x505f6c67, 0x746e696f, 0x657a6953, 0x00000000, 0x00070006, 0x0000000b,
        0x00000002, 0x435f6c67, 0x4470696c, 0x61747369, 0x0065636e, 0x00070006, 0x0000000b, 0x00000003,
        0x435f6c67, 0x446c6c75, 0x61747369, 0x0065636e, 0x00030005, 0x0000000d, 0x00000000, 0x00070005,
        0x00000011, 0x66696e55, 0x426d726f, 0x65666675, 0x6a624f72, 0x00746365, 0x00050006, 0x00000011,
        0x00000000, 0x65646f6d, 0x0000006c, 0x00050006, 0x00000011, 0x00000001, 0x77656976, 0x00000000,
        0x00050006, 0x00000011, 0x00000002, 0x6a6f7270, 0x00000000, 0x00030005, 0x00000013, 0x006f6275,
        0x00050005, 0x00000021, 0x6f506e69, 0x69746973, 0x00006e6f, 0x00050005, 0x0000002b, 0x67617266,
        0x6f6c6f43, 0x00000072, 0x00040005, 0x0000002d, 0x6f436e69, 0x00726f6c, 0x00040005, 0x00000031,
        0x67617266, 0x00005655, 0x00040005, 0x00000033, 0x56556e69, 0x00000000, 0x00050048, 0x0000000b,
        0x00000000, 0x0000000b, 0x00000000, 0x00050048, 0x0000000b, 0x00000001, 0x0000000b, 0x00000001,
        0x00050048, 0x0000000b, 0x00000002, 0x0000000b, 0x00000003, 0x00050048, 0x0000000b, 0x00000003,
        0x0000000b, 0x00000004, 0x00030047, 0x0000000b, 0x00000002, 0x00040048, 0x00000011, 0x00000000,
        0x00000005, 0x00050048, 0x00000011, 0x00000000, 0x00000023, 0x00000000, 0x00050048, 0x00000011,
        0x00000000, 0x00000007, 0x00000010, 0x00040048, 0x00000011, 0x00000001, 0x00000005, 0x00050048,
        0x00000011, 0x00000001, 0x00000023, 0x00000040, 0x00050048, 0x00000011, 0x00000001, 0x00000007,
        0x00000010, 0x00040048, 0x00000011, 0x00000002, 0x00000005, 0x00050048, 0x00000011, 0x00000002,
        0x00000023, 0x00000080, 0x00050048, 0x00000011, 0x00000002, 0x00000007, 0x00000010, 0x00030047,
        0x00000011, 0x00000002, 0x00040047, 0x00000013, 0x00000022, 0x00000000, 0x00040047, 0x00000013,
        0x00000021, 0x00000000, 0x00040047, 0x00000021, 0x0000001e, 0x00000000, 0x00040047, 0x0000002b,
        0x0000001e, 0x00000000, 0x00040047, 0x0000002d, 0x0000001e, 0x00000001, 0x00040047, 0x00000031,
        0x0000001e, 0x00000001, 0x00040047, 0x00000033, 0x0000001e, 0x00000002, 0x00020013, 0x00000002,
        0x00030021, 0x00000003, 0x00000002, 0x00030016, 0x00000006, 0x00000020, 0x00040017, 0x00000007,
        0x00000006, 0x00000004, 0x00040015, 0x00000008, 0x00000020, 0x00000000, 0x0004002b, 0x00000008,
        0x00000009, 0x00000001, 0x0004001c, 0x0000000a, 0x00000006, 0x00000009, 0x0006001e, 0x0000000b,
        0x00000007, 0x00000006, 0x0000000a, 0x0000000a, 0x00040020, 0x0000000c, 0x00000003, 0x0000000b,
        0x0004003b, 0x0000000c, 0x0000000d, 0x00000003, 0x00040015, 0x0000000e, 0x00000020, 0x00000001,
        0x0004002b, 0x0000000e, 0x0000000f, 0x00000000, 0x00040018, 0x00000010, 0x00000007, 0x00000004,
        0x0005001e, 0x00000011, 0x00000010, 0x00000010, 0x00000010, 0x00040020, 0x00000012, 0x00000002,
        0x00000011, 0x0004003b, 0x00000012, 0x00000013, 0x00000002, 0x0004002b, 0x0000000e, 0x00000014,
        0x00000002, 0x00040020, 0x00000015, 0x00000002, 0x00000010, 0x0004002b, 0x0000000e, 0x00000018,
        0x00000001, 0x00040017, 0x0000001f, 0x00000006, 0x00000003, 0x00040020, 0x00000020, 0x00000001,
        0x0000001f, 0x0004003b, 0x00000020, 0x00000021, 0x00000001, 0x0004002b, 0x00000006, 0x00000023,
        0x3f800000, 0x00040020, 0x00000029, 0x00000003, 0x00000007, 0x0004003b, 0x00000029, 0x0000002b,
        0x00000003, 0x00040020, 0x0000002c, 0x00000001, 0x00000007, 0x0004003b, 0x0000002c, 0x0000002d,
        0x00000001, 0x00040017, 0x0000002f, 0x00000006, 0x00000002, 0x00040020, 0x00000030, 0x00000003,
        0x0000002f, 0x0004003b, 0x00000030, 0x00000031, 0x00000003, 0x00040020, 0x00000032, 0x00000001,
        0x0000002f, 0x0004003b, 0x00000032, 0x00000033, 0x00000001, 0x00050036, 0x00000002, 0x00000004,
        0x00000000, 0x00000003, 0x000200f8, 0x00000005, 0x00050041, 0x00000015, 0x00000016, 0x00000013,
        0x00000014, 0x0004003d, 0x00000010, 0x00000017, 0x00000016, 0x00050041, 0x00000015, 0x00000019,
        0x00000013, 0x00000018, 0x0004003d, 0x00000010, 0x0000001a, 0x00000019, 0x00050092, 0x00000010,
        0x0000001b, 0x00000017, 0x0000001a, 0x00050041, 0x00000015, 0x0000001c, 0x00000013, 0x0000000f,
        0x0004003d, 0x00000010, 0x0000001d, 0x0000001c, 0x00050092, 0x00000010, 0x0000001e, 0x0000001b,
        0x0000001d, 0x0004003d, 0x0000001f, 0x00000022, 0x00000021, 0x00050051, 0x00000006, 0x00000024,
        0x00000022, 0x00000000, 0x00050051, 0x00000006, 0x00000025, 0x00000022, 0x00000001, 0x00050051,
        0x00000006, 0x00000026, 0x00000022, 0x00000002, 0x00070050, 0x00000007, 0x00000027, 0x00000024,
        0x00000025, 0x00000026, 0x00000023, 0x00050091, 0x00000007, 0x00000028, 0x0000001e, 0x00000027,
        0x00050041, 0x00000029, 0x0000002a, 0x0000000d, 0x0000000f, 0x0003003e, 0x0000002a, 0x00000028,
        0x0004003d, 0x00000007, 0x0000002e, 0x0000002d, 0x0003003e, 0x0000002b, 0x0000002e, 0x0004003d,
        0x0000002f, 0x00000034, 0x00000033, 0x0003003e, 0x00000031, 0x00000034, 0x000100fd, 0x00010038
    };
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &T_DESC_LAYOUT(app)
    };

    // stage
    VkShaderModuleCreateInfo shaderInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = sizeof(shaderVertCode),
        .pCode = shaderVertCode
    };
    VkPipelineShaderStageCreateInfo shaderStageState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = &shaderInfo,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .pName = "main"
    };

    args->preRasterizationShaderIndex = t_pipeline_init_layout_and_pre_rasterization_shaders(&app->pipeline, T_DEVICE(app), 1, &T_DESC_LAYOUT(app), 1, &shaderStageState);
    return 0 <= args->preRasterizationShaderIndex;
}

static bool t_app_pipeline_library_init_fragment_stage2(TApp* app, TPipelineIndices* args)
{
    const uint32_t shaderFragCode[] = {
        0x07230203, 0x00010600, 0x0008000b, 0x00000018, 0x00000000, 0x00020011, 0x00000001, 0x0006000b,
        0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001,
        0x0009000f, 0x00000004, 0x00000004, 0x6e69616d, 0x00000000, 0x00000009, 0x0000000d, 0x00000011,
        0x00000015, 0x00030010, 0x00000004, 0x00000007, 0x00030003, 0x00000002, 0x000001cc, 0x00040005,
        0x00000004, 0x6e69616d, 0x00000000, 0x00050005, 0x00000009, 0x4374756f, 0x726f6c6f, 0x00000000,
        0x00050005, 0x0000000d, 0x53786574, 0x6c706d61, 0x00007265, 0x00040005, 0x00000011, 0x67617266,
        0x00005655, 0x00050005, 0x00000015, 0x67617266, 0x6f6c6f43, 0x00000072, 0x00040047, 0x00000009,
        0x0000001e, 0x00000000, 0x00040047, 0x0000000d, 0x00000022, 0x00000000, 0x00040047, 0x0000000d,
        0x00000021, 0x00000001, 0x00040047, 0x00000011, 0x0000001e, 0x00000001, 0x00040047, 0x00000015,
        0x0000001e, 0x00000000, 0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002, 0x00030016,
        0x00000006, 0x00000020, 0x00040017, 0x00000007, 0x00000006, 0x00000004, 0x00040020, 0x00000008,
        0x00000003, 0x00000007, 0x0004003b, 0x00000008, 0x00000009, 0x00000003, 0x00090019, 0x0000000a,
        0x00000006, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x0003001b,
        0x0000000b, 0x0000000a, 0x00040020, 0x0000000c, 0x00000000, 0x0000000b, 0x0004003b, 0x0000000c,
        0x0000000d, 0x00000000, 0x00040017, 0x0000000f, 0x00000006, 0x00000002, 0x00040020, 0x00000010,
        0x00000001, 0x0000000f, 0x0004003b, 0x00000010, 0x00000011, 0x00000001, 0x00040020, 0x00000014,
        0x00000001, 0x00000007, 0x0004003b, 0x00000014, 0x00000015, 0x00000001, 0x00050036, 0x00000002,
        0x00000004, 0x00000000, 0x00000003, 0x000200f8, 0x00000005, 0x0004003d, 0x0000000b, 0x0000000e,
        0x0000000d, 0x0004003d, 0x0000000f, 0x00000012, 0x00000011, 0x00050057, 0x00000007, 0x00000013,
        0x0000000e, 0x00000012, 0x0004003d, 0x00000007, 0x00000016, 0x00000015, 0x00050085, 0x00000007,
        0x00000017, 0x00000013, 0x00000016, 0x0003003e, 0x00000009, 0x00000017, 0x000100fd, 0x00010038
    };

    const VkShaderModuleCreateInfo shaderInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = sizeof(shaderFragCode),
        .pCode = shaderFragCode
    };

    const VkPipelineShaderStageCreateInfo shaderStageState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = &shaderInfo,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pName = "main"
    };

    args->fragmentShaderIndex = t_pipeline_new_fragment_shader(&app->pipeline, T_DEVICE(app), 1, &shaderStageState, true);
    return 0 <= args->fragmentShaderIndex;
}

static bool t_app_init_pipeline__graphics_pipeline2(TApp* app, TPipelineIndices* args)
{
    // color blend state
    const VkPipelineColorBlendAttachmentState colorBlendAttachment = {
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };

    args->fragmentOutputInterfaceIndex = t_pipeline_new_fragment_output_interface(&app->pipeline, T_DEVICE(app), VK_SAMPLE_COUNT_1_BIT, 1, &colorBlendAttachment);
    if (FU_UNLIKELY(0 > args->fragmentOutputInterfaceIndex)) {
        fu_error("Failed to create graphics pipeline @fragmentOutputStage\n");
        return false;
    }

    return t_pipeline_init(&app->pipeline, T_DEVICE(app), args);
}

static void t_app_pipeline_destroy2(TApp* app)
{
    t_pipeline_destroy_all(&app->pipeline, T_DEVICE(app));
    fu_free(app->pipeline.library);
}

static bool t_app_init_pipeline(TApp* app)
{
    TPipelineIndices args = {};
    app->pipeline.library = fu_malloc0(sizeof(TPipelineLibrary));
    bool rev = t_app_init_pipeline__render_pass2(app);
    rev = rev && t_app_pipeline_library_init_input_stage2(app, &args);
    rev = rev && t_app_pipeline_library_init_shader_stage2(app, &args);
    rev = rev && t_app_pipeline_library_init_fragment_stage2(app, &args);
    return t_app_init_pipeline__graphics_pipeline2(app, &args);
}
//
//  description

static void t_app_destroy_description(TApp* app)
{
    vkDestroyDescriptorPool(T_DEVICE(app), T_DESC_POOL(app), &defCustomAllocator);
    vkDestroyDescriptorSetLayout(T_DEVICE(app), T_DESC_LAYOUT(app), &defCustomAllocator);
    vkDestroySampler(T_DEVICE(app), T_DESC_SAMPLER(app), &defCustomAllocator);
    for (uint32_t i = 0; i < app->swapchain.imageCount; i++) {
        vk_uniform_buffer_destroy(&T_DESC_BUFF(app)[i]);
    }
    vk_image_buffer_destroy(&T_DESC_IMAGE(app));
    vk_image_buffer_destroy(&T_DESC_DEPTH(app));
    fu_free(T_DESC_SETS(app));
    fu_free(T_DESC_BUFF(app));
}

static bool t_app_init_description__uniform(TApp* app)
{
    T_DESC_BUFF(app) = fu_malloc(sizeof(TUniformBuffer) * app->swapchain.imageCount);
    for (uint32_t i = 0; i < app->swapchain.imageCount; i++) {
        if (!vk_uniform_buffer_init(sizeof(TUniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT, &T_DESC_BUFF(app)[i])) {
            fu_error("Failed to init desc buffer: %u\n", i);
            return false;
        }
    }
    return true;
}

static bool t_app_init_description__depth(TApp* app)
{
    T_DESC_DEPTH(app).extent.width = app->surface.extent.width;
    T_DESC_DEPTH(app).extent.height = app->surface.extent.height;
    return vk_image_buffer_init(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_IMAGE_ASPECT_DEPTH_BIT, 1, VK_SAMPLE_COUNT_1_BIT, &T_DESC_DEPTH(app));
}

static bool t_app_init_description__texture(TApp* app)
{
    if (t_app_init_description__depth(app)) {
        int width, height, channels;
        stbi_uc* pixels = stbi_load(DEF_IMG, &width, &height, &channels, STBI_rgb_alpha);
        if (!pixels) {
            fu_error("Failed to load texture image!\n");
            return false;
        }
        if (vk_image_buffer_init_from_data(pixels, width, height, 1, VK_SAMPLE_COUNT_1_BIT, &T_DESC_IMAGE(app))) {
            stbi_image_free(pixels);
            return vk_sampler_init(VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, 1, &T_DESC_SAMPLER(app));
        }
        stbi_image_free(pixels);
    }
    return false;
}

typedef struct _TDescArgs {
    TApp* app;
    TDescriptorArgs* args;
    VkDescriptorBufferInfo bufferInfo;
    VkDescriptorImageInfo imageInfo;
} TDescArgs;

static TDescArgs* t_desc_args_new(TApp* app)
{
    TDescArgs* args = fu_malloc0(sizeof(TDescArgs));
    args->args = vk_descriptor_args_new(app->swapchain.imageCount, 2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    args->app = app;
    args->bufferInfo.range = sizeof(TUniformBufferObject);
    args->imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    args->imageInfo.imageView = T_DESC_IMAGE(app).buff.view;
    args->imageInfo.sampler = T_DESC_SAMPLER(app);
    return args;
}

static void t_desc_args_free(TDescArgs* args)
{
    vk_descriptor_args_free(args->args);
    fu_free(args);
}

static void t_app_init_description__update_set(VkWriteDescriptorSet* app, uint32_t index, void* usd)
{
    TDescArgs* args = usd;
    args->bufferInfo.buffer = T_DESC_BUFF(args->app)[index].buffer;
}

static bool t_app_init_description(TApp* app)
{
    bool rev = t_app_init_description__uniform(app);
    rev = rev && t_app_init_description__texture(app);
    if (rev) {
        TDescArgs* args = t_desc_args_new(app);
        vk_descriptor_args_update_info(args->args, (TWriteDescriptorSetInfo) { .bufferInfo = &args->bufferInfo }, (TWriteDescriptorSetInfo) { .imageInfo = &args->imageInfo });
        rev = t_descriptor_init(T_DEVICE(app), args->args, &app->description.descriptor, t_app_init_description__update_set, args);
        t_desc_args_free(args);
    }
    return rev;
}

static void t_app_update_uniform_buffer(TApp* app, uint32_t imageIndex)
{
    TUniformBufferObject ubo = {
        .model = GLM_MAT4_IDENTITY_INIT,
    };
#ifdef FU_USE_GLFW
    glm_rotate(ubo.model, glfwGetTime() * glm_rad(90.0f), (vec3) { 0.0f, 0.0f, 1.0f });
#else
    glm_rotate(ubo.model, SDL_GetTicks() / 100.0f * glm_rad(5.0f), (vec3) { 0.0f, 0.0f, 1.0f });
#endif
    glm_lookat((vec3) { 2.0f, 2.0f, 2.0f }, GLM_VEC3_ZERO, (vec3) { 0.0f, 0.0f, 1.0f }, ubo.view);
    glm_perspective(glm_rad(30.0f), app->surface.extent.width / (float)app->surface.extent.height, 0.1f, 10.0f, ubo.proj);
    ubo.proj[1][1] *= -1;
    memcpy(T_DESC_BUFF(app)[imageIndex].allocInfo.pMappedData, &ubo, sizeof(TUniformBufferObject));
}

//
//  swapchain
static void t_app_swapchain_cleanup(TApp* app)
{
    for (uint32_t i = 0; i < app->swapchain.imageCount; i++) {
        vkDestroyFramebuffer(T_DEVICE(app), T_FRAMEBUFFER(app)[i], &defCustomAllocator);
    }
    vk_image_buffer_destroy(&T_DESC_DEPTH(app));
}

static void t_app_destroy_swapchain(TApp* app)
{
    for (uint32_t i = 0; i < app->swapchain.imageCount; i++)
        vkDestroyImageView(T_DEVICE(app), T_SWAPCHAIN_IMAGES(app)[i].view, &defCustomAllocator);
    vkDestroySwapchainKHR(T_DEVICE(app), T_SWAPCHAIN(app), &defCustomAllocator);
    fu_free(T_SWAPCHAIN_IMAGES(app));
}

static void t_app_free(TApp* app)
{
    t_app_destroy_sync(app);
    t_app_destroy_buffers(app);

    t_app_pipeline_destroy2(app);
    t_app_destroy_description(app);
    t_swapchain_destroy(&app->swapchain, &app->device);
    t_device_destroy(&app->device);
    t_surface_destroy(&app->surface, app->instance.instance);
    t_instance_destroy(&app->instance);

    fu_free(app);
};

static TApp* t_app_new()
{
    TApp* app = (TApp*)fu_malloc0(sizeof(TApp));
    if (!t_instance_init("test", VK_MAKE_VERSION(0, 0, 3), &app->instance))
        goto out;
    if (!t_surface_init(1200, 800, "test", app->instance.instance, &app->surface))
        goto out;
    if (!t_device_init(app->instance.instance, T_SURFACE(app), &app->device))
        goto out;
    if (!t_swapchain_init(app->instance.instance, &app->device, &app->surface, &app->swapchain))
        goto out;
    if (!t_app_init_description(app))
        goto out;
    if (!t_app_init_pipeline(app))
        goto out;
    if (!t_app_init_buffers(app))
        goto out;
    if (!t_app_init_sync(app))
        goto out;
    return app;
out:
    t_app_free(app);
    return NULL;
};

static void t_app_draw(TApp* app)
{
    static uint32_t imageIndex, frameIndex = 0;
    vkWaitForFences(T_DEVICE(app), 1, &app->sync.inFlight[frameIndex], VK_TRUE, ~0U);
    VkResult rev = vkAcquireNextImageKHR(T_DEVICE(app), T_SWAPCHAIN(app), ~0U, app->sync.imageAvailable[frameIndex], VK_NULL_HANDLE, &imageIndex);
    if (rev) {
        if (VK_ERROR_OUT_OF_DATE_KHR == rev) {
            app->swapchain.outOfDate = true;
            return;
        }
        if (VK_SUBOPTIMAL_KHR != rev) {
            fu_error("Failed to acquire swapchain image\n");
            return;
        }
        fu_warning("swapchain suboptiomal\n");
    }

    VkCommandBuffer cmdf = T_CMD_BUFF(app)[frameIndex];

    vkResetFences(T_DEVICE(app), 1, &app->sync.inFlight[frameIndex]);
    vkResetCommandBuffer(cmdf, 0);
    t_app_update_uniform_buffer(app, imageIndex);
    if (!t_command_buffer_recording(app, cmdf, imageIndex)) {
        fu_error("Failed to record command buffer\n");
        return;
    }

    VkSemaphore waitSemaphores[] = { app->sync.imageAvailable[frameIndex] };
    VkSemaphore signalSemaphores[] = { app->sync.renderFinished[frameIndex] };
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
    if (vkQueueSubmit(T_QUEUE(app), 1, &submitInfo, app->sync.inFlight[frameIndex])) {
        fu_error("Failed to submit command buffer\n");
        return;
    }
    VkSwapchainKHR swapchains[] = { T_SWAPCHAIN(app) };
    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signalSemaphores,
        .swapchainCount = 1,
        .pSwapchains = swapchains,
        .pImageIndices = &imageIndex
    };
    rev = vkQueuePresentKHR(T_QUEUE(app), &presentInfo);
    if (rev) {
        if (VK_ERROR_OUT_OF_DATE_KHR == rev || VK_SUBOPTIMAL_KHR == rev)
            app->swapchain.outOfDate = true;
        else {
            fu_error("Failed to present command buffer\n");
        }
    }
    frameIndex = (frameIndex + 1) % app->swapchain.imageCount;
}

static bool t_app_check_and_exchange(TApp* app, const bool next)
{
    const bool prev = app->swapchain.outOfDate;
    if (prev != next)
        app->swapchain.outOfDate = next;
    return prev;
}

static void t_app_swapchain_update(TApp* app)
{

    while (glfwGetWindowAttrib(T_WINDOW(app), GLFW_ICONIFIED)) {
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(T_DEVICE(app));

    t_app_swapchain_cleanup(app);

    t_swapchain_update(&app->swapchain, &app->device, &app->surface);
    t_app_init_description__depth(app);
    // t_app_init_buffers__color(app);
    t_app_init_buffers__framebuffers(app);
}

static void t_app_run(TApp* app)
{
#ifdef FU_USE_GLFW
    while (!glfwWindowShouldClose(app->surface.window)) {
        glfwPollEvents();
        t_app_draw(app);
        if (t_app_check_and_exchange(app, false))
            t_app_swapchain_update(app);
    }
#else
    SDL_Event ev;
    bool running = true;
    do {
        while (SDL_PollEvent(&ev)) {
            if (SDL_QUIT == ev.type) {
                running = false;
                break;
            }
            if (SDL_WINDOWEVENT == ev.type) {
                if (SDL_WINDOWEVENT_CLOSE == ev.window.event) {
                    running = false;
                    break;
                }
                if (SDL_WINDOWEVENT_RESIZED == ev.window.event || SDL_WINDOWEVENT_RESTORED == ev.window.event) {
                    app->swapchain.outOfDate = true;
                }
            }
        }
        t_app_draw(app);
        if (t_app_check_and_exchange(app, false))
            t_app_swapchain_update(app);
    } while (running);
#endif
    vkDeviceWaitIdle(T_DEVICE(app));
}

int main(void)
{
    TApp* app = t_app_new();
    if (!app)
        return 1;
    t_app_run(app);
    t_app_free(app);
    printf("bye");
    return 0;
}