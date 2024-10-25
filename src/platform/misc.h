#pragma once
#include <stdint.h>
// custom header
#include "core/object.h"

#ifdef NDEBUG
#define DEF_WINDOW_TITLE_DEBUG_SUFFIX "[RELEASE]"
#else
#define DEF_WINDOW_TITLE_DEBUG_SUFFIX "[DEBUG]"
#endif
#ifdef FU_RENDERER_TYPE_VK
#define DEF_WINDOW_TITLE_RENDER_SUFFIX "[VK]"
#else
#define DEF_WINDOW_TITLE_RENDER_SUFFIX "[GL]"
#endif
#define DEF_WINDOW_TITLE "fusion"

typedef struct _FURect {
    uint32_t width, height, x, y;
} FURect;

typedef struct _FUSize {
    uint32_t width, height;
} FUSize;

typedef struct _FUOffset2 {
    int32_t x, y;
} FUOffset2;

typedef struct _FUOffset3 {
    int32_t x, y, z;
} FUOffset3;

FU_DECLARE_TYPE(FUWindow, fu_window)
#define FU_TYPE_WINDOW (fu_window_get_type())

FUSize* fu_size_new(uint32_t width, uint32_t height);
FURect* fu_rect_new(uint32_t width, uint32_t height, uint32_t x, uint32_t y);
//
// 窗口相关
typedef struct _FUWindowConfig {
    char* title;
    FUSize size;
    FUSize minSize;
    FUSize maxSize;
    bool fullscreen;
    bool keepAspectRatio;
    bool resizable;
} FUWindowConfig;

FUWindowConfig* fu_window_config_new(const char* title, uint32_t width, uint32_t height);
void fu_window_config_free(FUWindowConfig* cfg);
