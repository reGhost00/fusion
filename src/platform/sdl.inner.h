#ifdef FU_USE_SDL
#pragma once
#include <stdatomic.h>
// libs
#include <SDL2/SDL.h>
#ifdef FU_RENDERER_TYPE_GL
#include <SDL2/SDL_opengl.h>
#elif FU_RENDERER_TYPE_VK
#include <SDL2/SDL_vulkan.h>
#endif
// custom header
#include "core/array.h"
#include "renderer/misc.h"
#include "sdl.h"
typedef enum _EWindowEvent {
    E_WINDOW_EVENT_NONE,
    E_WINDOW_EVENT_CLOSE,
    E_WINDOW_EVENT_FOCUS,
    E_WINDOW_EVENT_RESIZE,
    E_WINDOW_EVENT_SCROLL,
    E_WINDOW_EVENT_MOUSE,
    E_WINDOW_EVENT_KEYBOARD,
    E_WINDOW_EVENT_GAMEPAD,
    E_WINDOW_EVENT_CNT
} EWindowEvent;

typedef struct _TWindowEvent TWindowEvent;
struct _TWindowEvent {
    SDL_Event event;
    TWindowEvent* next;
    EWindowEvent type;
    FUSize* resize;
    FUKeyboardEvent* keyboard;
    FUMouseEvent* mouse;
    FUScrollEvent* scroll;
    bool focused;
};

typedef struct _FUWindow {
    FUObject parent;
    FUApp* app;
    FUWindow* child;
    TWindowEvent* event;
    /** 用户数据源 */
    FUPtrArray* sources;
    FUPtrArray* contexts;
    SDL_Window* window;
#ifdef FU_RENDERER_TYPE_GL
    SDL_GLContext* glCtx;
#elif FU_RENDERER_TYPE_VK
    VkInstance vkInst;
    VkSurfaceKHR vkSf;
#endif
    FUVec3 color;
    FUSize size;
    atomic_bool active;
} FUWindow;

#endif