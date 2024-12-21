#ifdef FU_USE_GLFW
#include <stdatomic.h>
#include <stdio.h>
// custom
#include "core/logger.h"
#include "core/main.inner.h"
#include "core/memory.h"

#include "platform/misc.linux.inner.h"
#include "platform/misc.window.inner.h"

#include "renderer/backend.h"

#include "glfw.inner.h"

#define DEF_KEYPRESS_MAX_DUR 2000
#define DEF_OFFSET_MAX_DUR 500
#define DEF_ARRAY_LEN 5
#define DEF_CONTEXT_CNT DEF_ARRAY_LEN
#define DEF_USER_SOURCE_CNT DEF_ARRAY_LEN

typedef enum _EWindowSignal {
    E_WINDOW_SIGNAL_CLOSE,
    E_WINDOW_SIGNAL_FOCUS,
    E_WINDOW_SIGNAL_RESIZE,
    E_WINDOW_SIGNAL_KEYDOWN,
    E_WINDOW_SIGNAL_KEYUP,
    E_WINDOW_SIGNAL_KEYPRESS,
    E_WINDOW_SIGNAL_MOUSE_MOVE,
    E_WINDOW_SIGNAL_MOUSE_DOWN,
    E_WINDOW_SIGNAL_MOUSE_UP,
    E_WINDOW_SIGNAL_MOUSE_PRESS,
    E_WINDOW_SIGNAL_SCROLL,
    E_WINDOW_SIGNAL_CNT
} EWindowSignal;

static FUSignal* defWindowSignals[E_WINDOW_SIGNAL_CNT] = { 0 };
static atomic_uint_fast32_t defWindowCount = 0;

#ifdef FU_ENABLE_DEBUG_MESSAGE
static void glfw_error_callback(int error, const char* description)
{
    fu_error("GLFW Error %d: %s\n", error, description);
}
#else
#define glfw_error_callback(error, description)
#endif

static GLFWwindow* glfw_new_window(const char* title, uint32_t width, uint32_t height, bool resizable)
{
    GLFWmonitor* monitor = NULL;
    glfwInit();
#ifdef FU_RENDERER_TYPE_GL
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#elif FU_RENDERER_TYPE_VK
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#endif
    if (width && height && ~0U > width && ~0U > height)
        glfwWindowHint(GLFW_RESIZABLE, resizable ? GLFW_TRUE : GLFW_FALSE);
    else
        monitor = glfwGetPrimaryMonitor();

    char buff[1920];
    snprintf(buff, sizeof(buff), "%s%s%s", title ? title : DEF_WINDOW_TITLE, DEF_WINDOW_TITLE_RENDER_SUFFIX, DEF_WINDOW_TITLE_DEBUG_SUFFIX);
    GLFWwindow* win = glfwCreateWindow(width, height, buff, monitor, NULL);
    if (FU_UNLIKELY(!win)) {
        fu_error("failed to create window\n");
        return NULL;
    }
    if (!monitor)
        glfwSetWindowSizeLimits(win, width, height, GLFW_DONT_CARE, GLFW_DONT_CARE);

    return win;
}

//
// GLFW Event Wrap

static TWindowEvent* t_window_event_new()
{
    TWindowEvent* evt = fu_malloc0(sizeof(TWindowEvent));
    evt->next = fu_malloc0(sizeof(TWindowEvent));
    evt->next->focused = evt->focused = true;

    evt->next->next = evt;
    return evt;
}

static void t_window_event_free(TWindowEvent* evt)
{
    fu_free(evt->next);
    fu_free(evt);
}

//
// GLFW Event Wrap
// Window Callback
static void glfw_close_callback(GLFWwindow* window)
{
    FUWindow* win = (FUWindow*)glfwGetWindowUserPointer(window);
    TWindowEvent* evt = win->event = win->event->next;
    evt->type = E_WINDOW_EVENT_CLOSE;
    evt->stmp = fu_os_get_stmp();
    fu_signal_emit(defWindowSignals[E_WINDOW_SIGNAL_CLOSE]);
}

static void glfw_focus_callback(GLFWwindow* window, int focused)
{
    FUWindow* win = (FUWindow*)glfwGetWindowUserPointer(window);
    TWindowEvent* evt = win->event = win->event->next;
    evt->type = E_WINDOW_EVENT_FOCUS;
    evt->stmp = fu_os_get_stmp();
    evt->focused = focused;
    fu_signal_emit(defWindowSignals[E_WINDOW_SIGNAL_FOCUS], &evt->focused);
}

static void glfw_resize_callback(GLFWwindow* window, int width, int height)
{
    FUWindow* win = (FUWindow*)glfwGetWindowUserPointer(window);
    TWindowEvent* evt = win->event = win->event->next;
    evt->type = E_WINDOW_EVENT_RESIZE;
    evt->stmp = fu_os_get_stmp();
    evt->resize.width = fu_max(0, width);
    evt->resize.height = fu_max(0, height);
    fu_signal_emit(defWindowSignals[E_WINDOW_SIGNAL_RESIZE], &evt->resize);
}

static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    FUWindow* win = (FUWindow*)glfwGetWindowUserPointer(window);
    TWindowEvent* evt = win->event = win->event->next;
    evt->type = E_WINDOW_EVENT_KEYBOARD;
    evt->stmp = fu_os_get_stmp();
    evt->keyboard.key = key;
    evt->keyboard.scancode = scancode;
    evt->keyboard.mods = mods;
    evt->keyboard.name = glfwGetKeyName(key, scancode);
    if (!action) {
        fu_signal_emit(defWindowSignals[E_WINDOW_SIGNAL_KEYUP], &evt->keyboard);
        if (DEF_KEYPRESS_MAX_DUR > evt->stmp - evt->next->stmp)
            fu_signal_emit(defWindowSignals[E_WINDOW_SIGNAL_KEYPRESS], &evt->keyboard);
        return;
    }
    fu_signal_emit(defWindowSignals[E_WINDOW_SIGNAL_KEYDOWN], &evt->keyboard);
}

static void glfw_cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    FUWindow* win = (FUWindow*)glfwGetWindowUserPointer(window);
    TWindowEvent* evt = win->event = win->event->next;
    evt->type = E_WINDOW_EVENT_MOUSE;
    evt->mouse.position.x = xpos;
    evt->mouse.position.y = ypos;
    fu_signal_emit(defWindowSignals[E_WINDOW_SIGNAL_MOUSE_MOVE], &evt->mouse);
}

static void glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    FUWindow* win = (FUWindow*)glfwGetWindowUserPointer(window);
    TWindowEvent* evt = win->event = win->event->next;
    evt->type = E_WINDOW_EVENT_MOUSE;
    evt->stmp = fu_os_get_stmp();
    evt->mouse.button = button;
    evt->mouse.mods = mods;
    if (!action) {
        fu_signal_emit(defWindowSignals[E_WINDOW_SIGNAL_MOUSE_UP], &evt->mouse);
        if (DEF_KEYPRESS_MAX_DUR > evt->stmp - evt->next->stmp)
            fu_signal_emit(defWindowSignals[E_WINDOW_SIGNAL_MOUSE_PRESS], &evt->mouse);
        return;
    }
    fu_signal_emit(defWindowSignals[E_WINDOW_SIGNAL_MOUSE_DOWN], &evt->mouse);
}

static void glfw_scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    FUWindow* win = (FUWindow*)glfwGetWindowUserPointer(window);
    TWindowEvent* evt = win->event = win->event->next;
    evt->type = E_WINDOW_EVENT_SCROLL;
    evt->stmp = fu_os_get_stmp();
    evt->scroll.offset.x = xoffset;
    evt->scroll.offset.y = yoffset;
    if (500 < evt->stmp - evt->next->stmp) {
        evt->scroll.delta.x = xoffset;
        evt->scroll.delta.y = yoffset;
    } else {
        evt->scroll.delta.x += xoffset;
        evt->scroll.delta.y += yoffset;
    }
    fu_signal_emit(defWindowSignals[E_WINDOW_SIGNAL_SCROLL], &evt->scroll);
}

static void glfw_event_signal_wrap(FUWindow* win)
{
    glfwSetWindowCloseCallback(win->window, glfw_close_callback);
    glfwSetWindowFocusCallback(win->window, glfw_focus_callback);
    glfwSetWindowSizeCallback(win->window, glfw_resize_callback);
    glfwSetKeyCallback(win->window, glfw_key_callback);
    glfwSetCursorPosCallback(win->window, glfw_cursor_position_callback);
    glfwSetMouseButtonCallback(win->window, glfw_mouse_button_callback);
    glfwSetScrollCallback(win->window, glfw_scroll_callback);
    glfwSetWindowUserPointer(win->window, win);
}

//
// window
// static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static bool t_window_event_wrap_check(FUSource* source, FUWindow* usd)
{
    return !(atomic_load_explicit(&defWindowCount, memory_order_relaxed) && glfwWindowShouldClose(usd->window));
}

static bool t_window_event_wrap_cb(FUWindow* usd)
{
    // fixme:
    // 在 linux(wsl:fedora) 平台下
    // 此函数调用顺序没有按照预期
    // 在调用 fu_window_finalize() 仍然能进入下面的代码块
    // 导致 glfwPollEvents() 内部触发异常崩溃
    if (FU_LIKELY(atomic_load_explicit(&defWindowCount, memory_order_relaxed))) {
        glfwPollEvents();
        return true;
    }
    return false;
}

static void t_window_event_wrap_cleanup(FUSource* source, FUWindow* usd)
{
    // 自动清理
    if (FU_LIKELY(0 < atomic_load_explicit(&defWindowCount, memory_order_relaxed)))
        return;
    fu_source_remove(source);
    fu_object_unref(source);
}

FU_DEFINE_TYPE(FUWindow, fu_window, FU_TYPE_OBJECT)
static void fu_window_dispose(FUObject* obj)
{
    fu_print_func_name();
    FUWindow* win = (FUWindow*)obj;
    t_window_event_free(win->event);
    fu_ptr_array_unref(win->contexts);
    fu_ptr_array_unref(win->sources);
    fu_surface_destroy(&win->surface);
    fu_object_unref(win->app);
    glfwDestroyWindow(win->window);
    if (1 >= atomic_fetch_sub_explicit(&defWindowCount, 1, memory_order_relaxed)) {
        // 例外：构造函数带条件初始化全局变量
        // 例外：signal wrap callback 返回 false 时控制窗口退出。此时 defWindowCount 未归零，无法自动清理，但事件源已经移出主循环，导致内存泄漏
        // todo：验证多窗口情况下是否正常关闭
        //

        glfwTerminate();
    }
}

static void fu_window_class_init(FUObjectClass* oc)
{
    fu_print_func_name();

    glfwSetErrorCallback(glfw_error_callback);
    defWindowSignals[E_WINDOW_SIGNAL_CLOSE] = fu_signal_new("close", oc, false);
    defWindowSignals[E_WINDOW_SIGNAL_FOCUS] = fu_signal_new("focus", oc, true);
    defWindowSignals[E_WINDOW_SIGNAL_RESIZE] = fu_signal_new("resize", oc, true);
    defWindowSignals[E_WINDOW_SIGNAL_KEYDOWN] = fu_signal_new("key-down", oc, true);
    defWindowSignals[E_WINDOW_SIGNAL_KEYUP] = fu_signal_new("key-up", oc, true);
    defWindowSignals[E_WINDOW_SIGNAL_KEYPRESS] = fu_signal_new("key-press", oc, true);
    defWindowSignals[E_WINDOW_SIGNAL_MOUSE_MOVE] = fu_signal_new("mouse-move", oc, true);
    defWindowSignals[E_WINDOW_SIGNAL_MOUSE_DOWN] = fu_signal_new("mouse-down", oc, true);
    defWindowSignals[E_WINDOW_SIGNAL_MOUSE_UP] = fu_signal_new("mouse-up", oc, true);
    defWindowSignals[E_WINDOW_SIGNAL_MOUSE_PRESS] = fu_signal_new("mouse-press", oc, true);
    defWindowSignals[E_WINDOW_SIGNAL_SCROLL] = fu_signal_new("scroll", oc, true);
    oc->dispose = fu_window_dispose;
}

FUWindow* fu_window_new(FUApplication* app, const char* title, uint32_t width, uint32_t height, bool resizable)
{
    static const FUSourceFuncs defSignalWrapFns = {
        .check = (FUSourceFunc)t_window_event_wrap_check
    };
    fu_return_val_if_fail(app, NULL);
    GLFWwindow* glfw = glfw_new_window(title, width, height, resizable);
    if (FU_UNLIKELY(!glfw))
        return NULL;

    FUWindow* win = (FUWindow*)fu_object_new(FU_TYPE_WINDOW);
    win->window = fu_steal_pointer(&glfw);
    if (FU_UNLIKELY(!fu_surface_init(win->window, width, height, &win->surface))) {
        fu_error("Failed to create surface\n");
        glfwDestroyWindow(glfw);
        return NULL;
    }
    win->app = fu_object_ref(app);
    win->contexts = fu_ptr_array_new_full(DEF_USER_SOURCE_CNT, (FUNotify)fu_object_unref);
    win->sources = fu_ptr_array_new_full(DEF_USER_SOURCE_CNT, (FUNotify)fu_source_destroy);
    win->event = t_window_event_new();

    glfw_event_signal_wrap(win);
    if (atomic_fetch_add_explicit(&defWindowCount, 1, memory_order_relaxed))
        return win;

    FUSource* src = (FUSource*)fu_source_new(&defSignalWrapFns, win);
    fu_source_set_callback(src, (FUSourceCallback)t_window_event_wrap_cb, win);
    t_main_loop_attach(app->loop, (TSource*)src);
    fu_object_set_user_data(app, "signal-wrap", src, (FUNotify)fu_object_unref);
    return win;
}

void fu_window_take_source(FUWindow* win, FUSource** source)
{
    fu_return_if_fail(win && source && *source);
    TSource* src = (TSource*)fu_steal_pointer(source);
    fu_ptr_array_push(win->sources, src);
    t_main_loop_attach(win->app->loop, src);
}

// bool fu_window_add_context(FUWindow* win, FUContext* ctx)
// {
//     fu_print_func_name();
//     //  初始化 image buffer 时需要命令缓冲用于复制纹理数据
//     //  保持此初始化顺序
//     // if (FU_UNLIKELY(!fu_context_init_command(ctx, win->surface)))
//     //     return false;
//     // if (FU_UNLIKELY(!fu_context_init_buffers(ctx, win->surface)))
//     //     return false;
//     // if (FU_UNLIKELY(!fu_context_init_descriptor(ctx, win->surface)))
//     //     return false;
//     // if (FU_LIKELY(fu_context_init_synchronization_objects(ctx, win->surface))) {
//     //     fu_ptr_array_push(win->contexts, fu_object_ref(ctx));
//     //     fu_context_init_finish(ctx);
//     //     return true;
//     // }
//     return false;
// }

bool fu_window_present(FUWindow* win)
{
    fu_return_val_if_fail(win, false);
    FUContext* ctx;
    for (uint32_t i = 0; i < win->contexts->len; i++) {
        ctx = fu_ptr_array_index(win->contexts, i);
        t_context_present(ctx);
    }
    return true;
    // t_surface_present(win->surface, ctx);
}

#endif