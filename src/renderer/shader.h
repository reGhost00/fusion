#pragma once
#include <cglm/cglm.h>
// custom
#include "core/object.h"
#include "misc.h"

typedef enum _EFUShaderSourceType {
    EFU_SHADER_SOURCE_TYPE_GLSL,
    EFU_SHADER_SOURCE_TYPE_HLSL,
    EFU_SHADER_SOURCE_TYPE_CNT
} EFUShaderSourceType;

FU_DECLARE_TYPE(FUShader, fu_shader)
#define FU_TYPE_SHADER (fu_shader_get_type())

FUShader* fu_shader_new_from_source(const char* vertexSource, const char* fragmentSource, const EFUShaderSourceType type);

void fu_shader_program_set_int(FUShader* shader, const char* name, const int val);
void fu_shader_program_set_float(FUShader* shader, const char* name, const float val);
void fu_shader_program_set_vec2(FUShader* shader, const char* name, const FUVec2* val);
void fu_shader_program_set_vec3(FUShader* shader, const char* name, const FUVec3* val);
void fu_shader_program_set_vec4(FUShader* shader, const char* name, const FUVec4* val);
void fu_shader_program_set_mat3(FUShader* shader, const char* name, const mat3 val);
void fu_shader_program_set_mat4(FUShader* shader, const char* name, const mat4 val);
// void fu_shader_program_set_texture_id(FUShader* shader, const char* name, int index, GLint t);
// void fu_shader_program_set_vec2_array(FUShader* shader, const char* name, const FUVec2* vals, int count);
// void fu_shader_program_set_vec3_array(FUShader* shader, const char* name, const FUVec3* vals, int count);
// void fu_shader_program_set_vec4_array(FUShader* shader, const char* name, const FUVec4* vals, int count);
// void fu_shader_program_set_float_array(FUShader* shader, const char* name, const float* vals, int count);