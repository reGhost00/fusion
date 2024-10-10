#ifndef _FU_RENDERER_GL_RIVATE_
#define _FU_RENDERER_GL_RIVATE_
#include "../core/object.h"

FU_DECLARE_TYPE(FUGLRenderer, fu_gl_renderer)
#define FU_TYPE_GL_RENDERER (fu_gl_renderer_get_type())

FU_DECLARE_TYPE(FUGLSurface, fu_gl_surface)
#define FU_TYPE_GL_SURFACE (fu_gl_surface_get_type())

FU_DECLARE_TYPE(FUGLContext, fu_gl_context)
#define FU_TYPE_GL_CONTEXT (fu_gl_context_get_type())

#ifdef _FU_REND_GL_PRIV_IMPL
#define GLFW_INCLUDE_NONE
#include "../core/array.h"
#include "../core/utils.h"
#include "types.h"
#include <GLFW/glfw3.h>

struct _FUGLRenderer {
    FUObject parent;
    GLFWwindow* win;
    FURGBA* clearColor;
    float* vertiecs;
    uint32_t* indices;
};
FU_DEFINE_TYPE(FUGLRenderer, fu_gl_renderer, FU_TYPE_OBJECT)

struct _FUGLSurface {
    FUObject parent;
    FUGLRenderer* renderer;
    /** 填充顶点 */
    FUArray* fillVertices;
    /** 填充顶点索引 */
    FUArray* fillIndices;
    /** 描边顶点 */
    FUArray* strokeVertices;
    /** 描边顶点索引 */
    FUArray* strokeIndices;
    uint32_t shaderProgram;
    uint32_t VAO, VBO, IBO;
    int width, height;
    // float* vertiecs;
    // uint32_t* indices;
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
    FUPoint3* prevPosition;
    /** 路径 */
    FUPtrArray* path;
    /** 填充顶点 */
    FUArray* fillVertices;
    /** 填充顶点索引 */
    FUArray* fillIndices;
    /** 描边顶点 */
    FUArray* strokeVertices;
    /** 描边顶点索引 */
    FUArray* strokeIndices;
    /** 顶点颜色 */
    FURGBA* color;
    ECtxState state;
    // float* vertexBuff;
    // uint32_t* indices;
    // uint32_t vertexCount;
    // int width, height;
};
FU_DEFINE_TYPE(FUGLContext, fu_gl_context, FU_TYPE_OBJECT)

#endif
#endif