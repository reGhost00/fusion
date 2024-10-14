#include <string.h>
// custom
#include "../core/utils.h"
#include "types.h"

#ifdef FU_NO_DEBUG
#define DEF_WINDOW_TITLE_DEBUG_SUFFIX "[RELEASE]"
#else
#define DEF_WINDOW_TITLE_DEBUG_SUFFIX "[DEBUG]"
#endif
#ifdef FU_RENDERER_TYPE_GL
#define DEF_WINDOW_TITLE_RENDER_SUFFIX "[GL]"
#endif
#define DEF_WINDOW_TITLE "fusion"
FURect* fu_rect_new(uint32_t width, uint32_t height, uint32_t x, uint32_t y)
{
    FURect* rect = fu_malloc(sizeof(FURect));
    rect->width = width;
    rect->height = height;
    rect->x = x;
    rect->y = y;
    return rect;
}

FUSize* fu_size_new(uint32_t width, uint32_t height)
{
    FUSize* size = fu_malloc(sizeof(FUSize));
    size->width = width;
    size->height = height;
    return size;
}

FUVec2* fu_vec2_ndc_new_from_screen(FUSize* screen, uint32_t x, uint32_t y)
{
    FUVec2* ndc = fu_malloc(sizeof(FUVec2));
    ndc->x = 2.0f * x / screen->width - 1.0f;
    ndc->y = 1.0f - 2.0f * y / screen->height;
    return ndc;
}

void fu_vec2_ndc_init_from_screen(uint32_t width, uint32_t height, uint32_t x, uint32_t y, float* ndcX, float* ndcY)
{
    *ndcX = 2.0f * x / width - 1.0f;
    *ndcY = 1.0f - 2.0f * y / height;
}

FUPoint2* fu_point2_new(uint32_t x, uint32_t y)
{
    FUPoint2* point = fu_malloc(sizeof(FUPoint2));
    point->x = x;
    point->y = y;
    return point;
}

FURGBA* fu_rgba_new_white()
{
    FURGBA* rgba = fu_malloc(sizeof(FURGBA));
    memset(rgba, 0xff, sizeof(FURGBA));
    return rgba;
}

FURGBA* fu_rgba_new_gray1()
{
    FURGBA* rgba = fu_malloc(sizeof(FURGBA));
    memset(rgba, 0xdd, sizeof(FURGBA));
    return rgba;
}

FURGBA* fu_rgba_new_gray2()
{
    FURGBA* rgba = fu_malloc(sizeof(FURGBA));
    memset(rgba, 0xbb, sizeof(FURGBA));
    return rgba;
}

FURGBA* fu_rgba_new_gray3()
{
    FURGBA* rgba = fu_malloc(sizeof(FURGBA));
    memset(rgba, 0x99, sizeof(FURGBA));
    return rgba;
}

FURGBA* fu_rgba_new_black2()
{
    FURGBA* rgba = fu_malloc(sizeof(FURGBA));
    memset(rgba, 0x33, sizeof(FURGBA));
    return rgba;
}

FURGBA* fu_rgba_new_red()
{
    FURGBA* rgba = fu_malloc(sizeof(FURGBA));
    memset(rgba, 0xff, sizeof(FURGBA));
    rgba->r = 0xff;
    return rgba;
}

FURGBA* fu_rgba_new_green()
{
    FURGBA* rgba = fu_malloc(sizeof(FURGBA));
    memset(rgba, 0xff, sizeof(FURGBA));
    rgba->g = 0xff;
    return rgba;
}

FURGBA* fu_rgba_new_blue()
{
    FURGBA* rgba = fu_malloc(sizeof(FURGBA));
    memset(rgba, 0xff, sizeof(FURGBA));
    rgba->b = 0xff;
    return rgba;
}

void fu_rgb_to_float(FURGB* rgb, float* fR, float* fG, float* fB)
{
    *fR = (rgb->r & 0xff) / 255.0f;
    *fG = (rgb->g & 0xff) / 255.0f;
    *fB = (rgb->b & 0xff) / 255.0f;
}

void fu_rgba_to_float(FURGBA* rgba, float* fR, float* fG, float* fB, float* fA)
{
    *fR = (rgba->r & 0xff) / 255.0f;
    *fG = (rgba->g & 0xff) / 255.0f;
    *fB = (rgba->b & 0xff) / 255.0f;
    *fA = (rgba->a & 0xff) / 255.0f;
}

/**
 * @brief 获取默认窗口配置
 * 窗口标题附加模式，窗口可调大小
 * @param title 窗口标题
 * @param width 最小宽度
 * @param height 最小高度
 * @return FUWindowConfig*
 */
FUWindowConfig* fu_window_config_new(const char* title, uint32_t width, uint32_t height)
{
    FUWindowConfig* cfg = fu_malloc0(sizeof(FUWindowConfig));
    cfg->title = fu_strdup_printf("%s %s %s", title ? title : DEF_WINDOW_TITLE, DEF_WINDOW_TITLE_RENDER_SUFFIX, DEF_WINDOW_TITLE_DEBUG_SUFFIX);
    cfg->minWidth = cfg->width = width;
    cfg->minHeight = cfg->height = height;
    cfg->resizable = true;
    return cfg;
}

void fu_window_config_free(FUWindowConfig* cfg)
{
    fu_free(cfg->title);
    fu_free(cfg);
}