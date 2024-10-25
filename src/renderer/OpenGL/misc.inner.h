#ifdef FU_RENDERER_TYPE_GL
#pragma once
// #include "../../core/array.h"
// #include "../../core/memory.h"
// #include "../../core/object.h"
// #include "../backend2.h"
#include "../misc.h"
#include "../shader.h"

typedef struct _FUCommandBuffer {
    FUCommandBuffer* prev;
    FUCommandBuffer* next;
    FUShader* shader;
    FUVertex* vertices;
    FUPoint3* indices;
    uint32_t vertexCount, indexCount;
    // OpenGL specific
    uint32_t VAO, VBO, IBO;
} FUCommandBuffer;

// struct _TSurface {
//     FUObject parent;
//     // FUGLRenderer* renderer;
//     // /** 填充顶点 */
//     // FUArray* fillVertices;
//     // /** 填充顶点索引 */
//     // FUArray* fillIndices;
//     // /** 描边顶点 */
//     // FUArray* strokeVertices;
//     // /** 描边顶点索引 */
//     // FUArray* strokeIndices;
//     // /** 顶点 */
//     // FUArray* vertices;
//     // /** 顶点索引 */
//     // FUArray* indices;
//     // * window;
//     FUWindow* win;
//     FUCommandBuffer* cmds;
//     FUVec3* color;
//     FUSize size;
//     // // TShader* shader;
//     // uint32_t VAO, VBO, IBO;
//     // int width, height;
// };

// FU_DECLARE_TYPE(TSurface, t_surface)
// #define FU_TYPE_SURFACE (fu_surface_get_type())

//     fu_object_unref(sf->window);
// }

// static void r_surface_class_init(FUObjectClass* oc)
// {
//     oc->dispose = (FUObjectFunc)r_surface_dispose;
//     oc->finalize = (FUObjectFunc)r_surface_finalize;
// }

#endif // FU_RENDERER_TYPE_GL