#pragma once
#include "core/object.h"
#include "platform/glfw.h"
#include "platform/sdl.h"
// #include "frontend.h"
#include "misc.h"

typedef struct _FUMatrix2 {
    float xx, xy, x0;
    float yx, yy, y0;
} FUMatrix2;

typedef struct _FUContext FUContext;

FU_DECLARE_TYPE(FUContext2, fu_context2)
#define FU_TYPE_CONTEXT2 (fu_context2_get_type())
//
// matrix

void fu_matrix_init(FUMatrix2* matrix, float xx, float yx, float xy, float yy, float x0, float y0);
void fu_matrix_init_identity(FUMatrix2* matrix);
void fu_matrix_init_translate(FUMatrix2* matrix, float tx, float ty);
void fu_matrix_init_scale(FUMatrix2* matrix, float sx, float sy);
void fu_matrix_init_rotate(FUMatrix2* matrix, float radians);
void fu_matrix_multiply(FUMatrix2* result, const FUMatrix2* a, const FUMatrix2* b);
void fu_matrix_translate(FUMatrix2* matrix, float tx, float ty);
void fu_matrix_scale(FUMatrix2* matrix, float sx, float sy);
void fu_matrix_rotate(FUMatrix2* matrix, float radians);
void fu_matrix_transform_point(const FUMatrix2* matrix, float* x, float* y);
void fu_matrix_transform_distance(const FUMatrix2* matrix, float* dx, float* dy);
//
// context2

FUContext2* fu_context2_new(FUWindow* window);

void fu_context2_path_start(FUContext2* context);
void fu_context2_path_fill(FUContext2* context, const FURGBA* color);
// void fu_context2_path_stroke(FUContext2* context, const FURGBA* color);
FUCommandBuffer* fu_context2_submit(FUContext2* ctx);

void fu_context2_moveto(FUContext2* context, const FUPoint2* position);
void fu_context2_movetoxy(FUContext2* context, const uint32_t x, const uint32_t y);
void fu_context2_lineto(FUContext2* context, const FUPoint2* position);
void fu_context2_linetoxy(FUContext2* context, const uint32_t x, const uint32_t y);
// void fu_context_curveto2(FUContext* context, const FUPoint2* cp1, const FUPoint2* cp2, const FUPoint2* end);
// void fu_context_curveto2xy(FUContext* context, uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t x3, uint32_t y3);
void fu_context_set_color(FUContext* context, const FURGBA* color);
#define fu_context_set_color(ctx, color) fu_context_set_color((FUContext*)ctx, color)

void fu_context_set_transform2(FUContext* context, const FUMatrix2* matrix);
#define fu_context_set_transform2(ctx, matrix) (fu_context_set_transform2((FUContext*)ctx, matrix))