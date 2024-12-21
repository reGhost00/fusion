#ifdef FU_USE_GLFW
#pragma once
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
// custom header
#include "core/array.h"
#include "renderer/backend.h"

#include "glfw.h"
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
    TWindowEvent* next;
    EWindowEvent type;
    FUSize resize;
    FUKeyboardEvent keyboard;
    FUMouseEvent mouse;
    FUScrollEvent scroll;
    bool focused;
    uint64_t stmp;
};

typedef struct _FUWindow {
    FUObject parent;
    FUSurface surface;
    FUApplication* app;
    FUWindow* child;
    TWindowEvent* event;
    /** 用户数据源 */
    FUPtrArray *sources, *contexts;
    GLFWwindow* window;

} FUWindow;

#endif // FU_USE_GLFW