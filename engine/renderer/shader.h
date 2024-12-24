#pragma once
#ifdef FU_RENDERER_TYPE_GL
#elif defined(FU_RENDERER_TYPE_VK)
#include <vulkan/vulkan.h>

typedef struct _TSpirV {
    const uint32_t size;
    const uint32_t* code;
} FUShaderCode;
// typedef VkShaderStageFlagBits FUShaderStage;

typedef enum _FUShaderStageFlags {
    FU_SHADER_STAGE_FLAG_VERTEX = VK_SHADER_STAGE_VERTEX_BIT,
    FU_SHADER_STAGE_FLAG_TESSELLATION_CONTROL = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
    FU_SHADER_STAGE_FLAG_TESSELLATION_EVALUATION = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
    FU_SHADER_STAGE_FLAG_GEOMETRY = VK_SHADER_STAGE_GEOMETRY_BIT,
    FU_SHADER_STAGE_FLAG_FRAGMENT = VK_SHADER_STAGE_FRAGMENT_BIT,
    FU_SHADER_STAGE_FLAG_COMPUTE = VK_SHADER_STAGE_COMPUTE_BIT
} FUShaderStageFlags;
// FUShader* fu_shader_new_default(const char* name, VkShaderStageFlagBits stage);

// int fu_shader_add_data(FUShader* shader, const char* name, size_t size, void* data, FUNotify notify);

// void fu_shader_add_attributes(FUShader* shader, const uint32_t cnt, ...);
// // void fu_shader_add_binding(FUShader* shader, const uint32_t size, const VkVertexInputRate rate);
// void fu_shader_add_descriptor(FUShader* shader, const uint32_t count, const uint32_t size, void* data, FUNotify destroy);
// void fu_shader_update_sampler(FUShader* shader, VkFilter filter, VkSamplerMipmapMode mipmapMode, VkSamplerAddressMode addressMode);

#endif

typedef struct _FUShader {
    void* dummy[5];
} FUShader;

typedef struct _FUShaderGroup {
    FUShader* vertex;
    /**
    \brief Specifies the tessellation-control shader (also referred to as "Hull Shader").
    \remarks If this is used, the counter part must also be specified, i.e. \c tessEvaluationShader.
    \see tessEvaluationShader
    */
    FUShader* tessControl;

    /**
    \brief Specifies the tessellation-evaluation shader (also referred to as "Domain Shader").
    \remarks If this is used, the counter part must also be specified, i.e. \c tessControlShader.
    \see tessControlShader
    */
    FUShader* tessEvaluation;
    FUShader* geometry;
    FUShader* fragment;
    FUShader* compute;
} FUShaderGroup;

void fu_shader_free(FUShader* shader);
FUShader* fu_shader_new(const FUShaderCode* source, FUShaderStageFlags stage);
void fu_shader_set_attributes(FUShader* shader, const uint32_t cnt, ...);