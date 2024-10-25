#include <stdatomic.h>
// custom
#include "array.h"
#include "main.inner.h"
#include "memory.h"
#include "misc.h"

// 事件源 id
// 每个事件源都有唯一的 id
static atomic_uint defNextSourceId = 0;
static FUMainLoop* defMainLoop = NULL;

static_assert(sizeof(FUSource) == sizeof(TSource));
static_assert(alignof(FUSource) >= alignof(TSource));

FU_DEFINE_TYPE(FUMainLoop, fu_main_loop, FU_TYPE_OBJECT)

static_assert(sizeof(FUApp) == sizeof(TApp));
static_assert(alignof(FUApp) >= alignof(TApp));

static void fu_source_class_init(FUObjectClass* oc)
{
    printf("%s\n", __func__);
}

uint64_t fu_source_get_type()
{
    static uint64_t tid = 0;
    if (!tid) {
        FUTypeInfo* info = fu_malloc0(sizeof(FUTypeInfo));
        info->size = sizeof(TSource);
        info->init = fu_source_class_init;
        info->destroy = (FUTypeInfoDestroyFunc)fu_free;
        tid = fu_type_register(info, FU_TYPE_OBJECT);
    }
    return tid;
}

FUSource* fu_source_init(FUObject* obj, const FUSourceFuncs* fns, void* usd)
{
    TSource* src = (TSource*)obj;
    src->id = atomic_fetch_add_explicit(&defNextSourceId, 1, memory_order_relaxed);
    src->fns = fns;
    src->usdFN = usd;
    return (FUSource*)obj;
}

FUSource* fu_source_new(const FUSourceFuncs* fns, void* usd)
{
    FUSource* rev;
    TSource* src = (TSource*)(rev = (FUSource*)fu_object_new(FU_TYPE_SOURCE));
    src->id = atomic_fetch_add_explicit(&defNextSourceId, 1, memory_order_relaxed);
    src->fns = fns;
    src->usdFN = usd;
    printf("%s %p\n", __func__, usd);
    return rev;
}

void fu_source_set_callback(FUSource* source, FUSourceCallback cb, void* usd)
{
    TSource* real = (TSource*)source;
    real->cb = cb;
    real->usdCB = usd;
    printf("%s %p\n", __func__, usd);
}

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

void fu_source_destroy(FUSource* source)
{
    fu_source_remove(source);
    fu_object_unref((FUObject*)source);
}

void fu_source_attach(FUSource* source, FUMainLoop* loop)
{
    fu_return_if_fail(((TSource*)source)->cb);
    if (!loop)
        loop = defMainLoop;
    fu_return_if_fail_with_message(loop, "main loop not found");
    TSource* src = (TSource*)source;
    if (loop->srcs)
        loop->srcs->prev = src;
    src->next = loop->srcs;
    src->loop = loop;
    src->state = E_SOURCE_STATE_PREPAR;
    loop->srcs = src;
    atomic_fetch_add_explicit(&loop->cnt, 1, memory_order_relaxed);
}

//
// MainLoop
typedef void (*FUMainLoopStateFunc)(FUMainLoop* loop);
static void fu_main_loop_class_init(FUObjectClass* oc) { }
/**
 * @brief 主循环准备状态处理函数
 * 这是主循环第 1/4 个状态
 * 用于后续状态的资源分配（如果需要）
 * 如果状态处理函数返回 false，表示资源分配失败，跳过后续状态
 * @param loop
 */
static void fu_main_loop_prepare(FUMainLoop* loop)
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
static void fu_main_loop_check(FUMainLoop* loop)
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
static void fu_main_loop_dispatch(FUMainLoop* loop)
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
static void fu_main_loop_cleanup(FUMainLoop* loop)
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

FUMainLoop* fu_main_loop_new(void)
{
    FUMainLoop* loop = (FUMainLoop*)fu_object_new(FU_TYPE_MAIN_LOOP);
    atomic_init(&loop->cnt, 0);
    return defMainLoop = loop;
}

void fu_main_loop_quit(FUMainLoop* loop)
{
    loop->active = false;
}
/**
 * @brief 开始主循环
 * 主循环将在 4 个状态中循环，直到主循环中没有任何事件源，或者用户主动调用 fu_main_loop_quit
 * 不是一个循环迭代 4 个状态，而是每次循环向前 1 个状态
 * 每 4 次循环为一个周期，以此为单位检查是否应该退出
 * @param loop
 */
void fu_main_loop_run(FUMainLoop* loop)
{
    static const FUMainLoopStateFunc fns[] = {
        fu_main_loop_prepare,
        fu_main_loop_check,
        fu_main_loop_dispatch,
        fu_main_loop_cleanup,
        NULL
    };
    fu_return_if_fail_with_message(atomic_load_explicit(&loop->cnt, memory_order_relaxed) && !loop->active, "No event source in main loop");
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

//
// App
typedef enum _EAppEvent {
    E_APP_EVENT_ACTIVE,
    E_APP_EVENT_QUIT,
    E_APP_EVENT_CNT
} EAppEvent;

static FUSignal* defAppSignals[E_APP_EVENT_CNT] = { 0 };

static bool fu_app_source_active_cb(void* usd)
{
    fu_signal_emit(defAppSignals[E_APP_EVENT_ACTIVE]);
    return false;
}

static void fu_app_source_active_cleanup(FUSource* source, void* usd)
{
    // 自动清理
    fu_object_unref((FUObject*)source);
}

static bool fu_app_source_close_check(FUSource* source, void* usd)
{
    // return 1 >= atomic_load(&((TSource*)source)->loop->cnt);
    return 1 >= ((TSource*)source)->loop->cnt;
}

static bool fu_app_source_close_cb(void* usd)
{
    fu_signal_emit(defAppSignals[E_APP_EVENT_QUIT]);
    return false;
}

static void fu_app_source_close_cleanup(FUSource* source, void* usd)
{
    // 自动清理
    // 检查此数据源在 check 阶段的结果
    // 不要在此检查 loop->srcCnt，因为可能前面几个节点已经卸载
    // 和此数据源在 check 阶段检查结果矛盾
    // 所以此数据源实际上未从主循环中分离
    // 直接释放一个在主循环上的数据源会导致下一周期失败
    if (FU_LIKELY(((TSource*)source)->state))
        return;
    fu_object_unref((FUObject*)source);
}

static void fu_app_finalize(TApp* app)
{
    fu_ptr_array_unref(app->sources);
    fu_object_unref((FUObject*)app->loop);
}

static void fu_app_class_init(FUObjectClass* oc)
{
    defAppSignals[E_APP_EVENT_ACTIVE] = fu_signal_new("active", oc, false);
    defAppSignals[E_APP_EVENT_QUIT] = fu_signal_new("quit", oc, false);
    oc->finalize = (FUObjectFunc)fu_app_finalize;
}

uint64_t fu_app_get_type()
{
    static uint64_t tid = 0;
    if (!tid) {
        FUTypeInfo* info = fu_malloc0(sizeof(FUTypeInfo));
        info->size = sizeof(TApp);
        info->init = fu_app_class_init;
        info->destroy = (FUTypeInfoDestroyFunc)fu_free;
        tid = fu_type_register(info, FU_TYPE_OBJECT);
    }
    return tid;
}

FUApp* fu_app_init(FUObject* obj)
{
    static const FUSourceFuncs defAppActiveSourceFuncs = {
        .cleanup = fu_app_source_active_cleanup
    };
    static const FUSourceFuncs defAppActiveCloseFuncs = {
        .check = fu_app_source_close_check,
        .cleanup = fu_app_source_close_cleanup
    };

    // 例外：一次性数据源，触发后在 cleanup 中销毁
    FUSource* srcActive = fu_source_new(&defAppActiveSourceFuncs, NULL);
    FUSource* srcClose = fu_source_new(&defAppActiveCloseFuncs, NULL);
    TApp* app = (TApp*)obj;
    app->sources = fu_ptr_array_new_full(3, (FUNotify)fu_source_destroy);
    defMainLoop = app->loop = fu_main_loop_new();
    fu_source_set_callback(srcActive, fu_app_source_active_cb, NULL);
    fu_source_set_callback(srcClose, fu_app_source_close_cb, NULL);
    fu_source_attach(srcActive, app->loop);
    fu_source_attach(srcClose, app->loop);

    return (FUApp*)app;
}

FUApp* fu_app_new(void)
{
    return fu_app_init(fu_object_new(FU_TYPE_APP));
}

void fu_app_take_source(FUApp* app, FUSource** source)
{
    TApp* real = (TApp*)app;
    FUSource* src = fu_steal_pointer(source);
    fu_ptr_array_push(real->sources, src);
    fu_source_attach(src, real->loop);
}

void fu_app_run(FUApp* app)
{
    TApp* real = (TApp*)app;
    fu_main_loop_run(real->loop);
}

void fu_app_quit(FUApp* app)
{
    TApp* real = (TApp*)app;
    fu_main_loop_quit(real->loop);
}
