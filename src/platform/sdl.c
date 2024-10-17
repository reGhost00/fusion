#ifdef FU_USE_SDL
#include <stdio.h>
// libs
#include "glad/gl.h"
// custom header
#include "../core/main_inner.h"
#include "../core/utils.h"
#include "../renderer/gl2.h"
#include "sdl_inner.h"

#define DEF_KEYPRESS_MAX_DUR 200
#define DEF_OFFSET_MAX_DUR 500

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
static atomic_int defWindowCount = 0;

static TWindowEvent* t_window_event_new()
{
    TWindowEvent* evt = fu_malloc(sizeof(TWindowEvent));
    evt->resize = fu_malloc0(sizeof(FUSize));
    evt->keyboard = fu_malloc0(sizeof(FUKeyboardEvent));
    evt->mouse = fu_malloc0(sizeof(FUMouseEvent));
    evt->scroll = fu_malloc0(sizeof(FUScrollEvent));
    evt->focused = true;

    evt->next = fu_malloc(sizeof(TWindowEvent));
    evt->next->resize = fu_malloc0(sizeof(FUSize));
    evt->next->keyboard = fu_malloc0(sizeof(FUKeyboardEvent));
    evt->next->mouse = fu_malloc0(sizeof(FUMouseEvent));
    evt->next->scroll = fu_malloc0(sizeof(FUScrollEvent));
    evt->focused = true;

    evt->next->next = evt;
    return evt;
}

static void t_window_event_free(TWindowEvent* evt)
{
    fu_free(evt->next->resize);
    fu_free(evt->next->keyboard);
    fu_free(evt->next->mouse);
    fu_free(evt->next->scroll);
    fu_free(evt->next);

    fu_free(evt->resize);
    fu_free(evt->keyboard);
    fu_free(evt->mouse);
    fu_free(evt->scroll);
    fu_free(evt);
}

static inline void t_sdl_print_error()
{
    fprintf(stderr, "SDL Error: %s\n", SDL_GetError());
}

static FUWindow* t_sdl_new_window(FUWindowConfig* cfg, FUWindow* parent)
{
    fu_abort_if_true(0 > SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO), t_sdl_print_error);
    uint32_t flags = SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI;
    if (cfg->fullscreen)
        flags |= SDL_WINDOW_FULLSCREEN;
    else if (cfg->resizable)
        flags |= SDL_WINDOW_RESIZABLE;

#ifdef FU_RENDERER_TYPE_VK
    flags |= SDL_WINDOW_VULKAN;
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    flags |= SDL_WINDOW_OPENGL;

    parent->window = SDL_CreateWindow(cfg->title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, cfg->width, cfg->height, flags);
    fu_abort_if_true(!parent->window, t_sdl_print_error);

    parent->ctx = SDL_GL_CreateContext(parent->window);
    fu_abort_if_true(!parent->ctx, t_sdl_print_error);

    SDL_GL_SetSwapInterval(1);
    int ver = gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress);
    fprintf(stdout, "[FUSION][SDL] OpenGL %d.%d\n", GLAD_VERSION_MAJOR(ver), GLAD_VERSION_MINOR(ver));
#endif

    if (cfg->minWidth && cfg->minHeight)
        SDL_SetWindowMinimumSize(parent->window, cfg->minWidth, cfg->minHeight);
    if (cfg->maxWidth && cfg->maxHeight)
        SDL_SetWindowMaximumSize(parent->window, cfg->maxWidth, cfg->maxHeight);

    return parent;
}
//
// SDL Event Wrap

static void t_sdl_window_event_handler(FUWindow* win)
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
        return fu_signal_emit(defWindowSignals[E_WINDOW_SIGNAL_CLOSE]);
    case SDL_WINDOWEVENT_RESIZED:
        evt->type = E_WINDOW_EVENT_RESIZE;
        evt->resize->width = sev.window.data1;
        evt->resize->height = sev.window.data2;
        evt->focused = true;
        return fu_signal_emit_with_param(defWindowSignals[E_WINDOW_SIGNAL_RESIZE], evt->resize);
    case SDL_WINDOWEVENT_FOCUS_GAINED:
    case SDL_WINDOWEVENT_MAXIMIZED:
    case SDL_WINDOWEVENT_RESTORED:
        evt->type = E_WINDOW_EVENT_FOCUS;
        evt->focused = true;
        return fu_signal_emit_with_param(defWindowSignals[E_WINDOW_SIGNAL_FOCUS], &evt->focused);
    case SDL_WINDOWEVENT_FOCUS_LOST:
    case SDL_WINDOWEVENT_MINIMIZED:
        evt->type = E_WINDOW_EVENT_FOCUS;
        evt->focused = false;
        return fu_signal_emit_with_param(defWindowSignals[E_WINDOW_SIGNAL_FOCUS], &evt->focused);
    }
}

static void t_sdl_keyboard_event_handler(FUWindow* win)
{
    SDL_Event sev = win->event->event;
    TWindowEvent* evt = win->event = win->event->next;
    evt->type = E_WINDOW_EVENT_KEYBOARD;
    evt->keyboard->scanCode = sev.key.keysym.scancode;
    evt->keyboard->key = sev.key.keysym.sym;
    evt->focused = true;
    if (!sev.key.state) {
        fu_signal_emit_with_param(defWindowSignals[E_WINDOW_SIGNAL_KEYUP], evt->keyboard);
        if (DEF_KEYPRESS_MAX_DUR > sev.key.timestamp - evt->event.key.timestamp)
            fu_signal_emit_with_param(defWindowSignals[E_WINDOW_SIGNAL_KEYPRESS], evt->keyboard);
        return;
    }
    return fu_signal_emit_with_param(defWindowSignals[E_WINDOW_SIGNAL_KEYDOWN], evt->keyboard);
}

static void t_sdl_mouse_event_handler(FUWindow* win)
{
    SDL_Event sev = win->event->event;
    TWindowEvent* evt = win->event = win->event->next;
    evt->type = E_WINDOW_EVENT_MOUSE;
    evt->mouse->button = sev.button.button;
    evt->mouse->position.x = sev.button.x;
    evt->mouse->position.y = sev.button.y;
    if (SDL_MOUSEMOTION == sev.button.type)
        return fu_signal_emit_with_param(defWindowSignals[E_WINDOW_SIGNAL_MOUSE_MOVE], evt->mouse);
    if (SDL_MOUSEBUTTONDOWN == sev.button.type)
        return fu_signal_emit_with_param(defWindowSignals[E_WINDOW_SIGNAL_MOUSE_DOWN], evt->mouse);
    if (SDL_MOUSEBUTTONUP == sev.button.type) {
        fu_signal_emit_with_param(defWindowSignals[E_WINDOW_SIGNAL_MOUSE_UP], evt->mouse);
        if (DEF_KEYPRESS_MAX_DUR > sev.button.timestamp - evt->event.button.timestamp)
            fu_signal_emit_with_param(defWindowSignals[E_WINDOW_SIGNAL_MOUSE_PRESS], evt->mouse);
        return;
    }
    evt->type = E_WINDOW_EVENT_SCROLL;
    evt->scroll->offset.x = evt->scroll->delta.x = sev.wheel.x;
    evt->scroll->offset.y = evt->scroll->delta.y = sev.wheel.y;
    fu_signal_emit_with_param(defWindowSignals[E_WINDOW_SIGNAL_SCROLL], evt->scroll);
}
//
// 信号转发 & 关闭自动清理

static bool t_window_event_wrap_check(FUSource* source, FUWindow* usd)
{
    return SDL_PollEvent((SDL_Event*)usd->event);
}

static bool t_window_event_wrap_dispatch(FUWindow* usd)
{
    switch (usd->event->event.type) {
    case SDL_WINDOWEVENT:
        t_sdl_window_event_handler(usd);
        break;
    case SDL_KEYDOWN:
    case SDL_KEYUP:
        t_sdl_keyboard_event_handler(usd);
        break;
    case SDL_MOUSEMOTION:
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
    case SDL_MOUSEWHEEL:
        t_sdl_mouse_event_handler(usd);
        break;
    case SDL_QUIT:
        usd->event->type = E_WINDOW_EVENT_CLOSE;
        fu_signal_emit(defWindowSignals[E_WINDOW_SIGNAL_CLOSE]);
        return false;
    }

    return 0 < atomic_load_explicit(&defWindowCount, memory_order_relaxed);
}

static void t_window_event_wrap_cleanup(FUSource* source, FUWindow* usd)
{
    // 自动清理
    if (0 < atomic_load_explicit(&defWindowCount, memory_order_relaxed))
        return;
    printf("%s %p\n", __func__, usd);
    fu_source_remove(source);
    fu_object_unref(source);
}

static void fu_window_finalize(FUObject* obj)
{
    printf("%s\n", __func__);
    FUWindow* win = (FUWindow*)obj;
    TApp* app = (TApp*)win->app;
    // 例外：FUWindow 是链表
    if (win->next)
        win->next->prev = win->prev;
    if (win->prev)
        win->prev->next = win->next;
    SDL_GL_DeleteContext(win->ctx);
    SDL_DestroyWindow(win->window);

    fu_ptr_array_unref(win->sources);
    t_window_event_free(win->event);
    fu_object_unref(app);
    if (1 >= atomic_fetch_sub_explicit(&defWindowCount, 1, memory_order_relaxed))
        SDL_Quit();
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
        .check = (FUSourceFuncRevBool)t_window_event_wrap_check,
        .cleanup = (FUSourceFuncRevVoid)t_window_event_wrap_cleanup
    };

    fu_return_val_if_fail(app && cfg, NULL);

    FUWindow* win = t_sdl_new_window(cfg, (FUWindow*)fu_object_new(FU_TYPE_WINDOW));
    TApp* _app = (TApp*)(win->app = fu_object_ref(app));
    win->event = t_window_event_new();
    win->sources = fu_ptr_array_new_full(3, (FUNotify)fu_source_destroy);

    if (0 < atomic_fetch_add_explicit(&defWindowCount, 1, memory_order_relaxed))
        return win;

    FUSource* src = (FUSource*)fu_source_new(&defWindowActiveSourceFuncs, win);
    fu_source_set_callback(src, (FUSourceCallback)t_window_event_wrap_dispatch, win);
    fu_source_attach(src, _app->loop);

    atomic_init(&win->active, true);
    atomic_fetch_add_explicit(&defWindowCount, 1, memory_order_relaxed);
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