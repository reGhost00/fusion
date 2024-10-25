#include <time.h>
// custom
#include "../platform/misc.window.inner.h"
#include "main.h"
#include "memory.h"
#include "object.h"
#include "timer.h"

#define DEF_TIMER_FORMAT_BUFF_LEN 1600
//
// 计时器
// 精度：微秒
struct _FUTimer {
    FUObject parent;
    uint64_t prev;
};
FU_DEFINE_TYPE(FUTimer, fu_timer, FU_TYPE_OBJECT)
//
// 定时器事件源
// 精度：微秒
typedef struct _FUTimeoutSource {
    FUSource parent;
    uint64_t prev;
    uint64_t dur;
} FUTimeoutSource;

FU_DEFINE_TYPE(FUTimeoutSource, fu_timeout_source, FU_TYPE_SOURCE)

#ifdef _test
#ifdef FU_OS_WINDOW
#include <windows.h>

/** 时间戳，单位 微秒 */
uint64_t fu_timer_get_stmp()
{
    // 返回 100ns 时间戳
    // 1ms = 10'000ns
    uint64_t rev;
    QueryUnbiasedInterruptTime(&rev);
    return rev / 10ULL;
}

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
}

/** 获取自上次执行 fu_timer_start 后到现在经过的微秒 */
uint64_t fu_timer_measure(FUTimer* timer)
{
    uint64_t curr;
    QueryUnbiasedInterruptTime(&curr);
    uint64_t diff = (curr - timer->prev) / 10ULL;
    timer->prev = curr;
    return diff;
}

#else

uint64_t fu_timer_get_stmp()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1'000'000UL + ts.tv_nsec / 1'000UL;
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
}

/**
 * @brief 计算两个时间差
 * 精度: 微秒
 * @param obj
 * @return uint64_t
 */
uint64_t fu_timer_measure(FUTimer* obj)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t curr = ts.tv_sec * 1'000'000ULL + ts.tv_nsec / 1'000UL;
    uint64_t diff = curr - obj->prev;
    obj->prev = curr;
    return diff;
}

#endif
#endif

static void fu_timer_class_init(FUObjectClass* oc)
{
}

FUTimer* fu_timer_new()
{
    return (FUTimer*)fu_object_new(FU_TYPE_TIMER);
}

void fu_timer_start(FUTimer* timer)
{
    timer->prev = fu_os_get_stmp();
}

/**
 * @brief 计算两个时间差
 * 精度: 微秒
 * @param obj
 * @return uint64_t
 */
uint64_t fu_timer_measure(FUTimer* timer)
{
    uint64_t curr = fu_os_get_stmp();
    uint64_t diff = curr - timer->prev;
    timer->prev = curr;
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
    FUTimeoutSource* timeoutSrc = (FUTimeoutSource*)src;
    uint64_t curr = fu_os_get_stmp();

    if (curr < timeoutSrc->prev + timeoutSrc->dur)
        return false;
    timeoutSrc->prev = curr;
    return true;
}

FUSource* fu_timeout_source_new_microseconds(unsigned dur)
{
    static const FUSourceFuncs fns = {
        .check = fu_timeout_source_check
    };
    FUTimeoutSource* src = (FUTimeoutSource*)fu_source_init(fu_object_new(fu_timeout_source_get_type()), &fns, NULL);
    src->prev = fu_os_get_stmp();
    src->dur = dur;
    return (FUSource*)src;
}

char* fu_get_current_time_utc()
{
    struct timespec ts;
    char* rev = fu_malloc0(DEF_TIMER_FORMAT_BUFF_LEN);
    timespec_get(&ts, TIME_UTC);
    strftime(rev, DEF_TIMER_FORMAT_BUFF_LEN, "%F %T", gmtime(&ts.tv_sec));
    return rev;
}

char* fu_get_current_time_utc_format(const char* format)
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