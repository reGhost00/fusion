#ifdef FU_RENDERER_TYPE_GL
#include "glad/gl.h"
// custom

#include "../shader.inner.h"
#include "core/file.h"
#include "core/memory.h"

#ifdef FU_USE_SPIRV_COMPILER
// libs
#include <glslang/Include/glslang_c_interface.h>
#include <glslang/Public/resource_limits_c.h>

typedef struct _TSpirV {
    uint32_t* code;
    int size;
} TSpirV;

// struct _FUShader {
//     FUObject parent;
//     uint32_t* vertexCode;
//     uint32_t* fragmentCode;
//     int vertexCodeSize;
//     int fragmentCodeSize;
// };

FU_DEFINE_TYPE(FUShader, fu_shader, FU_TYPE_OBJECT)

static bool t_spirv_compile(const char* source, glslang_stage_t stage, glslang_source_t language, TSpirV* res)
{
    const glslang_input_t input = {
        .language = language,
        .stage = stage,
        .client = GLSLANG_CLIENT_OPENGL,
        .client_version = GLSLANG_TARGET_OPENGL_450,
        .target_language = GLSLANG_TARGET_SPV,
        .target_language_version = GLSLANG_TARGET_SPV_1_6,
        .code = source,
        .default_version = 330,
        .default_profile = GLSLANG_CORE_PROFILE,
        .messages = GLSLANG_MSG_DEFAULT_BIT,
        .resource = glslang_default_resource()
    };
    glslang_shader_t* shader = glslang_shader_create(&input);

    if (!glslang_shader_preprocess(shader, &input)) {
        fu_warning("Shader preprocessing failed:\n\tinfo: %s\n\tdebug: %s\n\tsource: %s", glslang_shader_get_info_log(shader), glslang_shader_get_info_debug_log(shader), source);
        glslang_shader_delete(shader);
        return false;
    }

    if (!glslang_shader_parse(shader, &input)) {
        fu_warning("Shader parsing failed:\n\tinfo: %s\n\tdebug: %s\n\tsource: %s", glslang_shader_get_info_log(shader), glslang_shader_get_info_debug_log(shader), source);
        glslang_shader_delete(shader);
        return false;
    }

    glslang_program_t* program = glslang_program_create();
    glslang_program_add_shader(program, shader);

    if (!glslang_program_link(program, GLSLANG_MSG_DEFAULT_BIT)) {
        fu_warning("Shader linking failed:\n\tinfo: %s\n\tdebug: %s\n\tsource: %s", glslang_program_get_info_log(program), glslang_program_get_info_debug_log(program), source);
        glslang_program_delete(program);
        glslang_shader_delete(shader);
        return false;
    }

    glslang_program_SPIRV_generate(program, (glslang_stage_t)stage);
    res->size = glslang_program_SPIRV_get_size(program);
    res->code = (uint32_t*)fu_malloc(res->size * sizeof(uint32_t));
    glslang_program_SPIRV_get(program, res->code);
    fu_info("Compiled shader: %s", glslang_program_SPIRV_get_messages(program));

    glslang_program_delete(program);
    glslang_shader_delete(shader);
    return true;
}

static void fu_shader_finalize(FUObject* obj)
{
    FUShader* shader = (FUShader*)obj;
    fu_free(shader->vertexCode);
    fu_free(shader->fragmentCode);
}

static void fu_shader_class_init(FUObjectClass* oc)
{
    oc->finalize = fu_shader_finalize;
}

FUShader* fu_shader_new_from_source(const char* vertexSource, const char* fragmentSource, const EFUShaderSourceType type)
{
    fu_return_val_if_fail(vertexSource && fragmentSource, NULL);
    TSpirV vertexSpirv, fragmentSpirv;
    glslang_source_t lang = !type ? GLSLANG_SOURCE_HLSL : GLSLANG_SOURCE_GLSL;
    if (!t_spirv_compile(vertexSource, GLSLANG_STAGE_VERTEX, lang, &vertexSpirv)) {
        fu_error("Failed to compile vertex shader");
        return NULL;
    }
    if (!t_spirv_compile(fragmentSource, GLSLANG_STAGE_FRAGMENT, lang, &fragmentSpirv)) {
        fu_error("Failed to compile fragment shader");
        fu_free(vertexSpirv.code);
        return NULL;
    }

    FUShader* shader = (FUShader*)fu_object_new(FU_TYPE_SHADER);
    shader->vertexCode = fu_steal_pointer(&vertexSpirv.code);
    shader->fragmentCode = fu_steal_pointer(&fragmentSpirv.code);
    shader->vertexCodeSize = vertexSpirv.size;
    shader->fragmentCodeSize = fragmentSpirv.size;
    return shader;
}
#else // FU_USE_SPIRV_COMPILER
// struct _FUShader {
//     FUObject parent;
//     uint32_t program;
// };

FU_DEFINE_TYPE(FUShader, fu_shader, FU_TYPE_OBJECT)

static void fu_shader_finalize(FUObject* obj)
{
    FUShader* shader = (FUShader*)obj;
    glDeleteProgram(shader->program);
}

static void fu_shader_class_init(FUObjectClass* oc)
{
    oc->finalize = fu_shader_finalize;
}

FUShader* fu_shader_new_from_source(const char* vertexSource, const char* fragmentSource, const EFUShaderSourceType type)
{
    fu_return_val_if_fail(vertexSource && fragmentSource, NULL);
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);
    // check for shader compile errors
    int succ;
    char msg[1920];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &succ);
    if (!succ) {
        glGetShaderInfoLog(vertexShader, sizeof(msg), NULL, msg);
        fu_error("Failed to compile vertex shader: %s", msg);
        return NULL;
    }
    // fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);
    // check for shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &succ);
    if (!succ) {
        glGetShaderInfoLog(fragmentShader, sizeof(msg), NULL, msg);
        glDeleteShader(vertexShader);
        fu_error("Failed to compile fragment shader: %s", msg);
        return NULL;
    }

    // link shaders
    uint32_t program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    // check for linking errors
    glGetProgramiv(program, GL_LINK_STATUS, &succ);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    if (!succ) {
        glGetProgramInfoLog(program, sizeof(msg), NULL, msg);
        fu_error("Failed to link shader program: %s", msg);
        return NULL;
    }

    FUShader* shader = (FUShader*)fu_object_new(FU_TYPE_SHADER);
    shader->program = program;
    return shader;
}
#endif
#ifndef FU_CHECK_CALLBACK_DISABLED
static void fu_shader_unknown_uniform_name(const char* name)
{
    fu_warning("Shader has no uniform called '%s'", name);
}
#else
#define fu_shader_unknown_uniform_name(name)
#endif
void fu_shader_program_set_int(FUShader* shader, const char* name, const int val)
{
    fu_return_if_fail(shader && name);
    int location = glGetUniformLocation(shader->program, name);
    fu_return_if_true(0 > location, fu_shader_unknown_uniform_name, name);
    glUseProgram(shader->program);
    glUniform1i(location, val);
}

void fu_shader_program_set_float(FUShader* shader, const char* name, const float val)
{
    fu_return_if_fail(shader && name);
    int location = glGetUniformLocation(shader->program, name);
    fu_return_if_true(0 > location, fu_shader_unknown_uniform_name, name);
    glUseProgram(shader->program);
    glUniform1f(location, val);
}

void fu_shader_program_set_vec2(FUShader* shader, const char* name, const FUVec2* val)
{
    fu_return_if_fail(shader && name);
    int location = glGetUniformLocation(shader->program, name);
    fu_return_if_true(0 > location, fu_shader_unknown_uniform_name, name);
    glUseProgram(shader->program);
    glUniform2f(location, val->x, val->y);
}

void fu_shader_program_set_vec3(FUShader* shader, const char* name, const FUVec3* val)
{
    fu_return_if_fail(shader && name);
    int location = glGetUniformLocation(shader->program, name);
    fu_return_if_true(0 > location, fu_shader_unknown_uniform_name, name);
    glUseProgram(shader->program);
    glUniform3f(location, val->x, val->y, val->z);
}

void fu_shader_program_set_vec4(FUShader* shader, const char* name, const FUVec4* val)
{
    fu_return_if_fail(shader && name);
    int location = glGetUniformLocation(shader->program, name);
    fu_return_if_true(0 > location, fu_shader_unknown_uniform_name, name);
    glUseProgram(shader->program);
    glUniform4f(location, val->x, val->y, val->z, val->w);
}

void fu_shader_program_set_mat3(FUShader* shader, const char* name, const mat3 val)
{
    fu_return_if_fail(shader && name);
    int location = glGetUniformLocation(shader->program, name);
    fu_return_if_true(0 > location, fu_shader_unknown_uniform_name, name);
    glUseProgram(shader->program);
    glUniformMatrix3fv(location, 1, GL_TRUE, (const float*)val);
}

void fu_shader_program_set_mat4(FUShader* shader, const char* name, const mat4 val)
{
    fu_return_if_fail(shader && name);
    int location = glGetUniformLocation(shader->program, name);
    fu_return_if_true(0 > location, fu_shader_unknown_uniform_name, name);
    glUseProgram(shader->program);
    glUniformMatrix4fv(location, 1, GL_FALSE, (const float*)val);
}
/**
// void fu_shader_program_set_texture(FUShader* shader, const char* name, int index, asset_hndl t) {
//   GLint location = glGetUniformLocation(shader->program, name);
//   if ( location == -1) {
//     fu_warning("Shader has no uniform called '%s'", name);
//   } else {
//     glActiveTexture(GL_TEXTURE0 + index);
//     glBindTexture(texture_type(asset_hndl_ptr(&t)), texture_handle(asset_hndl_ptr(&t)));
//     glUniform1i(location, index);
//   }
// }

void fu_shader_program_set_texture_id(FUShader* shader, const char* name, int index, GLint t)
{

    GLint location = glGetUniformLocation(shader->program, name);
    if (location == -1) {
        fu_warning("Shader has no uniform called '%s'", name);
    } else {
        glActiveTexture(GL_TEXTURE0 + index);
        glBindTexture(GL_TEXTURE_2D, t);
        glUniform1i(location, index);
    }
}

void fu_shader_program_set_float_array(FUShader* shader, const char* name, float* vals, int count)
{
    GLint location = glGetUniformLocation(shader->program, name);
    if (location == -1) {
        fu_warning("Shader has no uniform called '%s'", name);
    } else {
        glUniform1fv(location, count, vals);
    }
}

void fu_shader_program_set_vec2_array(FUShader* shader, const char* name, FUVec2* vals, int count)
{
    GLint location = glGetUniformLocation(shader->program, name);
    if (location == -1) {
        fu_warning("Shader has no uniform called '%s'", name);
    } else {
        glUniform2fv(location, count, (float*)vals);
    }
}

void fu_shader_program_set_vec3_array(FUShader* shader, const char* name, FUVec3* vals, int count)
{
    GLint location = glGetUniformLocation(shader->program, name);
    if (location == -1) {
        fu_warning("Shader has no uniform called '%s'", name);
    } else {
        glUniform3fv(location, count, (float*)vals);
    }
}

void fu_shader_program_set_vec4_array(FUShader* shader, const char* name, vec4* vals, int count)
{
    GLint location = glGetUniformLocation(shader->program, name);
    if (location == -1) {
        fu_warning("Shader has no uniform called '%s'", name);
    } else {
        glUniform4fv(location, count, (float*)vals);
    }
}

void fu_shader_program_set_mat4_array(FUShader* shader, const char* name, mat4* vals, int count)
{
    GLint location = glGetUniformLocation(shader->program, name);
    if (location == -1) {
        fu_warning("Shader has no uniform called '%s'", name);
    } else {
        glUniformMatrix4fv(location, count, GL_TRUE, (float*)vals);
    }
}

void fu_shader_program_enable_attribute(FUShader* shader, const char* name, int count, int stride, void* ptr)
{
    GLint attr = glGetAttribLocation(shader->program, name);
    if (attr == -1) {
        fu_warning("Shader has no attribute called '%s'", name);
    } else {
        glEnableVertexAttribArray(attr);
        glVertexAttribPointer(attr, count, GL_FLOAT, GL_FALSE, sizeof(float) * stride, ptr);
    }
}

void fu_shader_program_enable_attribute_instance(FUShader* shader, const char* name, int count, int stride, void* ptr)
{
    GLint attr = glGetAttribLocation(shader->program, name);
    if (attr == -1) {
        fu_warning("Shader has no attribute called '%s'", name);
    } else {
        glEnableVertexAttribArray(attr);
        glVertexAttribPointer(attr, count, GL_FLOAT, GL_FALSE, sizeof(float) * stride, ptr);
        glVertexAttribDivisor(attr, 1);
    }
}

void fu_shader_program_enable_attribute_instance_matrix(FUShader* shader, const char* name, void* ptr)
{
    GLint attr = glGetAttribLocation(shader->program, name);
    if (attr == -1) {
        fu_warning("Shader has no attribute called '%s'", name);
    } else {
        glEnableVertexAttribArray(attr + 0);
        glEnableVertexAttribArray(attr + 1);
        glEnableVertexAttribArray(attr + 2);
        glEnableVertexAttribArray(attr + 3);
        glVertexAttribPointer(attr + 0, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4 * 4, ptr);
        glVertexAttribPointer(attr + 1, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4 * 4, ptr + sizeof(float) * 4);
        glVertexAttribPointer(attr + 2, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4 * 4, ptr + sizeof(float) * 8);
        glVertexAttribPointer(attr + 3, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4 * 4, ptr + sizeof(float) * 12);
        glVertexAttribDivisor(attr + 0, 1);
        glVertexAttribDivisor(attr + 1, 1);
        glVertexAttribDivisor(attr + 2, 1);
        glVertexAttribDivisor(attr + 3, 1);
    }
}

void fu_shader_program_disable_attribute(FUShader* shader, const char* name)
{
    GLint attr = glGetAttribLocation(shader->program, name);
    if (attr == -1) {
        fu_warning("Shader has no attribute called '%s'", name);
    } else {
        glDisableVertexAttribArray(attr);
    }
}

void fu_shader_program_disable_attribute_matrix(FUShader* shader, const char* name)
{
    GLint attr = glGetAttribLocation(shader->program, name);
    if (attr == -1) {
        fu_warning("Shader has no attribute called '%s'", name);
    } else {
        glVertexAttribDivisor(attr + 0, 0);
        glVertexAttribDivisor(attr + 1, 0);
        glVertexAttribDivisor(attr + 2, 0);
        glVertexAttribDivisor(attr + 3, 0);
        glDisableVertexAttribArray(attr + 0);
        glDisableVertexAttribArray(attr + 1);
        glDisableVertexAttribArray(attr + 2);
        glDisableVertexAttribArray(attr + 3);
    }
}
*/
#endif