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

typedef struct _TDescriptor {
    VkDescriptorSetLayout layout;
    VkDescriptorPool pool;
    VkDescriptorSet* sets;
    uint64_t* offsets;
    uint64_t* ranges;
    uint64_t count, size;
} TDescriptor;

typedef struct _TBuffer {
    TUniformBuffer vertices;
    TUniformBuffer uniforms;
    TImageBuffer* images;
    /** multi sampling */
    TImageBuffer* color;
    /** depth test */
    TImageBuffer* depth;
    VkSampler sampler;
    VkCommandBuffer* commands;
    uint64_t vertexSize, indexSize, indexCount, uniformCount, imageCount, uniformSize;
} TBuffer;

typedef struct _TFrame {
    VkFramebuffer* buffers;
    VkClearValue* clearValues;
    VkCommandPool commandPool;
    uint64_t attachmentCount, currFrame;
} TFrame;

typedef struct _TSynchronizationObjects {
    VkSemaphore *available, *finished;
    VkFence* inFlight;
} TSynchronizationObjects;

typedef struct _FUContext {
    FUObject parent;
    TPipelineInitArgs* initArgs;
    fu_surface_t* surface;
    TDescriptor descriptor;
    TBuffer buffer;
    TFrame frame;
    TPipeline pipeline;
    TPipelineIndices pipelineIndices;
    TSynchronizationObjects sync;
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
    // TDescriptorArgs2* descriptors;
    fu_shader_buffer_usage_t usage;
    uint32_t codeSize, stride, attributeCount;
} fu_shader_t;
static_assert(sizeof(FUShader) == sizeof(fu_shader_t));

#endif