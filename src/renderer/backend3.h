#ifdef FU_RENDERER_TYPE_GL2
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
//
// #include "_gl.h"
// #include "../_inner.h"
#include "../core/object.h"
#include "types.h"

// FU_DECLARE_TYPE(FUGLRenderer, fu_gl_renderer)
// #define FU_TYPE_GL_RENDERER (fu_gl_renderer_get_type())

FU_DECLARE_TYPE(FUGLSurface, fu_gl_surface)
#define FU_TYPE_GL_SURFACE (fu_gl_surface_get_type())

FU_DECLARE_TYPE(FUGLContext, fu_gl_context)
#define FU_TYPE_GL_CONTEXT (fu_gl_context_get_type())

// GLFWwindow* _fu_gl_new_window(FUWindowConfig* cfg);
// FUGLRenderer* fu_gl_renderer_new(GLFWwindow* window);
// FUGLRenderer* fu_gl_renderer_new_with_window(FUWindowConfig* cfg, GLFWwindow** win);

// void fu_gl_renderer_clean_with_color(FUGLRenderer* renderer, FURGBA* color);
// void fu_gl_renderer_clean(FUGLRenderer* renderer);
// void fu_gl_renderer_swap(FUGLRenderer* renderer);
typedef struct _FUWindow FUWindow;
FUGLSurface* fu_gl_surface_new(FUWindow* window);
void fu_gl_surface_update_viewport_size(FUGLSurface* surface);
void fu_gl_surface_draw(FUGLSurface* surface);

FUGLContext* fu_gl_context_new(FUGLSurface* surface);
void fu_gl_context_path2_start(FUGLContext* context);
void fu_gl_context_path2_fill(FUGLContext* context, FURGBA* color);
void fu_gl_context_submit(FUGLContext* context);

void fu_gl_context_moveto2(FUGLContext* context, uint32_t x, uint32_t y);
void fu_gl_context_lineto2(FUGLContext* context, uint32_t x, uint32_t y);

// void fu_gl_context_path2_stroke(FUGLContext* context, FURGBA* color);

#define _fu_new_window(cfg) _fu_gl_new_window(cfg)
// #define FURenderer FUGLRenderer
// // #define fu_renderer_new(win) fu_gl_renderer_new(win)
// // #define fu_renderer_new_with_window(cfg, win) fu_gl_renderer_new_with_window(cfg, win)
// #define fu_renderer_clean(rd) fu_gl_renderer_clean(rd)
// #define fu_renderer_clean_with_color(rd, color) fu_gl_renderer_clean_with_color(rd, color)
// #define fu_renderer_swap(rd) fu_gl_renderer_swap(rd)

#define FUSurface FUGLSurface
#define fu_surface_new(rd) fu_gl_surface_new(rd)
#define fu_surface_update_viewport_size(srf) fu_gl_surface_update_viewport_size(srf);
#define fu_surface_draw(srf) fu_gl_surface_draw(srf)

#define FUContext FUGLContext
#define fu_context_new(srf) fu_gl_context_new(srf)
#define fu_context_path2_start(ctx) fu_gl_context_path2_start(ctx);
#define fu_context_path2_fill(ctx, c) fu_gl_context_path2_fill(ctx, c);
#define fu_context_submit(ctx) fu_gl_context_submit(ctx)
// #define fu_context_path2_stroke(ctx, c) fu_gl_context_path2_stroke(ctx, c);
#define fu_context_moveto2(ctx, x, y) fu_gl_context_moveto2(ctx, x, y)
#define fu_context_lineto2(ctx, x, y) fu_gl_context_lineto2(ctx, x, y)

#endif