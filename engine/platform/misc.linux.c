#ifdef FU_OS_LINUX
#include <errno.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "core/logger.h"
#include "core/memory.h"
#include "core/misc.h"
#include "core/object.h"
#include "misc.linux.inner.h"

void(fu_mutex_init)(FUMutex* mtx)
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(mtx, &attr);
    pthread_mutexattr_destroy(&attr);
}

struct _FUThread {
    FUObject parent;
    FUThreadFunc func;
    pthread_t hwnd;
    void* arg;
    void* rev;
    atomic_bool running;
};
FU_DEFINE_TYPE(FUThread, fu_thread, FU_TYPE_OBJECT)

static void fu_thread_finalize(FUObject* obj)
{
    FUThread* th = (FUThread*)obj;
    if (atomic_load(&th->running)) {
        fu_warning("Object are going to free but thread %d are still alive. detach it.", th->hwnd);
        pthread_cancel(th->hwnd);
        pthread_detach(th->hwnd);
    }
}

static void fu_thread_class_init(FUObjectClass* oc)
{
    oc->finalize = fu_thread_finalize;
}

static void* fu_thread_runner(void* param)
{
    FUThread* th = (FUThread*)param;
    fu_info("Thread %ld start\n", th->hwnd);
    th->rev = th->func(th->arg);
    atomic_store_explicit(&th->running, false, memory_order_release);
    // fu_atomic_bool_set_explicit(&th->running, false, EFU_MEMORY_ORDER_RELAXED);
    pthread_exit(th->rev);
    return NULL;
}

FUThread* fu_thread_new(FUThreadFunc func, void* arg)
{
    FUThread* th = (FUThread*)fu_object_new(FU_TYPE_THREAD);
    atomic_init(&th->running, true);
    // fu_atomic_bool_set_explicit(&th->running, true, EFU_MEMORY_ORDER_RELAXED);
    th->func = func;
    th->arg = arg;
    if (FU_LIKELY(!(errno = pthread_create(&th->hwnd, NULL, fu_thread_runner, th))))
        return th;
    perror("pthread_create");
    fu_object_unref(th);
    return NULL;
}

void* fu_thread_join(FUThread* thread)
{
    fu_return_val_if_fail(thread, NULL);
    if (FU_LIKELY(!(errno = pthread_join(thread->hwnd, NULL))))
        return thread->rev;
    perror("pthread_join");
    return NULL;
}

void fu_thread_detach(FUThread* thread)
{
    fu_return_if_fail(thread);
    pthread_detach(thread->hwnd);
    thread->hwnd = 0;
}

/** 获取系统时间戳，单位：微秒 */
uint64_t fu_os_get_stmp()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000UL + ts.tv_nsec / 1000UL;
}
#endif