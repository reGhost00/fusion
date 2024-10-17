#ifdef FU_USE_GLFW
#define GLFW_INCLUDE_NONE
#include <stdio.h>
// libs
#include "glad/gl.h"
#include <GLFW/glfw3.h>
// custom header
#include "../core/main_inner.h"
#include "../core/timer.h"
#include "../core/utils.h"
#include "../renderer/gl2.h"
#include "glfw_inner.h"

#define DEF_KEYPRESS_MAX_DUR 2000
#define DEF_OFFSET_MAX_DUR 500

typedef enum _EGlfwEvent {
    E_GLFW_EVENT_CLOSE,
    E_GLFW_EVENT_FOCUS,
    E_GLFW_EVENT_RESIZE,
    // E_GLFW_EVENT_SCALE,
    E_GLFW_EVENT_KEYDOWN,
    E_GLFW_EVENT_KEYUP,
    E_GLFW_EVENT_KEYPRESS,
    E_GLFW_EVENT_MOUSE_MOVE,
    E_GLFW_EVENT_MOUSE_DOWN,
    E_GLFW_EVENT_MOUSE_UP,
    E_GLFW_EVENT_MOUSE_PRESS,
    E_GLFW_EVENT_SCROLL,
    E_GLFW_EVENT_CNT
} EGlfwEvent;

static FUSignal* defGlfwSignals[E_GLFW_EVENT_CNT] = { 0 };
static atomic_int defWindowCount = 0;

static TWindowEvent* t_window_event_new()
{
    TWindowEvent* evt = fu_malloc0(sizeof(TWindowEvent));
    // evt->scale = fu_malloc0(sizeof(FUVec2));
    evt->resize = fu_malloc0(sizeof(FUSize));
    evt->keyboard = fu_malloc0(sizeof(FUKeyboardEvent));
    evt->mouse = fu_malloc0(sizeof(FUMouseEvent));
    evt->scroll = fu_malloc0(sizeof(FUScrollEvent));

    evt->next = fu_malloc0(sizeof(TWindowEvent));
    // evt->next->scale = fu_malloc0(sizeof(FUVec2));
    evt->next->resize = fu_malloc0(sizeof(FUSize));
    evt->next->keyboard = fu_malloc0(sizeof(FUKeyboardEvent));
    evt->next->mouse = fu_malloc0(sizeof(FUMouseEvent));
    evt->next->scroll = fu_malloc0(sizeof(FUScrollEvent));

    evt->next->next = evt;
    return evt;
}

static void t_window_event_free(TWindowEvent* evt)
{
    // fu_free(evt->next->scale);
    fu_free(evt->next->resize);
    fu_free(evt->next->keyboard);
    fu_free(evt->next->mouse);
    fu_free(evt->next->scroll);
    fu_free(evt->next);

    // fu_free(evt->scale);
    fu_free(evt->resize);
    fu_free(evt->keyboard);
    fu_free(evt->mouse);
    fu_free(evt->scroll);
    fu_free(evt);
}

//
// GLFW Event Wrap
static void t_glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}
//
// GLFW Event Wrap
// Window Callback
static void t_glfw_close_callback(GLFWwindow* window)
{
    FUWindow* win = (FUWindow*)glfwGetWindowUserPointer(window);
    TWindowEvent* evt = win->event = win->event->next;
    evt->type = E_WINDOW_EVENT_CLOSE;
    evt->stmp = fu_timer_get_stmp();
    fu_signal_emit(defGlfwSignals[E_GLFW_EVENT_CLOSE]);
}

static void t_glfw_focus_callback(GLFWwindow* window, int focused)
{
    FUWindow* win = (FUWindow*)glfwGetWindowUserPointer(window);
    TWindowEvent* evt = win->event = win->event->next;
    evt->type = E_WINDOW_EVENT_FOCUS;
    evt->stmp = fu_timer_get_stmp();
    evt->focused = focused;
    fu_signal_emit_with_param(defGlfwSignals[E_GLFW_EVENT_FOCUS], &evt->focused);
}

static void t_glfw_resize_callback(GLFWwindow* window, int width, int height)
{
    FUWindow* win = (FUWindow*)glfwGetWindowUserPointer(window);
    TWindowEvent* evt = win->event = win->event->next;
    evt->type = E_WINDOW_EVENT_RESIZE;
    evt->stmp = fu_timer_get_stmp();
    evt->resize->width = fu_max(0, width);
    evt->resize->height = fu_max(0, height);
    fu_signal_emit_with_param(defGlfwSignals[E_GLFW_EVENT_RESIZE], evt->resize);
}

// static void t_glfw_scale_callback(GLFWwindow* window, float xs, float ys)
// {
//     FUWindow* win = (FUWindow*)glfwGetWindowUserPointer(window);
//     TWindowEvent* evt = win->event = win->event->next;
//     evt->type = E_WINDOW_EVENT_SCALE;
//     evt->stmp = fu_timer_get_stmp();
//     evt->scale->x = xs;
//     evt->scale->y = ys;
//     fu_signal_emit_with_param(defGlfwSignals[E_GLFW_EVENT_SCALE], evt->scale);
// }

static void t_glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    FUWindow* win = (FUWindow*)glfwGetWindowUserPointer(window);
    TWindowEvent* evt = win->event = win->event->next;
    evt->type = E_WINDOW_EVENT_KEYBOARD;
    evt->stmp = fu_timer_get_stmp();
    evt->keyboard->key = key;
    evt->keyboard->scanCode = scancode;
    evt->keyboard->mods = mods;
    evt->keyboard->name = glfwGetKeyName(key, scancode);
    if (!action) {
        fu_signal_emit_with_param(defGlfwSignals[E_GLFW_EVENT_KEYUP], evt->keyboard);
        if (DEF_KEYPRESS_MAX_DUR > evt->stmp - evt->next->stmp)
            fu_signal_emit_with_param(defGlfwSignals[E_GLFW_EVENT_KEYPRESS], evt->keyboard);
        return;
    }
    fu_signal_emit_with_param(defGlfwSignals[E_GLFW_EVENT_KEYDOWN], evt->keyboard);
}

static void t_glfw_cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    FUWindow* win = (FUWindow*)glfwGetWindowUserPointer(window);
    TWindowEvent* evt = win->event = win->event->next;
    evt->type = E_WINDOW_EVENT_MOUSE;
    // evt->stmp = fu_timer_get_stmp();
    evt->mouse->position.x = xpos;
    evt->mouse->position.y = ypos;
    fu_signal_emit_with_param(defGlfwSignals[E_GLFW_EVENT_MOUSE_MOVE], evt->mouse);
}

static void t_glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    FUWindow* win = (FUWindow*)glfwGetWindowUserPointer(window);
    TWindowEvent* evt = win->event = win->event->next;
    evt->type = E_WINDOW_EVENT_MOUSE;
    evt->stmp = fu_timer_get_stmp();
    evt->mouse->button = button;
    evt->mouse->mods = mods;
    if (!action) {
        fu_signal_emit_with_param(defGlfwSignals[E_GLFW_EVENT_MOUSE_UP], evt->mouse);
        if (DEF_KEYPRESS_MAX_DUR > evt->stmp - evt->next->stmp)
            fu_signal_emit_with_param(defGlfwSignals[E_GLFW_EVENT_MOUSE_PRESS], evt->mouse);
        return;
    }
    fu_signal_emit_with_param(defGlfwSignals[E_GLFW_EVENT_MOUSE_DOWN], evt->mouse);
}

static void t_glfw_scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    FUWindow* win = (FUWindow*)glfwGetWindowUserPointer(window);
    TWindowEvent* evt = win->event = win->event->next;
    evt->type = E_WINDOW_EVENT_SCROLL;
    evt->stmp = fu_timer_get_stmp();
    evt->scroll->offset.x = xoffset;
    evt->scroll->offset.y = yoffset;
    if (500 < evt->stmp - evt->next->stmp) {
        evt->scroll->delta.x = xoffset;
        evt->scroll->delta.y = yoffset;
    } else {
        evt->scroll->delta.x += xoffset;
        evt->scroll->delta.y += yoffset;
    }
    fu_signal_emit_with_param(defGlfwSignals[E_GLFW_EVENT_SCROLL], evt->scroll);
}

static void t_glfw_event_signal_wrap(FUWindow* win)
{
    glfwSetWindowCloseCallback(win->window, t_glfw_close_callback);
    glfwSetWindowFocusCallback(win->window, t_glfw_focus_callback);
    glfwSetWindowSizeCallback(win->window, t_glfw_resize_callback);
    // glfwSetWindowContentScaleCallback(win->window, t_glfw_scale_callback);
    glfwSetKeyCallback(win->window, t_glfw_key_callback);
    glfwSetCursorPosCallback(win->window, t_glfw_cursor_position_callback);
    glfwSetMouseButtonCallback(win->window, t_glfw_mouse_button_callback);
    glfwSetScrollCallback(win->window, t_glfw_scroll_callback);
    glfwSetWindowUserPointer(win->window, win);
}
#ifdef FU_RENDERER_TYPE_GL
static GLFWwindow* t_glfw_new_window(FUWindowConfig* cfg)
{
    GLFWmonitor* monitor = NULL;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, cfg->resizable ? GLFW_TRUE : GLFW_FALSE);
    if (cfg->fullscreen)
        monitor = glfwGetPrimaryMonitor();
    GLFWwindow* win = glfwCreateWindow(cfg->width, cfg->height, cfg->title, monitor, NULL);
    fu_return_val_if_fail_with_message(win, "Failed to create GLFW window", NULL);
    if (cfg->minWidth && cfg->minHeight) {
        if (cfg->maxWidth && cfg->maxHeight)
            glfwSetWindowSizeLimits(win, cfg->minWidth, cfg->minHeight, cfg->maxWidth, cfg->maxHeight);
        else
            glfwSetWindowSizeLimits(win, cfg->minWidth, cfg->minHeight, GLFW_DONT_CARE, GLFW_DONT_CARE);
    } else if (cfg->maxWidth && cfg->maxHeight)
        glfwSetWindowSizeLimits(win, GLFW_DONT_CARE, GLFW_DONT_CARE, cfg->maxWidth, cfg->maxHeight);
    if (cfg->keepAspectRatio)
        glfwSetWindowAspectRatio(win, cfg->width, cfg->height);
    glfwMakeContextCurrent(win);
    gladLoadGL(glfwGetProcAddress);
    glfwSwapInterval(1);
    return win;
}
#endif
//
// window

static bool t_window_event_wrap_dispatch(FUWindow* usd)
{
    glfwPollEvents();
    return 0 < atomic_load_explicit(&defWindowCount, memory_order_relaxed);
    // return usd->active;
}

static void t_window_event_wrap_cleanup(FUSource* source, FUWindow* usd)
{
    // 自动清理
    if (0 < atomic_load_explicit(&defWindowCount, memory_order_relaxed))
        return;
    fu_source_remove(source);
    fu_object_unref(source);
}

static void fu_window_finalize(FUObject* obj)
{
    FUWindow* win = (FUWindow*)obj;
    TApp* app = (TApp*)win->app;
    // 例外：FUWindow 是链表
    if (win->next)
        win->next->prev = win->prev;
    if (win->prev)
        win->prev->next = win->next;
    glfwDestroyWindow(win->window);
    fu_ptr_array_unref(win->sources);
    t_window_event_free(win->event);
    fu_object_unref(app);
    atomic_fetch_sub_explicit(&defWindowCount, 1, memory_order_relaxed);
}

static void fu_window_dispose(FUObject* obj)
{
    printf("%s\n", __func__);
    FUWindow* win = (FUWindow*)obj;
    FUWindow* child = win->child;
    while (child) {
        win = child->next;
        fu_object_unref(child);
        child = win;
    }
}

static void fu_window_class_init(FUObjectClass* oc)
{
    printf("%s\n", __func__);
    glfwInit();
    glfwSetErrorCallback(t_glfw_error_callback);
    defGlfwSignals[E_GLFW_EVENT_CLOSE] = fu_signal_new("close", oc, false);
    defGlfwSignals[E_GLFW_EVENT_FOCUS] = fu_signal_new("focus", oc, true);
    defGlfwSignals[E_GLFW_EVENT_RESIZE] = fu_signal_new("resize", oc, true);
    // defGlfwSignals[E_GLFW_EVENT_SCALE] = fu_signal_new("scale", oc, true);
    defGlfwSignals[E_GLFW_EVENT_KEYDOWN] = fu_signal_new("key-down", oc, true);
    defGlfwSignals[E_GLFW_EVENT_KEYUP] = fu_signal_new("key-up", oc, true);
    defGlfwSignals[E_GLFW_EVENT_KEYPRESS] = fu_signal_new("key-press", oc, true);
    defGlfwSignals[E_GLFW_EVENT_MOUSE_MOVE] = fu_signal_new("mouse-move", oc, true);
    defGlfwSignals[E_GLFW_EVENT_MOUSE_DOWN] = fu_signal_new("mouse-down", oc, true);
    defGlfwSignals[E_GLFW_EVENT_MOUSE_UP] = fu_signal_new("mouse-up", oc, true);
    defGlfwSignals[E_GLFW_EVENT_MOUSE_PRESS] = fu_signal_new("mouse-press", oc, true);
    defGlfwSignals[E_GLFW_EVENT_SCROLL] = fu_signal_new("scroll", oc, true);
    oc->dispose = fu_window_dispose;
    oc->finalize = fu_window_finalize;
}

uint64_t fu_window_get_type()
{
    static uint64_t tid = 0;
    if (!tid) {
        FUTypeInfo* info = fu_malloc0(sizeof(FUTypeInfo));
        info->size = sizeof(FUWindow);
        info->init = fu_window_class_init;
        info->destroy = (FUTypeInfoDestroyFunc)fu_free;
        tid = fu_type_register(info, FU_TYPE_OBJECT);
    }
    return tid;
}

FUWindow* fu_window_new(FUApp* app, FUWindowConfig* cfg)
{
    static const FUSourceFuncs defWindowActiveSourceFuncs = {
        .cleanup = (FUSourceFuncRevVoid)t_window_event_wrap_cleanup
    };

    fu_return_val_if_fail(app && cfg, NULL);

    FUWindow* win = (FUWindow*)fu_object_new(FU_TYPE_WINDOW);
    TApp* _app = (TApp*)(win->app = fu_object_ref(app));
    win->window = t_glfw_new_window(cfg);
    win->event = t_window_event_new();
    win->sources = fu_ptr_array_new_full(3, (FUNotify)fu_source_destroy);
    atomic_init(&win->active, true);
    t_glfw_event_signal_wrap(win);
    if (0 < atomic_fetch_add_explicit(&defWindowCount, 1, memory_order_relaxed))
        return win;

    FUSource* src = (FUSource*)fu_source_new(&defWindowActiveSourceFuncs, win);
    fu_source_set_callback(src, (FUSourceCallback)t_window_event_wrap_dispatch, win);
    fu_source_attach(src, _app->loop);
    return win;
}

void fu_window_take_source(FUWindow* win, FUSource** source)
{
    FUSource* src = fu_steal_pointer(source);
    TApp* app = (TApp*)win->app;
    fu_ptr_array_push(win->sources, src);
    fu_source_attach(src, app->loop);
}

#endif