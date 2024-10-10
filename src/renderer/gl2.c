#ifdef FU_RENDERER_TYPE_GL
#define GLAD_GL_IMPLEMENTATION // Necessary for headeronly version.
#define GLFW_INCLUDE_NONE // 避免重复声明 OpenGL
#include <GLFW/glfw3.h>
#include <glad/gl.h>
#include <stddef.h>

#define _FU_PLATFORM_GLFW_ENABLE_
#define _FU_RENDERER_GL_ENABLE_
#include "../_inner.h"
#include "../core/array.h"
#include "../core/utils.h"
// #include "_gl.h"
#include "gl2.h"

#define DEF_VERTICES_ARR_LEN 99
#define DEF_MSG_BUFF_LEN 1600
#define DEF_VERTEX_LEN 8
#define DEF_INDEX_LEN 3

typedef struct _TVertex {
    float position[4];
    float color[4];
    float textureCoord[2];
} TVertex;

typedef struct _TShader {
    uint32_t id;
    uint32_t idx;
} TShader;

typedef enum _EVertexRaw {
    E_VERTEX_RAW_POSITION_X,
    E_VERTEX_RAW_POSITION_Y,
    E_VERTEX_RAW_POSITION_Z,
    E_VERTEX_RAW_POSITION_W,
    E_VERTEX_RAW_COLOR_R,
    E_VERTEX_RAW_COLOR_G,
    E_VERTEX_RAW_COLOR_B,
    E_VERTEX_RAW_COLOR_A,
    E_VERTEX_RAW_TEXTURE_X,
    E_VERTEX_RAW_TEXTURE_Y,
    E_VERTEX_RAW_CNT
} EVertexRaw;

typedef enum _EVertexPosition {
    E_VERTEX_POSITION_X,
    E_VERTEX_POSITION_Y,
    E_VERTEX_POSITION_Z,
    E_VERTEX_POSITION_W,
    E_VERTEX_POSITION_CNT
} EVertexPosition;

typedef enum _EVertexColor {
    E_VERTEX_COLOR_R,
    E_VERTEX_COLOR_G,
    E_VERTEX_COLOR_B,
    E_VERTEX_COLOR_A,
    E_VERTEX_COLOR_CNT
} EVertexColor;

typedef enum _EVertexTexture {
    E_VERTEX_TEXTURE_X,
    E_VERTEX_TEXTURE_Y,
    E_VERTEX_TEXTURE_CNT
} EVertexTexture;

// struct _FUGLRenderer {
//     FUObject parent;
//     GLFWwindow* win;
//     FURGBA* clearColor;
//     TVertex* fillVertiecs;
//     TVertex* strokeVertiecs;
//     uint32_t* fillIndices;
//     uint32_t* strokeIndices;
// };
// FU_DEFINE_TYPE(FUGLRenderer, fu_gl_renderer, FU_TYPE_OBJECT)

struct _FUGLSurface {
    FUObject parent;
    // FUGLRenderer* renderer;
    // /** 填充顶点 */
    // FUArray* fillVertices;
    // /** 填充顶点索引 */
    // FUArray* fillIndices;
    // /** 描边顶点 */
    // FUArray* strokeVertices;
    // /** 描边顶点索引 */
    // FUArray* strokeIndices;

    /** 顶点 */
    FUArray* vertices;
    /** 顶点索引 */
    FUArray* indices;
    FUWindow* window;
    FUVec4* clearColor;
    TShader* shader;
    uint32_t VAO, VBO, IBO;
    int width, height;
};
FU_DEFINE_TYPE(FUGLSurface, fu_gl_surface, FU_TYPE_OBJECT)

typedef enum _ECtxState {
    E_CTX_STATE_NONE,
    E_CTX_STATE_READY,
    /** 正在记录 2D 路径 */
    E_CTX_STATE_REC2,
    /** 正在记录 3D 路径 */
    E_CTX_STATE_REC3,
    E_CTX_STATE_CNT
} ECtxState;

struct _FUGLContext {
    FUObject parent;
    FUGLSurface* surface;
    FUPoint3* position;
    /** 路径 */
    FUPtrArray* path;
    /** 顶点 */
    FUArray* vertices;
    /** 顶点索引 */
    FUArray* indices;
    /** 顶点颜色 */
    FURGBA* color;
    ECtxState state;
    // float* vertexBuff;
    // uint32_t* indices;
    // uint32_t vertexCount;
    // int width, height;
};
FU_DEFINE_TYPE(FUGLContext, fu_gl_context, FU_TYPE_OBJECT)

typedef enum _EDefShader {
    E_DEF_SHADER_NORMAL,
    E_DEF_SHADER_CNT
} EDefShader;

static void fu_print_vertices(TVertex* buff, uint32_t col, uint32_t len)
{
    char str[len * 33];
    uint32_t i = 0;
    sprintf(str, "Float buff:\n");
    for (i = 0; i < col; i++) {
        sprintf(str, "%s%-7u", str, i);
    }
    sprintf(str, "%s\n", str);
    for (i = 0; i < len; i++) {
        if (!(i % col))
            sprintf(str, "%s\n", str);
        sprintf(str, "%s%-7.3f", str, buff[i]);
    }

    printf("%s\n", str);
    fflush(stdout);
}

static void fu_print_buff_uint(uint32_t* buff, uint32_t col, uint32_t len)
{
    char str[len * 33];
    uint32_t i = 0;
    sprintf(str, "Unsigned buff:\n");
    for (i = 0; i < len; i++) {
        sprintf(str, "%s%-7u", str, i);
    }

    sprintf(str, "%s\n", str);
    for (i = 0; i < len; i++) {
        if (!(i % col))
            sprintf(str, "%s\n", str);
        sprintf(str, "%s%-7u", str, buff[i]);
    }

    printf("%s\n", str);
    fflush(stdout);
}

/**
 * @brief 创建并编译着色器
 *
 * @param idx 着色器源码索引
 * @param shader 输出
 * @return true
 * @return false
 */
static TShader* t_shaders_new(const uint32_t idx)
{
    static const char* const defVertexShaders[] = {
        "#version 330 core\n"
        "#extension GL_ARB_bindless_texture : require\n"
        "layout (location = 0) in vec4 position;\n"
        "layout (location = 1) in vec4 color;\n"
        "layout (location = 2) in vec2 textureCoord;\n"
        "out vec4 rgb;\n"
        "void main()\n"
        "{\n"
        "   rgb = color;\n"
        "   gl_Position = position;\n"
        "}\0"
    };

    static const char* const defFragmentShaders[] = {
        "#version 330 core\n"
        "in vec4 rgb;\n"
        "out vec4 color;\n"
        "void main()\n"
        "{\n"
        "   color = rgb;\n"
        "}\0"
    };
    fu_return_val_if_fail(idx < E_DEF_SHADER_CNT, NULL);
    uint32_t vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &defVertexShaders[idx], NULL);
    glCompileShader(vertex);
#ifndef FU_NO_DEBUG
    int res;
    char* msg;
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &res);
    if (!res) {
        glGetShaderiv(vertex, GL_INFO_LOG_LENGTH, &res);
        msg = fu_malloc0(res);
        glGetShaderInfoLog(vertex, res, NULL, msg);
        glDeleteShader(vertex);
        fprintf(stderr, "Failed to compile vertex shader: %s\n", msg);
        fu_free(msg);
        return NULL;
    }
#endif
    uint32_t fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &defFragmentShaders[idx], NULL);
    glCompileShader(fragment);
#ifndef FU_NO_DEBUG
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &res);
    if (!res) {
        glGetShaderiv(fragment, GL_INFO_LOG_LENGTH, &res);
        msg = fu_malloc0(res);
        glGetShaderInfoLog(fragment, res, NULL, msg);
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        fprintf(stderr, "Failed to compile fragment shader: %s\n", msg);
        fu_free(msg);
        return NULL;
    }
#endif

    TShader* shader = fu_malloc0(sizeof(TShader));
    shader->idx = idx;
    shader->id = glCreateProgram();
    glAttachShader(shader->id, vertex);
    glAttachShader(shader->id, fragment);
    glLinkProgram(shader->id);
    glValidateProgram(shader->id);

    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return shader;
}

static void t_shader_free(TShader* shader)
{
    // if (!shader) return;
    glDeleteProgram(shader->id);
    fu_free(shader);
}

//
// Surface
static bool ifSurfaceInit = false;
static void fu_gl_surface_resize_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

static void fu_gl_surface_finalize(FUGLSurface* surface)
{
    glDeleteVertexArrays(1, &surface->VAO);
    glDeleteBuffers(1, &surface->VBO);
    glDeleteBuffers(1, &surface->IBO);
    t_shader_free(surface->shader);
    fu_array_unref(surface->vertices);
    fu_array_unref(surface->indices);
    fu_object_unref(surface->window);
    fu_free(surface->clearColor);
}

static void fu_gl_surface_class_init(FUObjectClass* oc)
{
    oc->finalize = (FUObjectFunc)fu_gl_surface_finalize;
}

FUGLSurface* fu_gl_surface_new(FUWindow* window)
{
    fu_return_val_if_fail(window, NULL);
    // int succ;
    // char buff[DEF_MSG_BUFF_LEN];
    // uint32_t vertexShader = glCreateShader(GL_VERTEX_SHADER);
    // fu_retrun_if_shader_compile_failed(vertexShader, defVertexShaders[E_DEF_SHADER_NORMAL], succ, buff);

    // uint32_t fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    // fu_retrun_if_shader_compile_failed(fragmentShader, defFragmentShaders[E_DEF_SHADER_NORMAL], succ, buff);

    // uint32_t shaderProgram = glCreateProgram();
    // fu_return_if_program_link_failed(shaderProgram, vertexShader, fragmentShader, succ, buff);
    TShader* shader = t_shaders_new(E_DEF_SHADER_NORMAL);
    if (!shader)
        return NULL;
    FUGLSurface* surface = (FUGLSurface*)fu_object_new(FU_TYPE_GL_SURFACE);
    surface->vertices = fu_array_new_full(sizeof(TVertex), DEF_VERTICES_ARR_LEN);
    surface->indices = fu_array_new_full(sizeof(FUPoint3), DEF_VERTICES_ARR_LEN);
    surface->window = fu_object_ref(window);
    surface->clearColor = fu_malloc(sizeof(FUVec4));
    surface->shader = fu_steal_pointer(&shader);
    glfwGetFramebufferSize(window->window, &surface->width, &surface->height);

    glGenVertexArrays(1, &surface->VAO);
    glGenBuffers(1, &surface->VBO);
    glGenBuffers(1, &surface->IBO);

    glBindVertexArray(surface->VAO);
    glBindBuffer(GL_ARRAY_BUFFER, surface->VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, surface->IBO);

    if (!ifSurfaceInit) {
        ifSurfaceInit = true;
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glViewport(0, 0, surface->width, surface->height);
        glfwSetFramebufferSizeCallback(window->window, fu_gl_surface_resize_callback);
    }

    // glDeleteShader(vertexShader);
    // glDeleteShader(fragmentShader);
    return surface;
}

void fu_gl_surface_draw(FUGLSurface* surface)
{
    fu_return_if_fail(surface);
    uint32_t vertexLen, indexLen;
    TVertex* vertices = fu_array_steal(surface->vertices, &vertexLen);
    FUPoint3* indices = fu_array_steal(surface->indices, &indexLen);

    if (indices && vertices) {

        // fu_print_buff_float((float*)vertices, 10, vertexLen * 10);
        // printf("vertexLen: %d, indexLen: %d\n", vertexLen, indexLen);

        glClearColor(surface->clearColor->x, surface->clearColor->y, surface->clearColor->z, surface->clearColor->w);
        glClear(GL_COLOR_BUFFER_BIT);

        glBufferData(GL_ARRAY_BUFFER, vertexLen * sizeof(TVertex), vertices, GL_DYNAMIC_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexLen * sizeof(FUPoint3), indices, GL_DYNAMIC_DRAW);
        // 位置
        glVertexAttribPointer(0, E_VERTEX_POSITION_CNT, GL_FLOAT, GL_FALSE, sizeof(TVertex), NULL);
        glEnableVertexAttribArray(0);
        // fu_gl_check_error();
        // 颜色
        glVertexAttribPointer(1, E_VERTEX_COLOR_CNT, GL_FLOAT, GL_FALSE, sizeof(TVertex), (void*)offsetof(TVertex, color));
        glEnableVertexAttribArray(1);
        // fu_gl_check_error();
        // 纹理
        glVertexAttribPointer(2, E_VERTEX_TEXTURE_CNT, GL_FLOAT, GL_FALSE, sizeof(TVertex), (void*)offsetof(TVertex, textureCoord));
        glEnableVertexAttribArray(2);
        // fu_gl_check_error();

        // glBindVertexArray(0);
        glUseProgram(surface->shader->id);
        glBindVertexArray(surface->VAO);
        glDrawElements(GL_TRIANGLES, vertexLen * indexLen, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        glfwSwapBuffers(surface->window->window);
    }
    // fu_print_buff_float(vertices, 8, vertexCount);
    // fu_print_buff_uint(indices, 3, indiexCount);
    fu_free(indices);
    fu_free(vertices);
}
//
// Context

static void fu_gl_context_finalize(FUGLContext* context)
{
    fu_array_unref(context->vertices);
    fu_array_unref(context->indices);
    fu_ptr_array_unref(context->path);
    fu_object_unref(context->surface);
    fu_free(context->position);
    fu_free(context->color);
}

static void fu_gl_context_class_init(FUObjectClass* oc)
{
    oc->finalize = (FUObjectFunc)fu_gl_context_finalize;
}

FUGLContext* fu_gl_context_new(FUGLSurface* surface)
{
    fu_return_val_if_fail(surface, NULL);
    FUGLContext* context = (FUGLContext*)fu_object_new(FU_TYPE_GL_CONTEXT);
    context->surface = fu_object_ref(surface);
    context->vertices = fu_array_new_full(sizeof(TVertex), DEF_VERTICES_ARR_LEN);
    context->indices = fu_array_new_full(sizeof(FUPoint3), DEF_VERTICES_ARR_LEN);
    context->path = fu_ptr_array_new_full(DEF_VERTICES_ARR_LEN, fu_free);
    context->position = fu_malloc(sizeof(FUPoint3));
    context->color = fu_malloc(sizeof(FURGBA));

    return context;
}
/**
 * @brief 开始记录 2d 路径
 * 绘制完成后调用 fu_gl_context_path2_fill 或 fu_gl_context_path2_stroke
 * @param context
 */
void fu_gl_context_path2_start(FUGLContext* context)
{
    fu_return_if_fail(context);
    if (FU_UNLIKELY(context->state)) {
        fu_array_remove_all(context->vertices);
        fu_array_remove_all(context->indices);
        fu_ptr_array_remove_all(context->path, fu_free);
    }
    memset(context->position, 0, sizeof(FUPoint3));
    context->state = E_CTX_STATE_READY;
}

void fu_gl_context_moveto2(FUGLContext* context, uint32_t x, uint32_t y)
{
    fu_return_if_fail(context);
    if (FU_UNLIKELY(!context->state))
        return;
    context->position->x = x;
    context->position->y = y;
    // context->prevPosition->z = 0;
}

void fu_gl_context_lineto2(FUGLContext* context, uint32_t x, uint32_t y)
{
    fu_return_if_fail(context);
    if (FU_UNLIKELY(!context->state))
        return;
    FUPtrArray* vertiecs = context->path;
    FUPoint2* curr;
    if (E_CTX_STATE_REC2 > context->state)
        fu_ptr_array_push(vertiecs, fu_memdup(context->position, sizeof(FUPoint2)));
    fu_ptr_array_push(vertiecs, curr = fu_point2_new(x, y));
    memcpy(context->position, curr, sizeof(FUPoint2));
    context->state = E_CTX_STATE_REC2;
}

void fu_gl_context_path2_fill(FUGLContext* context, FURGBA* color)
{
    static TVertex vertex;
    static FUPoint3 index;
    fu_return_if_fail(context);
    if (!context->state)
        return;
    const uint32_t width = context->surface->width;
    const uint32_t height = context->surface->height;
    const uint32_t len = context->path->len;
    const uint32_t prevVerticesLen = context->vertices->len;
    FUPoint2** pos = (FUPoint2**)fu_ptr_array_steal_all(context->path);
    // TVertex* vertex = NULL;
    if (color)
        memcpy(context->color, color, sizeof(FURGBA));
    memset(&vertex, 0, sizeof(TVertex));
    for (uint32_t i = 0; i < len; i++) {
        fu_vec2_ndc_init_from_screen(width, height, pos[i]->x, pos[i]->y, &vertex.position[E_VERTEX_POSITION_X], &vertex.position[E_VERTEX_POSITION_Y]);
        fu_rgba_to_float(context->color, &vertex.color[E_VERTEX_COLOR_R], &vertex.color[E_VERTEX_COLOR_G], &vertex.color[E_VERTEX_COLOR_B], &vertex.color[E_VERTEX_COLOR_A]);
        vertex.position[E_VERTEX_POSITION_W] = 1.0;
        fu_array_append_val(context->vertices, vertex);
        fu_free(pos[i]);
    }

    for (uint32_t i = prevVerticesLen; i < prevVerticesLen + len - 2; i++) {
        index.x = prevVerticesLen;
        index.y = i + 1;
        index.z = i + 2;
        fu_array_append_val(context->indices, index);
    }
    fu_free(pos);
    context->path = fu_ptr_array_new_full(DEF_VERTICES_ARR_LEN, fu_free);
    context->state = E_CTX_STATE_NONE;
}

void fu_gl_context_submit(FUGLContext* context)
{
    fu_return_if_fail(context);
    uint32_t vertexCount, indexCount;
    TVertex* vertices = fu_array_steal(context->vertices, &vertexCount);
    FUPoint3* indices = fu_array_steal(context->indices, &indexCount);

    if (vertices && indices) {
        fu_array_append_vals(context->surface->vertices, vertices, vertexCount);
        fu_array_append_vals(context->surface->indices, indices, indexCount);
        fu_free(vertices);
        fu_free(indices);
    }

    // vertices = fu_array_steal(context->strokeVertices, &vertexCount);
    // indices = fu_array_steal(context->strokeIndices, &indexCount);
    // if (vertices && indices) {
    //     fu_array_append_vals(context->surface->strokeVertices, vertices, vertexCount);
    //     fu_array_append_vals(context->surface->strokeIndices, indices, indexCount);
    //     fu_free(vertices);
    //     fu_free(indices);
    // }
}

#endif