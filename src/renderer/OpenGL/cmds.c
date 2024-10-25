#ifdef FU_RENDERER_TYPE_GL
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
// libs
#include <glad/gl.h>
// custom
#include "core/array.h"
#include "core/buddy.h"
#include "core/memory.h"
#include "misc.inner.h"

#define DEF_CMDF_POOL_SIZE (65536 * sizeof(FUCommandBuffer))
static atomic_bool ifPoolInit = false;
static void* defPool = NULL;
static struct buddy* defBuddy;

static void fu_pool_check_free()
{
    if (FU_UNLIKELY(defPool))
        fu_free(defPool);
}

/**
 * @brief 新建命令缓冲，并插入到指定命令缓冲之前
 *
 * @param head 之前的命令缓冲
 * @param vertices 顶点数据 元素所有权转移
 * @param indices 索引数据  元素所有权转移
 * @param shader 着色器     所有权转移
 * @return RCommandBuffer*
 */
FUCommandBuffer* fu_command_buffer_new_prepend_take(FUCommandBuffer* head, FUArray* vertices, FUArray* indices, FUShader** shader)
{
    fu_return_val_if_fail(vertices && indices, NULL);
    if (FU_UNLIKELY(!atomic_exchange_explicit(&ifPoolInit, true, memory_order_release))) {
        defPool = fu_malloc(DEF_CMDF_POOL_SIZE);
        defBuddy = buddy_embed(defPool, DEF_CMDF_POOL_SIZE);
        atexit(fu_pool_check_free);
    }

    // FUCommandBuffer* cmdf = fu_malloc(sizeof(FUCommandBuffer));
    FUCommandBuffer* cmdf = buddy_malloc(defBuddy, sizeof(FUCommandBuffer));
    cmdf->shader = fu_steal_pointer(shader);
    cmdf->vertices = fu_array_steal(vertices, &cmdf->vertexCount);
    cmdf->indices = fu_array_steal(indices, &cmdf->indexCount);
    if (head) {
        head->prev = cmdf;
        cmdf->next = head;
    } else
        cmdf->next = NULL;
    cmdf->prev = NULL;

    glCreateVertexArrays(1, &cmdf->VAO);
    glCreateBuffers(1, &cmdf->VBO);
    glCreateBuffers(1, &cmdf->IBO);

    glNamedBufferData(cmdf->VBO, sizeof(FUVertex) * cmdf->vertexCount, cmdf->vertices, GL_STATIC_DRAW);
    glNamedBufferData(cmdf->IBO, sizeof(FUPoint3) * cmdf->indexCount, cmdf->indices, GL_STATIC_DRAW);
    // position
    glEnableVertexArrayAttrib(cmdf->VAO, 0);
    glVertexArrayAttribBinding(cmdf->VAO, 0, 0);
    glVertexArrayAttribFormat(cmdf->VAO, 0, EFU_VERTEX_POSITION_CNT, GL_FLOAT, GL_FALSE, offsetof(FUVertex, position));
    // color
    glEnableVertexArrayAttrib(cmdf->VAO, 1);
    glVertexArrayAttribBinding(cmdf->VAO, 1, 0);
    glVertexArrayAttribFormat(cmdf->VAO, 1, EFU_VERTEX_COLOR_CNT, GL_FLOAT, GL_FALSE, offsetof(FUVertex, color));
    // texture
    glEnableVertexArrayAttrib(cmdf->VAO, 2);
    glVertexArrayAttribBinding(cmdf->VAO, 2, 0);
    glVertexArrayAttribFormat(cmdf->VAO, 2, EFU_VERTEX_TEXTURE_CNT, GL_FLOAT, GL_FALSE, offsetof(FUVertex, texture));

    glVertexArrayVertexBuffer(cmdf->VAO, 0, cmdf->VBO, 0, sizeof(FUVertex));
    glVertexArrayElementBuffer(cmdf->VAO, cmdf->IBO);
    return cmdf;
}

void fu_command_buffer_free(FUCommandBuffer* cmdf)
{
    if (FU_UNLIKELY(!cmdf))
        return;
    glDeleteVertexArrays(1, &cmdf->VAO);
    glDeleteBuffers(1, &cmdf->VBO);
    glDeleteBuffers(1, &cmdf->IBO);

    fu_object_unref(cmdf->shader);
    fu_free(cmdf->vertices);
    fu_free(cmdf->indices);
    buddy_free(defBuddy, cmdf);
}

void fu_command_buffer_free_all(FUCommandBuffer* cmdf)
{
    FUCommandBuffer* curr = cmdf;
    while (curr) {
        cmdf = curr->next;
        fu_command_buffer_free(curr);
        curr = cmdf;
    }
}

void fu_command_buffer_set_color(FUCommandBuffer* cmdf, const FURGBA* color)
{
    static FUVec4 rgba;
    fu_rgba_to_float(color, &rgba.x, &rgba.y, &rgba.z, &rgba.w);
    for (uint32_t i = 0; i < cmdf->vertexCount; i++) {
        memcpy(&cmdf->vertices[i].color, &rgba, sizeof(FUVec4)); // cmdf->vertices[i].color = rgba;
    }
    glNamedBufferSubData(cmdf->VBO, 0, sizeof(FUVertex) * cmdf->vertexCount, cmdf->vertices);
}

void fu_command_buffer_update(FUCommandBuffer* cmdf)
{
    glNamedBufferSubData(cmdf->VBO, 0, sizeof(FUVertex) * cmdf->vertexCount, cmdf->vertices);
}
// void fu_command_buffer_set_transform(FUCommandBuffer* commandBuffer, RCommandFunc func)
// {
//     fu_return_val_if_fail(commandBuffer && func, NULL);
//     RCommand* cmd = fu_malloc(sizeof(RCommand));
//     cmd->func = func;
//     cmd->buff = commandBuffer;
//     return cmd;
// }

// void r_command_free(RCommand* command)
// {
//     if (FU_UNLIKELY(!command))
//         return;
//     r_command_buffer_free(command->buff);
//     fu_free(command);
// }

#endif // FU_RENDERER_TYPE_GL