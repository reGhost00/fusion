#include "core/memory.h"
#include "misc.h"

FUWindowConfig* fu_window_config_new(const char* title, uint32_t width, uint32_t height)
{
    FUWindowConfig* cfg = fu_malloc0(sizeof(FUWindowConfig));
    cfg->title = fu_strdup_printf("%s %s %s", title ? title : DEF_WINDOW_TITLE, DEF_WINDOW_TITLE_RENDER_SUFFIX, DEF_WINDOW_TITLE_DEBUG_SUFFIX);
    cfg->size.width = width;
    cfg->size.height = height;
    cfg->minSize = cfg->size;
    cfg->resizable = true;
    return cfg;
}

void fu_window_config_free(FUWindowConfig* cfg)
{
    fu_free(cfg->title);
    fu_free(cfg);
}

FUSize* fu_size_new(uint32_t width, uint32_t height)
{
    FUSize* size = fu_malloc(sizeof(FUSize));
    size->width = width;
    size->height = height;
    return size;
}

FURect* fu_rect_new(uint32_t width, uint32_t height, uint32_t x, uint32_t y)
{
    FURect* rect = fu_malloc(sizeof(FURect));
    rect->width = width;
    rect->height = height;
    rect->x = x;
    rect->y = y;
    return rect;
}