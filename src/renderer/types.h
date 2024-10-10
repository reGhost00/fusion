#ifndef _FU_RENDERER_DEF_H_
#define _FU_RENDERER_DEF_H_

#include <stdint.h>

typedef struct _FURect {
    uint32_t width, height, x, y;
} FURect;

typedef struct _FUSize {
    uint32_t width, height;
} FUSize;

typedef struct _FUVec2 {
    float x, y;
} FUVec2;

typedef struct _FUVec3 {
    float x, y, z;
} FUVec3;

typedef struct _FUVec4 {
    float x, y, z, w;
} FUVec4;

typedef struct _FUPoint2 {
    uint32_t x, y;
} FUPoint2;

typedef struct _FUPoint3 {
    uint32_t x, y, z;
} FUPoint3;

typedef struct _FURGB {
    uint8_t r, g, b;
} FURGB;

typedef struct _FURGBA {
    uint8_t r, g, b, a;
} FURGBA;

//
// 窗口相关
typedef struct _FUWindowConfig {
    char* title;
    uint32_t width;
    uint32_t height;
    uint32_t minWidth;
    uint32_t minHeight;
    uint32_t maxWidth;
    uint32_t maxHeight;
    bool fullscreen;
    bool keepAspectRatio;
    bool resizable;
} FUWindowConfig;

//
FURect* fu_rect_new(uint32_t width, uint32_t height, uint32_t x, uint32_t y);
FUSize* fu_size_new(uint32_t width, uint32_t height);
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

void fu_rgb_to_float(FURGB* rgb, float* fR, float* fG, float* fB);
void fu_rgba_to_float(FURGBA* rgba, float* fR, float* fG, float* fB, float* fA);

FUWindowConfig* fu_window_config_new(const char* title, uint32_t width, uint32_t height);
void fu_window_config_free(FUWindowConfig* cfg);

#define fu_rect_free fu_free
#define fu_size_free fu_free
#define fu_vec2_free fu_free
#define fu_point2_free fu_free
#define fu_rgba_free fu_free

#endif