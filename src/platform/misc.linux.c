#ifdef FU_OS_LINUX
#include <stdatomic.h>
#include <errno.h>
#include <string.h>
#include <time.h>
// custom
#include "core/logger.h"
#include "core/memory.h"
#include "misc.linux.inner.h"

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
    fu_info("Thread %ld start", th->hwnd);
    th->rev = th->func(th->arg);
    atomic_store_explicit(&th->running, false, memory_order_release);
    pthread_exit(th->rev);
    return NULL;
}

FUThread* fu_thread_new(FUThreadFunc func, void* arg)
{
    FUThread* th = (FUThread*)fu_object_new(FU_TYPE_THREAD);
    atomic_init(&th->running, true);
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
    return ts.tv_sec * 1'000'000UL + ts.tv_nsec / 1'000UL;
}

void fu_os_print_error_from_code(const char* prefix, const int code)
{
    const char* msg = strerror(code);
    if (msg) {
        fu_error("%s %d %s\n", prefix, code, msg);
    }
    else {
        fu_error("%s %d\n", prefix, code);
    }
}
#endif