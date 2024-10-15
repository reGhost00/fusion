#include "timer.h"
#include <time.h>

#include "main.h"
#include "object.h"
#include "types.h"
#include "utils.h"

#define DEF_TIMER_FORMAT_BUFF_LEN 1600

struct _FUTimer {
    FUObject parent;
    uint64_t prev;
    bool active;
};
FU_DEFINE_TYPE(FUTimer, fu_timer, FU_TYPE_OBJECT)

#ifdef FU_OS_WINDOW
#include "thread.h"
#include <windows.h>

typedef enum _EFUTimeoutState {
    EFU_TIMEOUT_STATE_NONE,
    EFU_TIMEOUT_STATE_WAITING,
    EFU_TIMEOUT_STATE_ACTIVE,
    EFU_TIMEOUT_STATE_CNT
} EFUTimeoutState;

typedef struct _FUTimeoutSource {
    FUSource parent;
    FUThreadPool* pool;
    PTP_TIMER timer;
    EFUTimeoutState state;
    uint64_t ms;
} FUTimeoutSource;
FU_DEFINE_TYPE(FUTimeoutSource, fu_timeout_source, FU_TYPE_SOURCE)

uint64_t fu_timer_get_stmp()
{
    // 返回 100ns 时间戳
    // 1ms = 10'000ns
    uint64_t rev;
    QueryUnbiasedInterruptTime(&rev);
    return rev / 1000ULL;
}

//
// Timer

static void fu_timer_class_init(FUObjectClass* oc)
{
}

FUTimer* fu_timer_new()
{
    return (FUTimer*)fu_object_new(FU_TYPE_TIMER);
}

void fu_timer_start(FUTimer* timer)
{
    QueryUnbiasedInterruptTime(&timer->prev);
    timer->active = true;
}

uint64_t fu_timer_stop(FUTimer* timer)
{
    fu_return_val_if_fail_with_message(timer->active, "Timer not started", 0);
    uint64_t curr;
    QueryUnbiasedInterruptTime(&curr);
    timer->active = false;
    return (curr - timer->prev) / 10'000ULL;
}

uint64_t fu_timer_measure(FUTimer* timer)
{
    fu_return_val_if_fail_with_message(timer->active, "Timer not active", 0);
    uint64_t curr;
    QueryUnbiasedInterruptTime(&curr);
    uint64_t diff = (curr - timer->prev); // / 10'000ULL;
    timer->prev = curr;
    return diff;
}

//
// TimeoutSource

static void CALLBACK fu_timeout_timer_callback(PTP_CALLBACK_INSTANCE inst, PVOID usd, PTP_TIMER timer)
{
    FUTimeoutSource* timeoutSrc = (FUTimeoutSource*)usd;
    timeoutSrc->state = EFU_TIMEOUT_STATE_ACTIVE;
    SetThreadpoolTimer(timer, NULL, 0, 0);
}

static bool fu_timeout_source_prepare(FUSource* src, void* usd)
{
    FUTimeoutSource* timeoutSrc = (FUTimeoutSource*)src;
    if (FU_UNLIKELY(!timeoutSrc->state)) {
        FILETIME FileDueTime;
        ULARGE_INTEGER ulDueTime;
        ulDueTime.QuadPart = (ULONGLONG)(-10'000LL * timeoutSrc->ms);
        FileDueTime.dwHighDateTime = ulDueTime.HighPart;
        FileDueTime.dwLowDateTime = ulDueTime.LowPart;
        SetThreadpoolTimer(timeoutSrc->timer, &FileDueTime, 0, 0);
        timeoutSrc->state = EFU_TIMEOUT_STATE_WAITING;
        return false;
    }
    return true;
}

static bool fu_timeout_source_check(FUSource* src, void* usd)
{
    FUTimeoutSource* timeoutSrc = (FUTimeoutSource*)src;
    if (FU_LIKELY(EFU_TIMEOUT_STATE_ACTIVE > timeoutSrc->state))
        return false;
    timeoutSrc->state = EFU_TIMEOUT_STATE_NONE;
    return true;
}

static void fu_timeout_source_finalize(FUTimeoutSource* self)
{
    fu_object_unref(self->pool);
}

static void fu_timeout_source_class_init(FUObjectClass* oc)
{
    oc->finalize = (FUObjectFunc)fu_timeout_source_finalize;
}

FUSource* fu_timeout_source_new(unsigned ms)
{
    static const FUSourceFuncs fns = {
        .prepare = fu_timeout_source_prepare,
        .check = fu_timeout_source_check
    };
    // FUSource* src;
    // FUTimeoutSource* timeoutSrc = (FUTimeoutSource*)(src = (FUSource*)fu_object_new(fu_timeout_source_get_type()));
    // fu_source_init(src, &fns, NULL);
    // timeoutSrc->pool = fu_thread_pool_get_default();dv
    // fu_winapi_goto_if_fail(timeoutSrc->timer = CreateThreadpoolTimer(fu_timeout_timer_callback, timeoutSrc, fu_thread_pool_get_callback_env(timeoutSrc->pool)), out);
    // timeoutSrc->ms = ms;

    FUTimeoutSource* src = (FUTimeoutSource*)(fu_source_init(fu_object_new(fu_timeout_source_get_type()), &fns, NULL));
    src->pool = fu_thread_pool_get_default();
    fu_winapi_goto_if_fail(src->timer = CreateThreadpoolTimer(fu_timeout_timer_callback, src, fu_thread_pool_get_callback_env(src->pool)), out);
    src->ms = ms;
    return (FUSource*)src;
out:
    fu_object_unref(src);
    return NULL;
}

#else
#include <time.h>

typedef struct _FUTimeoutSource {
    FUSource parent;
    uint64_t prev;
    uint64_t ms;
} FUTimeoutSource;
FU_DEFINE_TYPE(FUTimeoutSource, fu_timeout_source, FU_TYPE_SOURCE)

uint64_t fu_timer_get_stmp()
{
    struct timespec ts;
    // timespec_get(&ts, TIME_UTC);
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 100'0ULL + ts.tv_nsec / 1'000'000ULL;
}

static void fu_timer_class_init(FUObjectClass* oc)
{
}

FUTimer* fu_timer_new()
{
    return (FUTimer*)fu_object_new(FU_TYPE_TIMER);
}

void fu_timer_start(FUTimer* obj)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    obj->prev = ts.tv_sec * 1'000'000UL + ts.tv_nsec / 1'000UL;
    obj->active = true;
}

uint64_t fu_timer_stop(FUTimer* obj)
{
    fu_return_val_if_fail_with_message(obj->active, "Timer not started", 0);
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t curr = ts.tv_sec * 1'000'000UL + ts.tv_nsec / 1'000UL;
    obj->active = false;
    return curr - obj->prev;
}

/**
 * @brief 计算两个时间差
 * 精度: 微秒
 * @param obj 
 * @return uint64_t 
 */
uint64_t fu_timer_measure(FUTimer* obj)
{
    fu_return_val_if_fail_with_message(obj->active, "Timer not active", 0);
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t curr = ts.tv_sec * 1'000'000ULL + ts.tv_nsec / 1'000UL;
    uint64_t diff = curr - obj->prev;
    obj->prev = curr;
    return diff;
}
//
// timeout source

static void fu_timeout_source_class_init(FUObjectClass* oc)
{
}

/**
 * @brief check if the timeout source should be triggered
 * precision: 1ms
 * @param src 
 * @param usd 
 * @return Check 
 */
static bool fu_timeout_source_check(FUSource* src, void* usd)
{
    struct timespec ts;
    FUTimeoutSource* timeoutSrc = (FUTimeoutSource*)src;
    timespec_get(&ts, TIME_UTC);
    uint64_t curr = ts.tv_sec * 1'000 + ts.tv_nsec / 1'000'000; // ms
    if (curr - timeoutSrc->prev < timeoutSrc->ms)
        return false;
    timeoutSrc->prev = curr;
    return true;
}

FUSource* fu_timeout_source_new(unsigned ms)
{
    static const FUSourceFuncs fns = {
        .check = fu_timeout_source_check
    };
    FUTimeoutSource* src = (FUTimeoutSource*)fu_source_init(fu_object_new(fu_timeout_source_get_type()), &fns, NULL);
    // FUTimeoutSource* timeoutSrc = (FUTimeoutSource*)(src = (FUSource*)fu_object_new(fu_timeout_source_get_type()));
    // fu_source_init(src, &fns, NULL);
    src->ms = ms;
    return (FUSource*)src;
}
#endif

char* fu_get_current_time_UTC()
{
    struct timespec ts;
    char* rev = fu_malloc0(DEF_TIMER_FORMAT_BUFF_LEN);
    timespec_get(&ts, TIME_UTC);
    strftime(rev, DEF_TIMER_FORMAT_BUFF_LEN, "%F %T", gmtime(&ts.tv_sec));
    return rev;
}

char* fu_get_current_time_UTC_format(const char* format)
{
    struct timespec ts;
    char* rev = fu_malloc0(DEF_TIMER_FORMAT_BUFF_LEN);
    timespec_get(&ts, TIME_UTC);
    strftime(rev, DEF_TIMER_FORMAT_BUFF_LEN, format, gmtime(&ts.tv_sec));
    return rev;
}

char* fu_get_current_time_local()
{
    struct timespec ts;
    char* rev = fu_malloc0(DEF_TIMER_FORMAT_BUFF_LEN);
    timespec_get(&ts, TIME_UTC);
    strftime(rev, DEF_TIMER_FORMAT_BUFF_LEN, "%x %X", localtime(&ts.tv_sec));
    return rev;
}

char* fu_get_current_time_local_format(const char* format)
{
    struct timespec ts;
    char* rev = fu_malloc0(DEF_TIMER_FORMAT_BUFF_LEN);
    timespec_get(&ts, TIME_UTC);
    strftime(rev, DEF_TIMER_FORMAT_BUFF_LEN, format, localtime(&ts.tv_sec));
    return rev;
}
