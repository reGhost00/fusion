#pragma once
#ifdef FU_RENDERER_TYPE_GL
#elif defined(FU_RENDERER_TYPE_VK)
#include <vulkan/vulkan.h>

typedef struct _TSpirV {
    const uint32_t size;
    const uint32_t* code;
} FUShaderCode;
typedef VkShaderStageFlagBits FUShaderStage;

// FUShader* fu_shader_new_default(const char* name, VkShaderStageFlagBits stage);

// int fu_shader_add_data(FUShader* shader, const char* name, size_t size, void* data, FUNotify notify);

// void fu_shader_add_attributes(FUShader* shader, const uint32_t cnt, ...);
// // void fu_shader_add_binding(FUShader* shader, const uint32_t size, const VkVertexInputRate rate);
// void fu_shader_add_descriptor(FUShader* shader, const uint32_t count, const uint32_t size, void* data, FUNotify destroy);
// void fu_shader_update_sampler(FUShader* shader, VkFilter filter, VkSamplerMipmapMode mipmapMode, VkSamplerAddressMode addressMode);

#endif

typedef struct _FUShader {
    void* dummy[6];
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
FUShader* fu_shader_new(const FUShaderCode* source, FUShaderStage stage);
void fu_shader_set_attributes(FUShader* shader, const uint32_t cnt, ...);