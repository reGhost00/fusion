#ifdef FU_USE_GLFW
#define GLFW_INCLUDE_NONE
#pragma once
#include <stdatomic.h>
// libs
#include <GLFW/glfw3.h>
// custom header
#include "core/array.h"
#include "glfw.h"
#include "renderer/misc.h"

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
    FUSize* resize;
    FUKeyboardEvent* keyboard;
    FUMouseEvent* mouse;
    FUScrollEvent* scroll;
    bool focused;
    uint64_t stmp;
};

typedef struct _FUWindow {
    FUObject parent;
    // FUWindow* prev;
    // FUWindow* next;
    FUApp* app;
    FUWindow* child;
    TWindowEvent* event;
    /** 用户数据源 */
    FUPtrArray* sources;
    FUPtrArray* contexts;
    GLFWwindow* window;
    FUVec3 color;
    FUSize size;
    atomic_bool active;
} FUWindow;

#endif // FU_USE_GLFW