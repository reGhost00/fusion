#include <math.h>
#include <string.h>
// libs
#include <cglm/cglm.h>
#include <glad/gl.h>
// custom
#include "backend2.h"
#include "context2.h"
#include "core/array.h"
#include "core/buddy.h"
#include "core/memory.h"
#include "misc.inner.h"
#include "platform/glfw.inner.h"
#include "platform/sdl.inner.h"
#include "shader.h"

typedef struct _TPos2Node TPos2Node;
struct _TPos2Node {
    TPos2Node* next;
    TPos2Node* prev;
    FUVec4 position;
};

typedef enum _ECtx2State {
    E_CTX2_STATE_NONE,
    E_CTX2_STATE_PATH_START,
    E_CTX2_STATE_PATH_REC,
    E_CTX2_STATE_TO_COMMAND,
    E_CTX2_STATE_CNT
} ECtx2State;

// 前面的参数需要和 FUContext 对齐
struct _FUContext2 {
    FUObject parent;
    FUCommandBuffer* commands;
    FUSize size;
    uint32_t idx;
    FUArray* vertices;
    FUArray* indices;
    // ctx2 specific
    FUPoint2 prevPos;
    FUMatrix2 mat;
    TPos2Node* paths;
    ECtx2State state;
};

// FU_DEFINE_TYPE(FUContext2, fu_context2, FU_TYPE_OBJECT)
#define DEF_POS2NODE_SIZE (65536 * sizeof(TPos2Node))
#define DEF_VERTEX_LEN 9
static void* defPos2NodePool;
static struct buddy* defPos2NodeBuddy;

/**
 * SECTION:cairo-matrix
 * @Title: FUMatrix2
 * @Short_Description: Generic matrix operations
 * @See_Also: #cairo_t
 *
 * #FUMatrix2 is used throughout cairo to convert between different
 * coordinate spaces.  A #FUMatrix2 holds an affine transformation,
 * such as a scale, rotation, shear, or a combination of these.
 * The transformation of a point (<literal>x</literal>,<literal>y</literal>)
 * is given by:
 *
 * <programlisting>
 * x_new = xx * x + xy * y + x0;
 * y_new = yx * x + yy * y + y0;
 * </programlisting>
 *
 * The current transformation matrix of a #cairo_t, represented as a
 * #FUMatrix2, defines the transformation from user-space
 * coordinates to device-space coordinates. See cairo_get_matrix() and
 * cairo_set_matrix().
 **/

/**
 * cairo_matrix_init:
 * @matrix: a #FUMatrix2
 * @xx: xx component of the affine transformation
 * @yx: yx component of the affine transformation
 * @xy: xy component of the affine transformation
 * @yy: yy component of the affine transformation
 * @x0: X translation component of the affine transformation
 * @y0: Y translation component of the affine transformation
 *
 * Sets @matrix to be the affine transformation given by
 * @xx, @yx, @xy, @yy, @x0, @y0. The transformation is given
 * by:
 * <programlisting>
 *  x_new = xx * x + xy * y + x0;
 *  y_new = yx * x + yy * y + y0;
 * </programlisting>
 *
 * Since: 1.0
 **/
void fu_matrix_init(FUMatrix2* matrix, float xx, float yx, float xy, float yy, float x0, float y0)
{
    matrix->xx = xx;
    matrix->yx = yx;
    matrix->xy = xy;
    matrix->yy = yy;
    matrix->x0 = x0;
    matrix->y0 = y0;
}
/**
 * cairo_matrix_init_identity:
 * @matrix: a #FUMatrix2
 *
 * Modifies @matrix to be an identity transformation.
 *
 * Since: 1.0
 **/
void fu_matrix_init_identity(FUMatrix2* matrix)
{
    fu_matrix_init(matrix, 1, 0, 0, 1, 0, 0);
}

/**
 * cairo_matrix_init_translate:
 * @matrix: a #FUMatrix2
 * @tx: amount to translate in the X direction
 * @ty: amount to translate in the Y direction
 *
 * Initializes @matrix to a transformation that translates by @tx and
 * @ty in the X and Y dimensions, respectively.
 *
 * Since: 1.0
 **/
void fu_matrix_init_translate(FUMatrix2* matrix, float tx, float ty)
{
    fu_matrix_init(matrix, 1, 0, 0, 1, tx, ty);
}

/**
 * cairo_matrix_init_scale:
 * @matrix: a #FUMatrix2
 * @sx: scale factor in the X direction
 * @sy: scale factor in the Y direction
 *
 * Initializes @matrix to a transformation that scales by @sx and @sy
 * in the X and Y dimensions, respectively.
 *
 * Since: 1.0
 **/
void fu_matrix_init_scale(FUMatrix2* matrix, float sx, float sy)
{
    fu_matrix_init(matrix, sx, 0, 0, sy, 0, 0);
}

/**
 * cairo_matrix_init_rotate:
 * @matrix: a #FUMatrix2
 * @radians: angle of rotation, in radians. The direction of rotation
 * is defined such that positive angles rotate in the direction from
 * the positive X axis toward the positive Y axis. With the default
 * axis orientation of cairo, positive angles rotate in a clockwise
 * direction.
 *
 * Initialized @matrix to a transformation that rotates by @radians.
 *
 * Since: 1.0
 **/
void fu_matrix_init_rotate(FUMatrix2* matrix, float radians)
{
    float s = sin(radians);
    float c = cos(radians);

    fu_matrix_init(matrix, c, s, -s, c, 0, 0);
}

/**
 * cairo_matrix_multiply:
 * @result: a #FUMatrix2 in which to store the result
 * @a: a #FUMatrix2
 * @b: a #FUMatrix2
 *
 * Multiplies the affine transformations in @a and @b together
 * and stores the result in @result. The effect of the resulting
 * transformation is to first apply the transformation in @a to the
 * coordinates and then apply the transformation in @b to the
 * coordinates.
 *
 * It is allowable for @result to be identical to either @a or @b.
 *
 * Since: 1.0
 **/
/*
 * XXX: The ordering of the arguments to this function corresponds
 *      to [row_vector]*A*B. If we want to use column vectors instead,
 *      then we need to switch the two arguments and fix up all
 *      uses.
 */
void fu_matrix_multiply(FUMatrix2* result, const FUMatrix2* a, const FUMatrix2* b)
{
    FUMatrix2 r;

    r.xx = a->xx * b->xx + a->yx * b->xy;
    r.yx = a->xx * b->yx + a->yx * b->yy;

    r.xy = a->xy * b->xx + a->yy * b->xy;
    r.yy = a->xy * b->yx + a->yy * b->yy;

    r.x0 = a->x0 * b->xx + a->y0 * b->xy + b->x0;
    r.y0 = a->x0 * b->yx + a->y0 * b->yy + b->y0;

    *result = r;
}
/**
 * cairo_matrix_translate:
 * @matrix: a #FUMatrix2
 * @tx: amount to translate in the X direction
 * @ty: amount to translate in the Y direction
 *
 * Applies a translation by @tx, @ty to the transformation in
 * @matrix. The effect of the new transformation is to first translate
 * the coordinates by @tx and @ty, then apply the original transformation
 * to the coordinates.
 *
 * Since: 1.0
 **/
void fu_matrix_translate(FUMatrix2* matrix, float tx, float ty)
{
    FUMatrix2 tmp;

    fu_matrix_init_translate(&tmp, tx, ty);

    fu_matrix_multiply(matrix, &tmp, matrix);
}

/**
 * cairo_matrix_scale:
 * @matrix: a #FUMatrix2
 * @sx: scale factor in the X direction
 * @sy: scale factor in the Y direction
 *
 * Applies scaling by @sx, @sy to the transformation in @matrix. The
 * effect of the new transformation is to first scale the coordinates
 * by @sx and @sy, then apply the original transformation to the coordinates.
 *
 * Since: 1.0
 **/
void fu_matrix_scale(FUMatrix2* matrix, float sx, float sy)
{
    FUMatrix2 tmp;

    fu_matrix_init_scale(&tmp, sx, sy);

    fu_matrix_multiply(matrix, &tmp, matrix);
}

/**
 * cairo_matrix_rotate:
 * @matrix: a #FUMatrix2
 * @radians: angle of rotation, in radians. The direction of rotation
 * is defined such that positive angles rotate in the direction from
 * the positive X axis toward the positive Y axis. With the default
 * axis orientation of cairo, positive angles rotate in a clockwise
 * direction.
 *
 * Applies rotation by @radians to the transformation in
 * @matrix. The effect of the new transformation is to first rotate the
 * coordinates by @radians, then apply the original transformation
 * to the coordinates.
 *
 * Since: 1.0
 **/
void fu_matrix_rotate(FUMatrix2* matrix, float radians)
{
    FUMatrix2 tmp;

    fu_matrix_init_rotate(&tmp, radians);

    fu_matrix_multiply(matrix, &tmp, matrix);
}

/**
 * cairo_matrix_transform_distance:
 * @matrix: a #FUMatrix2
 * @dx: X component of a distance vector. An in/out parameter
 * @dy: Y component of a distance vector. An in/out parameter
 *
 * Transforms the distance vector (@dx,@dy) by @matrix. This is
 * similar to cairo_matrix_transform_point() except that the translation
 * components of the transformation are ignored. The calculation of
 * the returned vector is as follows:
 *
 * <programlisting>
 * dx_new = xx * dx + xy * dy;
 * dy_new = yx * dx + yy * dy;
 * </programlisting>
 *
 * Since: 1.0
 **/
void fu_matrix_transform_distance(const FUMatrix2* matrix, float* dx, float* dy)
{
    float new_x = (matrix->xx * *dx + matrix->xy * *dy);
    float new_y = (matrix->yx * *dx + matrix->yy * *dy);

    *dx = new_x;
    *dy = new_y;
}

/**
 * cairo_matrix_transform_point:
 * @matrix: a #FUMatrix2
 * @x: X position. An in/out parameter
 * @y: Y position. An in/out parameter
 *
 * Transforms the point (@x, @y) by @matrix.
 *
 * Since: 1.0
 **/
void fu_matrix_transform_point(const FUMatrix2* matrix, float* x, float* y)
{
    fu_matrix_transform_distance(matrix, x, y);

    *x += matrix->x0;
    *y += matrix->y0;
}
//
// pos2node

static void t_pos2_node_prepend(TPos2Node** head, const FUSize* screenSize, const FUPoint2* src, const FUMatrix2* matrix)
{
    TPos2Node* pos = buddy_malloc(defPos2NodeBuddy, sizeof(TPos2Node));
    memset(pos, 0, sizeof(TPos2Node));
    pos->position.x = src->x;
    pos->position.y = src->y;
    fu_matrix_transform_point(matrix, &pos->position.x, &pos->position.y);
    fu_vec2_ndc_init_from_screen(screenSize->width, screenSize->height, pos->position.x, pos->position.y, &pos->position.x, &pos->position.y);
    pos->position.z = 0.0f;
    pos->position.w = 1.0f;
    pos->next = *head;
    pos->prev = NULL;
    if (*head)
        (*head)->prev = pos;
    *head = pos;
}

static void t_pos2_node_remove_all(TPos2Node* pos)
{
    TPos2Node* next;
    while (pos) {
        next = pos->next;
        buddy_free(defPos2NodeBuddy, pos);
        pos = next;
    }
}
//
// context2
static void fu_context2_destroy(FUTypeInfo* info)
{
    fu_free(defPos2NodePool);
    fu_free(info);
}

static void fu_context2_finalize(FUObject* obj)
{
    FUContext2* ctx = (FUContext2*)obj;
    fu_array_unref(ctx->vertices);
    fu_array_unref(ctx->indices);
    // 例外：命令缓冲是链表
    // 例外：命令缓冲链表中可能有其它 context 的命令
    fu_command_buffer_free_all(ctx->commands);
    t_pos2_node_remove_all(ctx->paths);
    // fu_object_unref(ctx->surface);
}

static void fu_context2_class_init(FUObjectClass* oc)
{
    oc->finalize = fu_context2_finalize;
}

uint64_t fu_context2_get_type()
{
    static uint64_t tid = 0;
    if (!tid) {
        FUTypeInfo* info = fu_malloc0(sizeof(FUTypeInfo));
        info->size = sizeof(FUContext2);
        info->init = fu_context2_class_init;
        info->destroy = fu_context2_destroy;
        tid = fu_type_register(info, 0);
        defPos2NodePool = fu_malloc(DEF_POS2NODE_SIZE);
        defPos2NodeBuddy = buddy_embed(defPos2NodePool, DEF_POS2NODE_SIZE);
    }
    return tid;
}

FUContext2* fu_context2_new(FUWindow* win)
{
    FUContext2* ctx = (FUContext2*)fu_object_new(FU_TYPE_CONTEXT2);
    ctx->vertices = fu_array_new_full(sizeof(FUVertex), DEF_VERTEX_LEN);
    ctx->indices = fu_array_new_full(sizeof(FUPoint3), DEF_VERTEX_LEN);
    t_surface_update_size(win, &ctx->size);
    fu_matrix_init_identity(&ctx->mat);
    return ctx;
}

void fu_context2_path_start(FUContext2* ctx)
{
    fu_return_if_fail(ctx);
    t_pos2_node_remove_all(ctx->paths);
    fu_matrix_init_identity(&ctx->mat);
    memset(&ctx->prevPos, 0, sizeof(FUPoint2));

    ctx->state = E_CTX2_STATE_PATH_START;
}

void fu_context2_moveto(FUContext2* ctx, const FUPoint2* pos)
{
    fu_return_if_fail(ctx && pos);
    fu_return_if_true_with_message(E_CTX2_STATE_PATH_REC < ctx->state, "Please call fu_context_path2_start first");
    memcpy(&ctx->prevPos, pos, sizeof(FUPoint2));
}

void fu_context2_movetoxy(FUContext2* ctx, const uint32_t x, const uint32_t y)
{
    static FUPoint2 pos;
    fu_return_if_fail(ctx);
    pos.x = x;
    pos.y = y;
    fu_context2_moveto(ctx, &pos);
}

void fu_context2_lineto(FUContext2* ctx, const FUPoint2* pos)
{
    fu_return_if_fail(ctx && pos);
    fu_return_if_true_with_message(E_CTX2_STATE_PATH_REC < ctx->state, "Please call fu_context_path2_start first");
    if (E_CTX2_STATE_PATH_START == ctx->state)
        t_pos2_node_prepend(&ctx->paths, &ctx->size, &ctx->prevPos, &ctx->mat);
    t_pos2_node_prepend(&ctx->paths, &ctx->size, pos, &ctx->mat);
    ctx->state = E_CTX2_STATE_PATH_REC;
}

void fu_context2_linetoxy(FUContext2* ctx, const uint32_t x, const uint32_t y)
{
    static FUPoint2 pos;
    fu_return_if_fail(ctx);
    pos.x = x;
    pos.y = y;
    fu_context2_lineto(ctx, &pos);
}

void fu_context2_path_fill(FUContext2* ctx, const FURGBA* color)
{
    static FUVertex vetx;
    static FUPoint3 idx;
    fu_return_if_fail(ctx);
    fu_return_if_fail(E_CTX2_STATE_PATH_REC == ctx->state && ctx->paths);
    TPos2Node* pos = ctx->paths;
    uint32_t prevVertCnt = ctx->vertices->len;
    uint32_t currVertCnt = prevVertCnt;
    memset(&vetx, 0, sizeof(FUVertex));
    if (FU_UNLIKELY(!color))
        memset(&vetx.color, 1, sizeof(FUVec4));
    else
        fu_rgba_to_float(color, &vetx.color.x, &vetx.color.y, &vetx.color.z, &vetx.color.w);
    while (pos) {
        ctx->paths = pos->next;
        memcpy(&vetx.position, &pos->position, sizeof(FUVec4));
        fu_array_append_val(ctx->vertices, vetx);
        buddy_free(defPos2NodeBuddy, pos);
        pos = ctx->paths;
        currVertCnt += 1;
    }
    for (uint32_t i = prevVertCnt; i < currVertCnt - 2; i++) {
        idx.x = prevVertCnt;
        idx.y = i + 1;
        idx.z = i + 2;
        fu_array_append_val(ctx->indices, idx);
    }
    ctx->state = E_CTX2_STATE_TO_COMMAND;
}

FUCommandBuffer* fu_context2_submit(FUContext2* ctx)
{
    static const char* const defVertexShaders = {
        "#version 330 core\n"
        "#extension GL_ARB_bindless_texture : require\n"
        "layout (location = 0) in vec4 position;\n"
        "layout (location = 1) in vec4 color;\n"
        "layout (location = 2) in vec2 textureCoord;\n"
        "uniform mat4 transform;\n"
        "out vec4 rgb;\n"
        "void main()\n"
        "{\n"
        "   rgb = color;\n"
        "   gl_Position = transform * position;\n"
        "}\0"
    };

    static const char* const defFragmentShaders = {
        "#version 330 core\n"
        "in vec4 rgb;\n"
        "out vec4 color;\n"
        "void main()\n"
        "{\n"
        "   color = rgb;\n"
        "}\0"
    };
    fu_return_val_if_fail(ctx, NULL);
    fu_return_val_if_fail_with_message(E_CTX2_STATE_TO_COMMAND == ctx->state, "Please draw vertices first", NULL);
    // mat4 transform = GLM_MAT4_IDENTITY;
    FUShader* shader = fu_shader_new_from_source(defVertexShaders, defFragmentShaders, EFU_SHADER_SOURCE_TYPE_GLSL);
    fu_shader_program_set_mat4(shader, "transform", GLM_MAT4_IDENTITY);
    ctx->commands = fu_command_buffer_new_prepend_take(ctx->commands, ctx->vertices, ctx->indices, &shader);
    return ctx->commands;
}
#undef fu_context_set_color
void fu_context_set_color(FUContext* ctx, const FURGBA* color)
{
    fu_command_buffer_set_color(ctx->commands, color);
}

#undef fu_context_set_transform2
void fu_context_set_transform2(FUContext* ctx, const FUMatrix2* matrix)
{
    TCommandBuffer* cmdf = (TCommandBuffer*)ctx->commands;
    for (uint32_t i = 0; i < cmdf->vertexCount; i++) {
        fu_matrix_transform_point(matrix, &cmdf->vertices[i].position.x, &cmdf->vertices[i].position.y);
    }

    fu_command_buffer_update(cmdf);
}