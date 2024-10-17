#ifdef _use_thread
#include <stdatomic.h>

#include "main.h"
#include "thread.h"
#include "utils.h"

static unsigned defMaximumProcessorCount = 0;
static FUThreadPool* defThreadPool = NULL;

// typedef struct _TTaskWorkerFunc TTaskWorkerFunc;
// struct _TTaskWorkerFunc {
//     TTaskWorkerFunc* next;
//     FUTask* task;
//     FUWorkerFunc fn;
//     uint32_t idx;
//     // bool cancelled;
// };

// struct _FUAsyncResult {
//     // FUTask* task;
//     // FUNotify dtFree;
//     void* data;
//     char* msg;
//     bool ifSucc;
// };

// typedef enum _ETaskState {
//     E_TASK_STATE_NONE,
//     E_TASK_STATE_RUNNING,
//     E_TASK_STATE_DONE,
//     E_TASK_STATE_CNT
// } ETaskState;

// struct _FUTask {
//     FUObject parent;
//     FUSource* src;
//     FUAsyncReadyCallback cb;
//     FUAsyncResult* res;
//     FUTaskFunc fn;
//     atomic_uint state;
//     void* host;
//     void* cbUsd;
//     void* fnUsd;
//     bool active;
// };
// FU_DEFINE_TYPE(FUTask, fu_task, FU_TYPE_OBJECT)

#ifdef FU_OS_WINDOW
#include <windows.h>

// struct _FUMutex {
//     HANDLE handle;
// };

struct _FUThread {
    FUObject parent;
    FUThreadPool* pool;
    FUThreadFunc fn;
    PTP_WORK work;
    void* usd;
};
FU_DEFINE_TYPE(FUThread, fu_thread, FU_TYPE_OBJECT)

struct _FUThreadPool {
    FUObject parent;
    /** 线程池 */
    PTP_POOL pool;
    /** 回调函数环境 (window) */
    TP_CALLBACK_ENVIRON callBackEnviron;
    /** 清理组 (window) */
    PTP_CLEANUP_GROUP cleanupGroup;
};

FU_DEFINE_TYPE(FUThreadPool, fu_thread_pool, FU_TYPE_OBJECT)

// void fu_mutex_init(FUMutex* mutex) {

// }

// void fu_mutex_free(FUMutex* mutex) {}

static void fu_default_thread_pool_free()
{
    if (FU_LIKELY(defThreadPool))
        fu_object_unref(defThreadPool);
}

static bool fu_default_thread_pool_init()
{
    if (FU_UNLIKELY(!(defThreadPool = fu_thread_pool_new()))) {
        fprintf(stderr, "Init default thread pool failed\n");
        return false;
    }
    atexit(fu_default_thread_pool_free);
    return true;
}

static void fu_thread_cb(PTP_CALLBACK_INSTANCE inst, PVOID usd, PTP_WORK work)
{
    FUThread* th = (FUThread*)usd;
    th->fn(th, th->usd);
    fu_object_unref(th->pool);
    fu_object_unref(th);
}

static void fu_thread_class_init(FUObjectClass* oc)
{
}

bool fu_thread_new_run(FUThreadPool* pool, FUThreadFunc fn, void* usd)
{
    fu_return_val_if_fail(fn, false);
    if (!pool) {
        if (FU_UNLIKELY(!(defThreadPool || fu_default_thread_pool_init())))
            return false;
        pool = defThreadPool;
    }
    FUThread* th = (FUThread*)fu_object_new(FU_TYPE_THREAD);
    fu_winapi_goto_if_fail(th->work = CreateThreadpoolWork(fu_thread_cb, th, &pool->callBackEnviron), out);
    th->pool = (FUThreadPool*)fu_object_ref((FUObject*)pool);
    th->fn = fn;
    th->usd = usd;
    SubmitThreadpoolWork(th->work);
    return true;
out:
    fu_object_unref(th);
    return false;
}

FUThread* fu_thread_new(FUThreadFunc fn, void* usd)
{
    fu_return_val_if_fail(fn, NULL);
    FUThread* th = (FUThread*)fu_object_new(FU_TYPE_THREAD);
    th->fn = fn;
    th->usd = usd;
    return th;
}

bool fu_thread_run(FUThread* thread, FUThreadPool* pool, void* usd)
{
    fu_return_val_if_fail(thread, false);
    if (!pool) {
        if (FU_UNLIKELY(!(defThreadPool || fu_default_thread_pool_init())))
            return false;
        pool = defThreadPool;
    }
    if (!thread->work) {
        fu_winapi_return_val_if_fail(thread->work = CreateThreadpoolWork(fu_thread_cb, thread, &pool->callBackEnviron), false);
    }
    if (!usd)
        thread->usd = usd;
    thread->pool = (FUThreadPool*)fu_object_ref((FUObject*)pool);
    SubmitThreadpoolWork(thread->work);
    fu_object_ref((FUObject*)thread);
    return true;
}

static void fu_thread_pool_finalize(FUThreadPool* self)
{
    printf("%s %p\n", __func__, self);
    CloseThreadpoolCleanupGroupMembers(self->cleanupGroup, FALSE, NULL);
    CloseThreadpoolCleanupGroup(self->cleanupGroup);
    CloseThreadpool(self->pool);
}

static void fu_thread_pool_class_init(FUObjectClass* oc)
{
    if (!(defMaximumProcessorCount = GetMaximumProcessorCount(ALL_PROCESSOR_GROUPS)))
        defMaximumProcessorCount = 1;
    oc->finalize = (FUObjectFunc)fu_thread_pool_finalize;
}

FUThreadPool* fu_thread_pool_new()
{
    FUThreadPool* thread = (FUThreadPool*)fu_object_new(FU_TYPE_THREAD_POOL);
    InitializeThreadpoolEnvironment(&thread->callBackEnviron);
    fu_winapi_goto_if_fail(thread->pool = CreateThreadpool(NULL), out);

    fu_winapi_goto_if_fail(SetThreadpoolThreadMinimum(thread->pool, 1), out);
    fu_winapi_goto_if_fail(thread->cleanupGroup = CreateThreadpoolCleanupGroup(), out);
    SetThreadpoolThreadMaximum(thread->pool, defMaximumProcessorCount);

    SetThreadpoolCallbackPool(&thread->callBackEnviron, thread->pool);
    SetThreadpoolCallbackCleanupGroup(&thread->callBackEnviron, thread->cleanupGroup, NULL);
    return thread;
out:
    fu_object_unref(thread);
    return NULL;
}

void fu_thread_pool_wait(FUThreadPool* pool)
{
    if (!pool)
        pool = defThreadPool;
    if (FU_UNLIKELY(!pool))
        return;
    CloseThreadpoolCleanupGroupMembers(pool->cleanupGroup, FALSE, NULL);
}

TP_CALLBACK_ENVIRON* fu_thread_pool_get_callback_env(FUThreadPool* pool)
{
    return &pool->callBackEnviron;
}

FUThreadPool* fu_thread_pool_get_default()
{
    if (FU_UNLIKELY(!defThreadPool)) {
        if (FU_UNLIKELY(!fu_default_thread_pool_init()))
            return NULL;
    }

    return (FUThreadPool*)fu_object_ref((FUObject*)defThreadPool);
}

// void fu_thread_pool_push_default(FUTask* task)
// {
//     if (FU_UNLIKELY(!defThreadPool)) {
//         if (FU_UNLIKELY(!fu_default_thread_pool_init()))
//             return;
//     }
//
//     // thread->pool = defThreadPool;
//     thread->usd = task;
// }
#endif
#endif
/**
static bool fu_task_source_prepare(FUSource* self, void* usd)
{
    FUTask* task = (FUTask*)usd;
    // printf("%s\n", __func__);
    return E_TASK_STATE_RUNNING <= atomic_load(&task->state);
}

static bool fu_task_source_check(FUSource* self, void* usd)
{
    FUTask* task = (FUTask*)usd;
    // uint32_t st = atomic_load(&task->state);
    // printf("%s %u\n", __func__, st);
    // return E_TASK_STATE_DONE <= st;
    return E_TASK_STATE_DONE <= atomic_load(&task->state);
}

static bool fu_task_source_cb(FUTask* usd)
{
    usd->cb(usd->host, usd->res, usd->cbUsd);
    return false;
}

static void fu_task_source_cleanup(FUSource* self, void* usd)
{
    // 自动释放
    FUTask* task = (FUTask*)usd;
    if (FU_UNLIKELY(E_TASK_STATE_DONE <= atomic_load(&task->state))) {
        // fu_object_unref(task->src);)
        fu_object_unref(usd);
    }
}

static void fu_task_finalize(FUTask* self)
{
    printf("%s\n", __func__);
    fu_object_unref(self->src);
}

static void fu_task_class_init(FUObjectClass* oc)
{
    oc->finalize = (FUObjectFunc)fu_task_finalize;
}

FUTask* fu_task_new(void* host, FUAsyncReadyCallback callback, void* usd)
{
    static const FUSourceFuncs defTaskSourceFuncs = {
        .prepare = fu_task_source_prepare,
        .check = fu_task_source_check,
        .cleanup = fu_task_source_cleanup
    };
    FUTask* task = (FUTask*)fu_object_new(FU_TYPE_TASK);
    atomic_init(&task->state, E_TASK_STATE_NONE);
    task->src = fu_source_new(&defTaskSourceFuncs, task);
    task->host = host;
    task->cb = callback;
    task->cbUsd = usd;
    return task;
}

void fu_task_set_finish_data(FUTask* task, void* data)
{
    task->res = fu_malloc0(sizeof(FUAsyncResult));
    task->res->data = data;
}

static void fu_task_run_async(FUThread* th, FUTask* task)
{
    FUAsyncResult* res = task->res; // fu_malloc0(sizeof(FUAsyncResult));
    if (res)
        res->ifSucc = !(res->msg = task->fn(task, task->fnUsd, res->data));
    else
        task->fn(task, task->fnUsd, NULL);
    // res->task = task;
    // task->res = fu_steal_pointer(&res);
    atomic_store(&task->state, E_TASK_STATE_DONE);
}

void fu_task_run(FUTask* task, FUTaskFunc fn, void* usd)
{
    fu_return_if_fail(task && fn);
    if (FU_UNLIKELY(!defThreadPool)) {
        if (FU_UNLIKELY(!fu_default_thread_pool_init()))
            return;
    }

    task->fnUsd = usd;
    task->fn = fn;
    fu_thread_new_run(defThreadPool, (FUThreadFunc)fu_task_run_async, task);
    fu_source_set_callback(task->src, (FUSourceCallback)fu_task_source_cb, task);
    fu_source_attach(task->src, NULL);
    fu_object_ref((FUObject*)task);
    atomic_store(&task->state, E_TASK_STATE_RUNNING);
}

void* fu_task_run_finish(FUAsyncResult* res, char** msg)
{
    // FUTask* task = res->task;
    void* ret = fu_steal_pointer(&res->data);
    if (!res->ifSucc && msg)
        *msg = fu_steal_pointer(&res->msg);

    fu_free(res);
    return ret;
}
*/