#pragma once
#ifdef FU_RENDERER_TYPE_GL
#elif defined(FU_RENDERER_TYPE_VK)
#include <vulkan/vulkan.h>
#endif
#include "core/object.h"

#include "shader.h"

typedef struct _FUWindow FUWindow;

typedef struct _FUContextArgs {
    bool depthTest, mipmap, multiSample;
} FUContextArgs;

FU_DECLARE_TYPE(FUContext, fu_context)
#define FU_TYPE_CONTEXT (fu_context_get_type())

FUContext* fu_context_new_take(const FUWindow* win, FUShaderGroup* shaders, const FUContextArgs* args);
int fu_context_add_uniform_descriptor(FUContext* ctx, const uint32_t size, FUShaderStageFlags stage);
int fu_context_add_sampler_descriptor(FUContext* ctx, const uint32_t width, const uint32_t height, const uint8_t* data);
bool fu_context_update_input_data2(FUContext* ctx, uint64_t vertexSize, const float* vertices, uint64_t indexSize, const uint32_t* indices);
void fu_context_update_uniform_buffer(FUContext* ctx, const uint32_t index, void* data, const VkDeviceSize size);
#ifdef fesd
typedef struct _FUWindow FUWindow;

typedef enum _EFUContextType {
    EFU_CONTEXT_TYPE_GRAPHICS,
    EFU_CONTEXT_TYPE_COMPUTE,
    EFU_CONTEXT_TYPE_CNT
} EFUContextType;

FUContext* fu_context_new_default(FUWindow* win);
bool fu_context_take_shader(FUContext* ctx, FUShaderGroup* shaders);
bool fu_context_update_vertex(FUContext* ctx, const void* vertex, VkDeviceSize size, uint32_t count);
bool fu_context_update_indices(FUContext* ctx, const uint32_t* indices, uint32_t count);
/**
\brief Rasterizer state descriptor structure.
\see GraphicsPipelineDescriptor::rasterizer
*/

typedef struct _FUPipelineAssemblyArgs {
    VkPrimitiveTopology topology;
    VkBool32 ifPrimitiveRestart;
} FUPipelineAssemblyArgs;

typedef struct _FUPipelineRasterizerArgs {
    //! Polygon render mode. By default PolygonMode::Fill.
    VkPolygonMode polygonMode;
    bool ifCullBack;
    // //! Polygon face culling mode. By default CullMode::Disabled.
    // EFUCullMode cullMode;

    // //! Specifies the parameters to bias fragment depth values.
    // DepthBiasDescriptor depthBias;

    // //! If enabled, front facing polygons are in counter-clock-wise winding, otherwise in clock-wise winding. By default disabled.
    // bool                frontCCW                    = false;

    /**
    \brief If enabled, primitives are discarded after optional stream-outputs but before the rasterization stage. By default disabled.
    \note Only supported with: OpenGL, Vulkan, Metal.
    */
    VkBool32 ifDiscard;

    //! If enabled, there is effectively no near and far clipping plane. By default disabled.
    VkBool32 ifDepthClamp;

    /**
    \brief Specifies whether scissor test is enabled or disabled. By default disabled.
    \see CommandBuffer::SetScissor
    \see CommandBuffer::SetScissors
    */
    VkBool32 ifScissorTest;

    //! Specifies whether multi-sampling is enabled or disabled. By default disabled.
    VkBool32 ifMultiSample;

    //! Specifies whether lines are rendered with or without anti-aliasing. By default disabled.
    VkBool32 ifAntiAliasedLine;

    /**
    \brief If true, conservative rasterization is enabled. By default disabled.
    \note Only supported with:
    - Direct3D 12
    - Direct3D 11.3
    - OpenGL (if the extension \c GL_NV_conservative_raster or \c GL_INTEL_conservative_rasterization is supported)
    - Vulkan (if the extension \c VK_EXT_conservative_rasterization is supported).
    \see https://www.opengl.org/registry/specs/NV/conservative_raster.txt
    \see https://www.opengl.org/registry/specs/INTEL/conservative_rasterization.txt
    \see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VK_EXT_conservative_rasterization.html
    \see RenderingFeatures::hasConservativeRasterization
    */
    VkBool32 ifConservativeRasterization;

    /**
    \brief Specifies the width of all generated line primitives. By default 1.0.
    \remarks The minimum and maximum supported line width can be determined by the \c lineWidthRange member in the RenderingCapabilities structure.
    If this line width is out of range, it will be clamped silently during graphics pipeline creation.
    \note Only supported with: OpenGL, Vulkan.
    \see RenderingLimits::lineWidthRange
    */
    float lineWidth;
} FUPipelineRasterizerArgs;

typedef struct _FUPipelineGraphicsArgs {
    // FUShaderGroup* shaders;
    FUPipelineAssemblyArgs assembly;
    FUPipelineRasterizerArgs rasterizer;
    // EFUPipelineIndexFormat indexFormat;
} FUPipelineGraphicsArgs;

typedef struct _FUPipelineComputeArgs {

} FUPipelineComputeArgs;

typedef struct _FUPipelineArgs {
    EFUPipelineType type;
    union fu_pipeline_args_u {
        FUPipelineGraphicsArgs graphics;
        FUPipelineComputeArgs compute;
    } args;
    // FUPipelineShaders shaders;
    // uint32_t* uniformBufferSize;
    // uint32_t uniformBufferCount;
} FUPipelineArgs;

FUContext* fu_context_new(const FUPipelineArgs* args);
bool fu_pipeline_set_shader(FUPipeline* pipeline, FUShaderGroup* shaders);
bool fu_pipeline_add_uniform_buffer(FUPipeline* pipeline, VkDeviceSize size, uint32_t cnt, void* data, FUNotify destroy);
// bool fu_pipeline_new_to_vulkan(FUPipeline* pipeline, TSurface* surface);
#endif