#ifdef FU_USE_SDL
#pragma once
#define SDL_MAIN_HANDLED
#ifdef FU_RENDERER_TYPE_GL
#include <SDL2/SDL_opengl.h>
#elif defined(FU_RENDERER_TYPE_VK)
#include <SDL2/SDL_vulkan.h>
#endif
#include "core/array.h"

#include "renderer/backend.h"
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
    FUSize resize;
    FUKeyboardEvent keyboard;
    FUMouseEvent mouse;
    FUScrollEvent scroll;
    bool focused;
};

typedef struct _FUWindow {
    FUObject parent;
    FUSurface surface;
    FUApplication* app;
    FUWindow* child;
    TWindowEvent* event;
    /** 用户数据源 */
    FUPtrArray *sources, *contexts;
    SDL_Window* window;

} FUWindow;

#endif