#include <stdatomic.h>
#include <string.h>
// custom
#include "logger.h"
#include "main.inner.h"
#include "memory.h"
// #include "misc.h"

static_assert(sizeof(FUSource) == sizeof(TSource));
FU_DEFINE_TYPE(FUSource, fu_source, FU_TYPE_OBJECT)

static atomic_uint_fast32_t defNextSourceId = 0;

static void fu_source_class_init(FUObjectClass* oc)
{
}

/**
 * @brief 初始化事件源
 * 用于初始化其它继承了 FUSource 的对象
 */
FUSource* fu_source_init(FUObject* obj, const FUSourceFuncs* fns, void* usd)
{
    TSource* src = (TSource*)obj;
    src->id = atomic_fetch_add_explicit(&defNextSourceId, 1, memory_order_relaxed);
    src->fns = fns;
    src->usdFN = usd;
    return (FUSource*)obj;
}

/** Create a new source */
FUSource* fu_source_new(const FUSourceFuncs* fns, void* usd)
{
    FUSource* rev;
    TSource* src = (TSource*)(rev = (FUSource*)fu_object_new(FU_TYPE_SOURCE));
    src->id = atomic_fetch_add_explicit(&defNextSourceId, 1, memory_order_relaxed);
    src->fns = fns;
    src->usdFN = usd;
    return rev;
}

/** 设置当事件源触发时的回调 */
void fu_source_set_callback(FUSource* source, FUSourceCallback cb, void* usd)
{
    TSource* real = (TSource*)source;
    real->cb = cb;
    real->usdCB = usd;
}

/** 将事件源从所在事件循环中移除 */
void fu_source_remove(FUSource* source)
{
    TSource* real = (TSource*)source;
    if (real->prev)
        real->prev->next = real->next;
    else if (real->loop)
        real->loop->srcs = real->next;
    if (real->next)
        real->next->prev = real->prev;
    if (real->loop) {
        atomic_fetch_sub_explicit(&real->loop->cnt, 1, memory_order_relaxed);
        real->loop = NULL;
    }
    real->prev = real->next = NULL;
}

/** 将事件源从所在事件循环中移除，并销毁 */
void fu_source_destroy(FUSource* source)
{
    fu_source_remove(source);
    fu_object_unref((FUObject*)source);
}

//
// 主循环
FU_DEFINE_TYPE(TMainLoop, t_main_loop, FU_TYPE_OBJECT)
static TMainLoop* defMainLoop = NULL;

static void fu_app_class_init(FUObjectClass* oc)
{
}

/** 主循环状态处理函数 */
typedef void (*TMainLoopStateFunc)(TMainLoop* loop);
static void t_main_loop_class_init(FUObjectClass* oc)
{
}
/**
 * @brief 主循环准备状态处理函数
 * 这是主循环第 1/4 个状态
 * 用于后续状态的资源分配（如果需要）
 * 如果状态处理函数返回 false，表示资源分配失败，跳过后续状态
 * @param loop
 */
static void t_main_loop_prepare(TMainLoop* loop)
{
    fu_return_if_fail(loop->active && loop->srcs);
    for (TSource* src = loop->srcs; src; src = src->next) {
        src->state = E_SOURCE_STATE_CHECK;
        if (src->fns->prepare && !src->fns->prepare((FUSource*)src, src->usdFN))
            src->state = E_SOURCE_STATE_PREPAR;
    }
}
/**
 * @brief 主循环检查状态处理函数
 * 这是主循环第 2/4 个状态
 * 用于检查事件源是否应该触发
 * 如果状态处理函数返回 false，表示不触发，跳过后续状态
 * 如果用户未提供此函数，默认触发
 * @param loop
 */
static void t_main_loop_check(TMainLoop* loop)
{
    fu_return_if_fail(loop->active && loop->srcs);
    for (TSource* src = loop->srcs; src; src = src->next) {
        if (E_SOURCE_STATE_CHECK != src->state)
            continue;
        src->state = E_SOURCE_STATE_ACTIVE;
        if (src->fns->check && !src->fns->check((FUSource*)src, src->usdFN))
            src->state = E_SOURCE_STATE_CHECK;
    }
}
/**
 * @brief 主循环调度状态处理函数
 * 这是主循环第 3/4 个状态
 * 在上一个状态中，用户提供的状态处理函数返回 true，或者用户未提供状态处理函数
 * 则在此阶段调度事件源
 * 如果事件处理函数返回 false
 * 表示无需继续处理此事件源，将在下一个状态中从主循环中分离
 * @param loop
 */
static void t_main_loop_dispatch(TMainLoop* loop)
{
    fu_return_if_fail(loop->active && loop->srcs);
    for (TSource* src = loop->srcs; src; src = src->next) {
        if (E_SOURCE_STATE_ACTIVE != src->state)
            continue;
        fu_continue_if_fail(src->cb);
        src->state = E_SOURCE_STATE_CLEANUP;
        if (!src->cb(src->usdCB))
            src->state = E_SOURCE_STATE_NONE;
    }
}
/**
 * @brief 主循环清理状态处理函数
 * 这是主循环第 4/4 个状态
 * 清理阶段无条件调用用户提供的状态处理函数（如果有）
 * 如果某个事件源在上一阶段的用户处理函数返回 false，此事件源从主循环中分离
 * @param loop
 */
static void t_main_loop_cleanup(TMainLoop* loop)
{
    fu_return_if_fail(loop->active && loop->srcs);
    TSource *tmp, *src = loop->srcs;
    do {
        tmp = src->next;
        if (!src->state)
            fu_source_remove((FUSource*)src);
        if (src->fns->cleanup)
            src->fns->cleanup((FUSource*)src, src->usdFN);

        src = tmp;
    } while (src);
}

TMainLoop* t_main_loop_new(void)
{
    TMainLoop* loop = (TMainLoop*)fu_object_new(T_TYPE_MAIN_LOOP);
    atomic_init(&loop->cnt, 0);
    if (FU_LIKELY(!defMainLoop))
        defMainLoop = loop;
    return loop;
}

// /**
//  * @brief 设置主循环状态为退出
//  * 当主循环运行一周后，主循环将自动退出
//  * @param loop
//  */
// void fu_main_loop_quit(TMainLoop* loop)
// {
//     loop->active = false;
// }
/**
 * @brief 开始主循环
 * 主循环将在 4 个状态中循环，直到主循环中没有任何事件源，或者用户主动调用 fu_main_loop_quit
 * 不是一个循环迭代 4 个状态，而是每次循环向前 1 个状态
 * 每 4 次循环为一个周期，以此为单位检查是否应该退出
 * @param loop
 */
void t_main_loop_run(TMainLoop* loop)
{
    static const TMainLoopStateFunc fns[] = {
        t_main_loop_prepare,
        t_main_loop_check,
        t_main_loop_dispatch,
        t_main_loop_cleanup,
        NULL
    };
    fu_return_if_fail(loop);
    fu_return_if_fail(atomic_load_explicit(&loop->cnt, memory_order_relaxed) && !loop->active);
    loop->active = true;
    do {
        if (fns[loop->state])
            fns[loop->state](loop);
        loop->state = ((loop->state + 1) % E_MAIN_LOOP_STATE_CNT);
        if (!loop->state) {
            if (FU_UNLIKELY(!(loop->active = 0 < atomic_load_explicit(&loop->cnt, memory_order_relaxed))))
                return;
        }
    } while (loop->active);
}

void t_main_loop_attach(TMainLoop* loop, TSource* src)
{
    fu_return_if_fail(src && src->cb);
    if (!loop)
        loop = defMainLoop;
    if (loop->srcs)
        loop->srcs->prev = src;
    src->next = loop->srcs;
    src->loop = loop;
    src->state = E_SOURCE_STATE_PREPAR;
    loop->srcs = src;
    atomic_fetch_add_explicit(&loop->cnt, 1, memory_order_relaxed);
}

//
// App
FU_DEFINE_TYPE(FUApplication, fu_application, FU_TYPE_OBJECT)
static FUSignal* defAppActiveSig = NULL;

static void fu_application_dispose(FUObject* obj)
{
    FUApplication* app = (FUApplication*)obj;
    fu_ptr_array_free(app->sources, true);
    fu_object_unref(app->loop);
}

static void fu_application_class_init(FUObjectClass* oc)
{
    oc->dispose = fu_application_dispose;
    defAppActiveSig = fu_signal_new("active", oc, false);
}

static bool _fu_application_source_cb(void* usd)
{
    fu_signal_emit(defAppActiveSig);
    return false;
}

static void _fu_application_source_cleanup(FUSource* src, void* usd)
{
    fu_source_destroy(src);
}

FUApplication* fu_application_new(void)
{
    static const FUSourceFuncs fns = {
        .cleanup = _fu_application_source_cleanup
    };

    FUApplication* app = (FUApplication*)fu_object_new(FU_TYPE_APPLICATION);
    FUSource* src = fu_source_new(&fns, app);
    app->loop = t_main_loop_new();
    app->sources = fu_ptr_array_new_full(3, (FUNotify)fu_source_destroy);

    fu_source_set_callback(src, _fu_application_source_cb, app);
    t_main_loop_attach(app->loop, (TSource*)src);
    return app;
}

void fu_application_take_source(FUApplication* app, FUSource** source)
{
    fu_return_if_fail(app && source && *source);
    fu_return_if_fail(((TSource*)*source)->cb);
    FUSource* src = fu_steal_pointer(source);
    fu_ptr_array_push(app->sources, src);
    t_main_loop_attach(app->loop, (TSource*)src);
}

void fu_application_run(FUApplication* app)
{
    t_main_loop_run(app->loop);
}

void fu_application_quit(FUApplication* app)
{
    app->loop->active = false;
}