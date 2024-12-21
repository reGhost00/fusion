
#ifdef FU_USE_SDL
#define SDL_MAIN_HANDLED
#include <stdatomic.h>
// custom
#include "core/logger.h"
#include "core/main.inner.h"
#include "core/memory.h"

#include "renderer/backend.h"
#include "sdl.inner.h"

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

#ifdef FU_ENABLE_DEBUG_MSG
static void sdl_print_error(const char* msg)
{
    fu_error("%s%s\n", msg, SDL_GetError());
}
#else
#define sdl_print_error(msg)
#endif

static SDL_Window* sdl_window_new(const char* title, uint32_t width, uint32_t height, bool resizable)
{
    if (FU_UNLIKELY(0 > SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))) {
        sdl_print_error("Failed to initialize SDL:\n");
        return NULL;
    }
    bool ifWindow = false;
    uint32_t flags = SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI;
    if (width && height && ~0U > width && ~0U > height) {
        if (resizable)
            flags |= SDL_WINDOW_RESIZABLE;
        ifWindow = true;
    } else {
        SDL_DisplayMode desktop;
        SDL_GetDesktopDisplayMode(0, &desktop);
        flags |= SDL_WINDOW_FULLSCREEN;
        width = desktop.w;
        height = desktop.h;
    }
#ifdef FU_RENDERER_TYPE_GL
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    flags |= SDL_WINDOW_OPENGL;
#elif FU_RENDERER_TYPE_VK
    flags |= SDL_WINDOW_VULKAN;
    SDL_Vulkan_LoadLibrary(NULL);
#endif

    char buff[1920];
    sprintf(buff, "%s%s%s", title ? title : DEF_WINDOW_TITLE, DEF_WINDOW_TITLE_RENDER_SUFFIX, DEF_WINDOW_TITLE_DEBUG_SUFFIX);
    SDL_Window* win = SDL_CreateWindow(buff, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, flags);
    if (FU_UNLIKELY(!win)) {
        sdl_print_error("Failed to create SDL window:\n");
        return NULL;
    }

    if (ifWindow)
        SDL_SetWindowMinimumSize(win, width, height);

    return win;
}

//
// SDL Event Wrap
static bool sdl_window_event_handler(FUWindow* win)
{
    /*
        在 Windows 操作系统下使用 SDL（Simple DirectMedia Layer）开发程序时
        你可能会注意到在调整窗口大小时，只会在调整完毕后收到 SDL_WINDOWEVENT_RESIZED 和 SDL_WINDOWEVENT_SIZE_CHANGED 事件。
        这是因为 Windows 在处理窗口大小调整时的行为特定于操作系统的事件队列机制。

        Windows 处理窗口调整大小的行为
        在 Windows 下，当用户调整窗口大小时，操作系统会发送一系列的消息来处理这个过程：

        WM_SIZING：当用户开始调整窗口大小时，会连续发送该消息，表示窗口正在调整大小过程中。
        WM_SIZE：当用户调整窗口大小结束时，会发送该消息，表示窗口大小调整完毕。
        WM_EXITSIZEMOVE：当用户完成调整大小或移动窗口操作时，会发送该消息。
        SDL 的事件处理机制
        SDL 在后台使用了上述 Windows 消息来生成 SDL 事件。
        由于 WM_SIZE 和 WM_EXITSIZEMOVE 只在用户调整大小结束时发送，因此 SDL 相应地只在这些时刻生成 SDL_WINDOWEVENT_RESIZED 和 SDL_WINDOWEVENT_SIZE_CHANGED 事件。

        具体来说：

        SDL_WINDOWEVENT_RESIZED：当窗口大小调整完毕后发送。
        SDL_WINDOWEVENT_SIZE_CHANGED：窗口大小发生变化时发送。
        这解释了为何你只会在调整窗口大小结束后收到这些事件，而不是在调整过程中连续收到。
        */
    SDL_Event sev = win->event->event;
    TWindowEvent* evt = win->event = win->event->next;
    switch (sev.window.event) {
    case SDL_WINDOWEVENT_CLOSE:
        evt->type = E_WINDOW_EVENT_CLOSE;
        fu_signal_emit(defWindowSignals[E_WINDOW_SIGNAL_CLOSE]);
        return false;
    case SDL_WINDOWEVENT_RESIZED:
        evt->type = E_WINDOW_EVENT_RESIZE;
        evt->resize.width = fu_max(0, sev.window.data1);
        evt->resize.height = fu_max(0, sev.window.data2);
        evt->focused = true;
        // t_surface_valid_check_and_exchange(win->surface, true);
        fu_signal_emit(defWindowSignals[E_WINDOW_SIGNAL_RESIZE], &evt->resize);
        return true;
    case SDL_WINDOWEVENT_FOCUS_GAINED:
    case SDL_WINDOWEVENT_MAXIMIZED:
    case SDL_WINDOWEVENT_RESTORED:
        evt->type = E_WINDOW_EVENT_FOCUS;
        evt->focused = true;
        fu_signal_emit(defWindowSignals[E_WINDOW_SIGNAL_FOCUS], &evt->focused);
        return true;
    case SDL_WINDOWEVENT_FOCUS_LOST:
    case SDL_WINDOWEVENT_MINIMIZED:
        evt->type = E_WINDOW_EVENT_FOCUS;
        evt->focused = false;
        fu_signal_emit(defWindowSignals[E_WINDOW_SIGNAL_FOCUS], &evt->focused);
    }
    return true;
}

static void sdl_keyboard_event_handler(FUWindow* win)
{
    SDL_Event sev = win->event->event;
    TWindowEvent* evt = win->event = win->event->next;
    evt->type = E_WINDOW_EVENT_KEYBOARD;
    evt->keyboard.scancode = sev.key.keysym.scancode;
    evt->keyboard.key = sev.key.keysym.sym;
    evt->focused = true;
    if (!sev.key.state) {
        fu_signal_emit(defWindowSignals[E_WINDOW_SIGNAL_KEYUP], &evt->keyboard);
        if (DEF_KEYPRESS_MAX_DUR > sev.key.timestamp - evt->event.key.timestamp)
            fu_signal_emit(defWindowSignals[E_WINDOW_SIGNAL_KEYPRESS], &evt->keyboard);
        return;
    }
    return fu_signal_emit(defWindowSignals[E_WINDOW_SIGNAL_KEYDOWN], &evt->keyboard);
}

static void sdl_mouse_event_handler(FUWindow* win)
{
    SDL_Event sev = win->event->event;
    TWindowEvent* evt = win->event = win->event->next;
    evt->type = E_WINDOW_EVENT_MOUSE;
    evt->mouse.button = sev.button.button;
    evt->mouse.position.x = sev.button.x;
    evt->mouse.position.y = sev.button.y;
    if (SDL_MOUSEMOTION == sev.button.type)
        return fu_signal_emit(defWindowSignals[E_WINDOW_SIGNAL_MOUSE_MOVE], &evt->mouse);
    if (SDL_MOUSEBUTTONDOWN == sev.button.type)
        return fu_signal_emit(defWindowSignals[E_WINDOW_SIGNAL_MOUSE_DOWN], &evt->mouse);
    if (SDL_MOUSEBUTTONUP == sev.button.type) {
        fu_signal_emit(defWindowSignals[E_WINDOW_SIGNAL_MOUSE_UP], &evt->mouse);
        if (DEF_KEYPRESS_MAX_DUR > sev.button.timestamp - evt->event.button.timestamp)
            fu_signal_emit(defWindowSignals[E_WINDOW_SIGNAL_MOUSE_PRESS], &evt->mouse);
        return;
    }
    evt->type = E_WINDOW_EVENT_SCROLL;
    evt->scroll.offset.x = evt->scroll.delta.x = sev.wheel.x;
    evt->scroll.offset.y = evt->scroll.delta.y = sev.wheel.y;
    fu_signal_emit(defWindowSignals[E_WINDOW_SIGNAL_SCROLL], &evt->scroll);
}
//
// 信号转发 & 关闭自动清理

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

static bool t_window_event_wrap_check(FUSource* source, void* usd)
{
    FUWindow* win = (FUWindow*)usd;
    if (FU_LIKELY(atomic_load_explicit(&defWindowCount, memory_order_relaxed)))
        return SDL_PollEvent((SDL_Event*)win->event);
    return false;
}

static bool t_window_event_wrap_cb(FUWindow* usd)
{
    if (FU_LIKELY(0 < atomic_load_explicit(&defWindowCount, memory_order_relaxed))) {
        switch (usd->event->event.type) {
        case SDL_MOUSEMOTION:
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
        case SDL_MOUSEWHEEL:
            sdl_mouse_event_handler(usd);
            break;
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            sdl_keyboard_event_handler(usd);
            break;
        case SDL_WINDOWEVENT:
            return sdl_window_event_handler(usd);
        case SDL_QUIT:
            usd->event->type = E_WINDOW_EVENT_CLOSE;
            fu_signal_emit(defWindowSignals[E_WINDOW_SIGNAL_CLOSE]);
            return false;
        }
        return true;
    }
    return false;
}

FU_DEFINE_TYPE(FUWindow, fu_window, FU_TYPE_OBJECT)
static void fu_window_dispose(FUObject* obj)
{
    fu_print_func_name();
    FUWindow* win = (FUWindow*)obj;

    SDL_DestroyWindow(win->window);
    t_window_event_free(win->event);
    fu_ptr_array_unref(win->contexts);
    fu_ptr_array_unref(win->sources);
    // fu_object_unref(win->surface);
    fu_object_unref(win->app);
    if (1 >= atomic_fetch_sub_explicit(&defWindowCount, 1, memory_order_relaxed)) {
        // 例外：构造函数带条件初始化全局变量
        // 例外：signal wrap callback 返回 false 时控制窗口退出。此时 defWindowCount 未归零，无法自动清理，但事件源已经移出主循环，导致内存泄漏
        // todo：验证多窗口情况下是否正常关闭
        //
        SDL_Quit();
    }
}

static void fu_window_class_init(FUObjectClass* oc)
{
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
        .check = t_window_event_wrap_check
    };
    fu_return_val_if_fail(app, NULL);
    fu_info("Windowing: SDL\n");
    SDL_Window* sdl = sdl_window_new(title, width, height, resizable);
    if (FU_UNLIKELY(!sdl))
        return NULL;

    FUWindow* win = (FUWindow*)fu_object_new(FU_TYPE_WINDOW);
    if (FU_UNLIKELY(!fu_surface_init(sdl, width, height, &win->surface))) {
        fu_error("Failed to create surface\n");
        SDL_DestroyWindow(sdl);
        return NULL;
    }
    win->app = fu_object_ref(app);
    win->window = fu_steal_pointer(&sdl);
    // win->surface = fu_steal_pointer(&sf);
    win->contexts = fu_ptr_array_new_full(DEF_USER_SOURCE_CNT, (FUNotify)fu_object_unref);
    win->sources = fu_ptr_array_new_full(DEF_USER_SOURCE_CNT, (FUNotify)fu_source_destroy);
    win->event = t_window_event_new();

    SDL_SetWindowData(win->window, "window", win);
    if (atomic_fetch_add_explicit(&defWindowCount, 1, memory_order_relaxed))
        return win;
    // 例外：在多窗口情况下
    // sdl 通过 window id 标识不同窗口的事件
    // 不需要为每个窗口创建独立的事件源
    FUSource* signalWrapSrc = (FUSource*)fu_source_new(&defSignalWrapFns, win);
    fu_source_set_callback(signalWrapSrc, (FUSourceCallback)t_window_event_wrap_cb, win);
    t_main_loop_attach(app->loop, (TSource*)signalWrapSrc);
    fu_object_set_user_data(app, "signal-wrap", signalWrapSrc, (FUNotify)fu_object_unref);
    return win;
}

void fu_window_take_source(FUWindow* win, FUSource** source)
{
    fu_return_if_fail(win && source && *source);
    TSource* src = (TSource*)fu_steal_pointer(source);
    fu_ptr_array_push(win->sources, src);
    t_main_loop_attach(win->app->loop, src);
}

bool fu_window_add_context(FUWindow* win, FUContext* ctx)
{
    fu_print_func_name();
#ifdef ffd
    //  初始化 image buffer 时需要命令缓冲用于复制纹理数据
    //  保持此初始化顺序
    if (FU_UNLIKELY(!fu_context_init_command(ctx, win->surface)))
        return false;
    if (FU_UNLIKELY(!fu_context_init_buffers(ctx, win->surface)))
        return false;
    if (FU_UNLIKELY(!fu_context_init_descriptor(ctx, win->surface)))
        return false;
    if (FU_LIKELY(fu_context_init_synchronization_objects(ctx, win->surface))) {
        fu_ptr_array_push(win->contexts, fu_object_ref(ctx));
        fu_context_init_finish(ctx);
        return true;
    }
#endif
    return false;
}

void fu_window_present(FUWindow* win, FUContext* ctx)
{
#ifdef ffd
    fu_return_if_fail(win);
    if (FU_UNLIKELY(t_surface_valid_check_and_exchange(win->surface, false))) {
        t_surface_resize(win->surface, win->contexts);
    }
    t_surface_present(win->surface, ctx);
#endif
}

#endif