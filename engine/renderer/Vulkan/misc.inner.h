#ifdef FU_RENDERER_TYPE_VK
#include "../backend.h"
#include "../context.h"
// #include "../misc.h"
#include "../shader.h"
#include "inner/wrapper.h"

typedef struct _fu_surface {
    TInstance instance;
    TDevice device;
    TSurface surface;
    TSwapchain swapchain;
    TPipelineLibrary library;
} fu_surface_t;
static_assert(sizeof(FUSurface) == sizeof(fu_surface_t));

typedef struct _TPipelineInitArgs {
    FUContextArgs ctx;
    FUShaderGroup* shaders;
    TDescriptorArgs2* desc;
} TPipelineInitArgs;

typedef struct _FUContext {
    FUObject parent;
    TPipelineInitArgs* initArgs;
    fu_surface_t* surface;
    TPipeline pipeline;
    TPipelineIndices pipelineIndices;
    TUniformBuffer vertices, indices;
    /** user data */
    TUniformBuffer* uniformBuffers;
    /** multi sampling */
    TImageBuffer* colorBuffer;
    /** depth test */
    TImageBuffer* depthBuffer;
    /** user texture */
    TImageBuffer* textureBuffers;
    VkFramebuffer* framebuffers;
    VkCommandPool pool;
    VkCommandBuffer* commandBuffers;
    VkSemaphore *imageAvailableSemaphores, *renderFinishedSemaphores;
    VkFence* inFlightFences;
    VkClearValue* clearValues;
    uint32_t uniformBufferCount, imageBufferCount, attachmentCount, indexCount;
} FUContext;
bool fu_context_init_framebuffers(FUContext* ctx, fu_surface_t* surface);

typedef union _fu_shader_buffer_usage {
    VkBufferUsageFlags buffer;
    VkImageUsageFlags image;
} fu_shader_buffer_usage_t;

typedef struct _fu_shader {
    uint32_t* code;
    VkShaderStageFlagBits stage;
    VkVertexInputAttributeDescription* attributes;
    TDescriptorArgs2* descriptors;
    fu_shader_buffer_usage_t usage;
    uint32_t codeSize, stride, attributeCount;
} fu_shader_t;
static_assert(sizeof(FUShader) == sizeof(fu_shader_t));

#endif