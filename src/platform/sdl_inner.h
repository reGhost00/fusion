#ifdef FU_USE_SDL
#pragma once
#include <stdatomic.h>
// libs
#include <SDL2/SDL.h>
#ifdef FU_RENDERER_TYPE_VK
#include <SDL2/SDL_vulkan.h>
#else
#include <SDL2/SDL_opengl.h>
#endif
// custom header
#include "core/array.h"
#include "sdl.h"
typedef enum _EWindowEvent {
    E_WINDOW_EVENT_NONE,
    E_WINDOW_EVENT_CLOSE,
    E_WINDOW_EVENT_FOCUS,
    E_WINDOW_EVENT_RESIZE,
    // E_WINDOW_EVENT_SCALE,
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

struct _FUWindow {
    FUObject parent;
    FUWindow* child;
    FUWindow* prev;
    FUWindow* next;
    FUApp* app;
    TWindowEvent* event;
    /** 用户数据源 */
    FUPtrArray* sources;
    SDL_Window* window;
#ifdef FU_RENDERER_TYPE_VK
    VkInstance instance;
    VkSurfaceKHR surface;
#else
    SDL_GLContext* ctx;
#endif
    atomic_bool active;
};

// struct _FUWindowe {
//     FUObject parent;
//     FUWindow* child;
//     FUWindow* prev;
//     FUWindow* next;
//     FUApp* app;
//     // FURenderer* renderer;
//     // FUSurface* surface;
//     TWindowEvent* event;
//     /** 用户数据源 */
//     FUPtrArray* sources;
//     GLFWwindow* window;
//     // bool active;
//     atomic_bool active;
//     char* test;
// };
#endif