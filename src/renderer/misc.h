#pragma once
#include "platform/misc.h"

typedef struct _FUVec2 {
    float x, y;
} FUVec2;

typedef struct _FUVec3 {
    float x, y, z;
} FUVec3;

typedef struct _FUVec4 {
    float x, y, z, w;
} FUVec4;

typedef struct _FURGB {
    uint8_t r, g, b;
} FURGB;

typedef struct _FURGBA {
    uint8_t r, g, b, a;
} FURGBA;

typedef struct _FUPoint2 {
    uint32_t x, y;
} FUPoint2;

typedef struct _FUPoint3 {
    uint32_t x, y, z;
} FUPoint3;

typedef struct _FUVertex {
    FUVec4 position;
    FUVec4 color;
    FUVec2 texture;
} FUVertex;

typedef enum _EFUVertexLayout {
    EFU_VERTEX_LAYPUT_POSITION,
    EFU_VERTEX_LAYPUT_POSITION_X = EFU_VERTEX_LAYPUT_POSITION,
    EFU_VERTEX_LAYPUT_POSITION_Y,
    EFU_VERTEX_LAYPUT_POSITION_Z,
    EFU_VERTEX_LAYPUT_POSITION_W,
    EFU_VERTEX_LAYPUT_COLOR,
    EFU_VERTEX_LAYPUT_COLOR_R = EFU_VERTEX_LAYPUT_COLOR,
    EFU_VERTEX_LAYPUT_COLOR_G,
    EFU_VERTEX_LAYPUT_COLOR_B,
    EFU_VERTEX_LAYPUT_COLOR_A,
    EFU_VERTEX_LAYPUT_TEXTURE,
    EFU_VERTEX_LAYPUT_TEXTURE_X = EFU_VERTEX_LAYPUT_TEXTURE,
    EFU_VERTEX_LAYPUT_TEXTURE_Y,
    EFU_VERTEX_LAYPUT_CNT
} EFUVertexLayout;

typedef enum _EFUVertexPosition {
    EFU_VERTEX_POSITION_X,
    EFU_VERTEX_POSITION_Y,
    EFU_VERTEX_POSITION_Z,
    EFU_VERTEX_POSITION_W,
    EFU_VERTEX_POSITION_CNT
} EFUVertexPositionLayout;

typedef enum _EFUVertexColor {
    EFU_VERTEX_COLOR_R,
    EFU_VERTEX_COLOR_G,
    EFU_VERTEX_COLOR_B,
    EFU_VERTEX_COLOR_A,
    EFU_VERTEX_COLOR_CNT
} EFUVertexColor;

typedef enum _EFUVertexTexture {
    EFU_VERTEX_TEXTURE_X,
    EFU_VERTEX_TEXTURE_Y,
    EFU_VERTEX_TEXTURE_CNT
} EFUVertexTexture;

FUVec2* fu_vec2_ndc_new_from_screen(FUSize* screen, uint32_t x, uint32_t y);
void fu_vec2_ndc_init_from_screen(uint32_t width, uint32_t height, uint32_t x, uint32_t y, float* ndcX, float* ndcY);

FUPoint2* fu_point2_new(uint32_t x, uint32_t y);

FURGBA* fu_rgba_new_white();
FURGBA* fu_rgba_new_gray1();
FURGBA* fu_rgba_new_gray2();
FURGBA* fu_rgba_new_gray3();
FURGBA* fu_rgba_new_black2();
FURGBA* fu_rgba_new_red();
FURGBA* fu_rgba_new_green();
FURGBA* fu_rgba_new_blue();

void fu_rgb_to_float(const FURGB* rgb, float* fR, float* fG, float* fB);
void fu_rgba_to_float(const FURGBA* rgba, float* fR, float* fG, float* fB, float* fA);

typedef struct _FUContext FUContext;
typedef struct _FUCommandBuffer FUCommandBuffer;

void fu_window_push_context(FUWindow* win, FUContext* ctx);
void fu_window_remove_context(FUWindow* win, FUContext* ctx);