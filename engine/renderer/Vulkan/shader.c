#ifdef FU_RENDERER_TYPE_VK
#include "core/logger.h"
#include "core/memory.h"

#include "misc.inner.h"

void fu_shader_free(FUShader* shader)
{
    if (!shader)
        return;
    fu_shader_t* sd = (fu_shader_t*)shader;
    fu_free(sd->attributes);
    fu_free(sd->code);
    fu_free(sd);
}

FUShader* fu_shader_new(const FUShaderCode* source, FUShaderStageFlags stage)
{
    fu_return_val_if_fail(source && source->code && source->size, NULL);
    fu_shader_t* sd = fu_malloc0(sizeof(fu_shader_t));
    sd->code = fu_memdup(source->code, source->size);
    sd->codeSize = source->size;
    sd->stage = (VkShaderStageFlagBits)stage;
    return (FUShader*)sd;
}

void fu_shader_set_attributes(FUShader* shader, const uint32_t cnt, ...)
{
    const uint32_t defAttrSizes[] = {
        [VK_FORMAT_R32_SINT] = sizeof(int32_t),
        [VK_FORMAT_R32G32_SINT] = sizeof(int32_t[2]),
        [VK_FORMAT_R32G32B32_SINT] = sizeof(int32_t[3]),
        [VK_FORMAT_R32G32B32A32_SINT] = sizeof(int32_t[4]),
        [VK_FORMAT_R32_UINT] = sizeof(uint32_t),
        [VK_FORMAT_R32G32_UINT] = sizeof(uint32_t[2]),
        [VK_FORMAT_R32G32B32_UINT] = sizeof(uint32_t[3]),
        [VK_FORMAT_R32G32B32A32_UINT] = sizeof(uint32_t[4]),
        [VK_FORMAT_R64_SINT] = sizeof(int64_t),
        [VK_FORMAT_R64G64_SINT] = sizeof(int64_t[2]),
        [VK_FORMAT_R64G64B64_SINT] = sizeof(int64_t[3]),
        [VK_FORMAT_R64G64B64A64_SINT] = sizeof(int64_t[4]),
        [VK_FORMAT_R64_UINT] = sizeof(uint64_t),
        [VK_FORMAT_R64G64_UINT] = sizeof(uint64_t[2]),
        [VK_FORMAT_R64G64B64_UINT] = sizeof(uint64_t[3]),
        [VK_FORMAT_R64G64B64A64_UINT] = sizeof(uint64_t[4]),
        [VK_FORMAT_R32_SFLOAT] = sizeof(float),
        [VK_FORMAT_R32G32_SFLOAT] = sizeof(float[2]),
        [VK_FORMAT_R32G32B32_SFLOAT] = sizeof(float[3]),
        [VK_FORMAT_R32G32B32A32_SFLOAT] = sizeof(float[4]),
        [VK_FORMAT_R64_SFLOAT] = sizeof(double),
        [VK_FORMAT_R64G64_SFLOAT] = sizeof(double[2]),
        [VK_FORMAT_R64G64B64_SFLOAT] = sizeof(double[3]),
        [VK_FORMAT_R64G64B64A64_SFLOAT] = sizeof(double[4])
    };
    fu_return_if_fail(shader && cnt);
    fu_shader_t* sd = (fu_shader_t*)shader;
    fu_return_if_true(VK_SHADER_STAGE_VERTEX_BIT != sd->stage);

    va_list ap;
    va_start(ap, cnt);

    sd->attributeCount = cnt;
    sd->attributes = fu_malloc0(sizeof(VkVertexInputAttributeDescription) * cnt);
    for (uint32_t i = 0; i < cnt; i++) {
        sd->attributes[i].location = i;
        sd->attributes[i].format = va_arg(ap, VkFormat);
        sd->attributes[i].offset = sd->stride;
        sd->stride += defAttrSizes[sd->attributes[i].format];
    }
    va_end(ap);
}

// /**
//  * @brief 添加统一描述符
//  */
// int fu_shader_add_uniform_descriptor(FUShader* shader, const uint32_t size, const uint32_t count, VkBufferUsageFlags usage)
// {
//     fu_return_val_if_fail(shader && count, -1);
//     fu_shader_t* sd = (fu_shader_t*)shader;
//     sd->descriptors = t_descriptor_args_prepare(sd->descriptors, count, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, sd->stage);
//     sd->usage.buffer = usage;
//     return sd->descriptors->binding.binding;
// }

// int fu_shader_add_sampler_descriptor(FUShader* shader, const uint32_t size, const uint32_t count, VkImageUsageFlags usage)
// {
//     fu_return_val_if_fail(shader && count, -1);
//     fu_shader_t* sd = (fu_shader_t*)shader;
//     sd->descriptors = t_descriptor_args_prepare(sd->descriptors, count, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, sd->stage);
//     sd->usage.image = usage;
//     return sd->descriptors->binding.binding;
// }

#endif