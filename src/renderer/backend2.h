/**
 * @file backend2.h
 * @author your name (you@domain.com)
 * @brief OpenGL 后端 私有头文件
 * @version 0.1
 * @date 2024-10-22
 *
 * @copyright Copyright (c) 2024
 *
 */
#ifdef FU_RENDERER_TYPE_GL
#pragma once
#include "../core/array.h"
// #include "../core/object.h"
#include "../platform/glfw.h"
#include "../platform/sdl.h"
#include "misc.h"
#include "shader.h"

// FU_DECLARE_TYPE(TSurface, t_surface)
// #define T_TYPE_SURFACE (t_surface_get_type())

void t_surface_init(FUWindow* window);
// void t_surface_set_size(FUWindow* window, const FUSize* size);
// void t_surface_get_size(FUWindow* window, FUSize* size);
// void t_surface_prepend_command(TSurface* surface, FUCommandBuffer* cmdf);
void t_surface_present(FUWindow* window);

FUCommandBuffer* fu_command_buffer_new_prepend_take(FUCommandBuffer* head, FUArray* vertices, FUArray* indices, FUShader** shader);

void fu_command_buffer_free(FUCommandBuffer* cmdf);
void fu_command_buffer_free_all(FUCommandBuffer* cmdf);

void fu_command_buffer_set_color(FUCommandBuffer* cmdf, const FURGBA* color);
void fu_command_buffer_update(FUCommandBuffer* cmdf);
#define fu_command_buffer_update(cmdf) (fu_command_buffer_update((FUCommandBuffer*)cmdf))

#endif // FU_RENDERER_TYPE_GL