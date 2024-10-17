#ifdef FU_USE_GLFW
#pragma once
#include <stdatomic.h>
// custom header
#include "core/array.h"
#include "glfw.h"
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
    TWindowEvent* next;
    EWindowEvent type;
    // FUVec2* scale;
    FUSize* resize;
    FUKeyboardEvent* keyboard;
    FUMouseEvent* mouse;
    FUScrollEvent* scroll;
    bool focused;
    uint64_t stmp;
};

struct _FUWindow {
    FUObject parent;
    FUWindow* child;
    FUWindow* prev;
    FUWindow* next;
    FUApp* app;
    // FURenderer* renderer;
    // FUSurface* surface;
    TWindowEvent* event;
    /** 用户数据源 */
    FUPtrArray* sources;
    GLFWwindow* window;
    // bool active;
    atomic_bool active;
};

#endif // FU_USE_GLFW