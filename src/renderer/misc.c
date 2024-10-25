#include <string.h>
// custom
#include "core/memory.h"
#include "misc.inner.h"
#include "platform/glfw.inner.h"
#include "platform/sdl.inner.h"
//
// private

void t_surface_update_size(FUWindow* window, FUSize* size)
{
    fu_return_if_fail(window && size);

#ifdef FU_USE_GLFW
    glfwGetFramebufferSize(window->window, (int*)(&size->width), (int*)(&size->height));
    memcpy(&window->size, size, sizeof(FUSize));
#else
#ifdef FU_USE_SDL
    SDL_GL_GetDrawableSize(window->window, &size->width, &size->height);
    memcpy(&window->size, size, sizeof(FUSize));
#else
    fu_error("Not supported");
#endif
#endif
}
//
// public

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

void fu_rgb_to_float(const FURGB* rgb, float* fR, float* fG, float* fB)
{
    *fR = (rgb->r & 0xff) / 255.0f;
    *fG = (rgb->g & 0xff) / 255.0f;
    *fB = (rgb->b & 0xff) / 255.0f;
}

void fu_rgba_to_float(const FURGBA* rgba, float* fR, float* fG, float* fB, float* fA)
{
    *fR = (rgba->r & 0xff) / 255.0f;
    *fG = (rgba->g & 0xff) / 255.0f;
    *fB = (rgba->b & 0xff) / 255.0f;
    *fA = (rgba->a & 0xff) / 255.0f;
}

void fu_window_push_context(FUWindow* win, FUContext* ctx)
{
    fu_return_if_fail(win && ctx);
    ctx->idx = win->contexts->len;
    fu_ptr_array_push(win->contexts, ctx);
}

void fu_window_remove_context(FUWindow* win, FUContext* ctx)
{
    fu_return_if_fail(win && ctx);
    fu_ptr_array_remove_index(win->contexts, ctx->idx);
}
