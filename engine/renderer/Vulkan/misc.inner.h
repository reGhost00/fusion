#ifdef FU_RENDERER_TYPE_VK
#include "../backend.h"
#include "../context.h"
#include "../misc.h"
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