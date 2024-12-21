#include "fusion.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// #define _use_futh 1
// #define _use_mutex 1
// #define _use_atomic 1
typedef struct _TData {
    uint32_t val;
#ifdef _use_mutex
    FUMutex mutex;
#endif
} TData;
#ifdef _use_osth
#define furev static DWORD WINAPI
#define threv(rev) 0
#else
#define furev static void*
#define threv(rev) rev
#endif
#ifdef _use_mutex
#define lock_init(x) fu_mutex_init(x)
#define lock_destroy(x) fu_mutex_destroy(x)
#define lock(x) fu_mutex_lock(x)
#define unlock(x) fu_mutex_unlock(x)
#else
#define lock(x)
#define unlock(x)
#define lock_init(x)
#define lock_destroy(x)
// #define lock(x) WaitForSingleObject(x, INFINITE)
// #define unlock(x) ReleaseMutex(x)
#endif

#ifdef _use_atomic
#define _add(r, x, y) (fu_atomic_uint_add(&(x), (y)))
#define _sub(r, x, y) (fu_atomic_uint_sub(&(x), (y)))
#else
#define _add(r, x, y) ((r) = (x) + (y))
#define _sub(r, x, y) ((r) = (x) - (y))
#endif
furev test_calc_add(void* data)
{
    TData* dt = (TData*)data;
    lock(dt->mutex);
    for (uint32_t i = 0; i < 100000; i++) {
        _add(dt->val, dt->val, 2);
        _sub(dt->val, dt->val, 2);
        _add(dt->val, dt->val, 2);
    }
    unlock(dt->mutex);
    return threv(data);
}

furev test_calc_sub(void* data)
{
    TData* dt = (TData*)data;
    lock(dt->mutex);
    for (uint32_t i = 0; i < 200000; i++) {
        _sub(dt->val, dt->val, 1);
        _add(dt->val, dt->val, 1);
        _sub(dt->val, dt->val, 1);
    }
    unlock(dt->mutex);
    return threv(data);
}

int main(int argc, char* argv[])
{
    TData* dt1 = fu_malloc(sizeof(TData));
    dt1->val = 100;
    FUTimer* timer = fu_timer_new();
    fu_timer_start(timer);
#ifdef _use_osth
    HANDLE ths[5];
    ths[0] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)test_calc_add, dt1, 0, NULL);
    ths[1] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)test_calc_sub, dt1, 0, NULL);
    ths[2] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)test_calc_add, dt1, 0, NULL);
    ths[3] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)test_calc_sub, dt1, 0, NULL);
    ths[4] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)test_calc_add, dt1, 0, NULL);

    printf("start\n");
    WaitForMultipleObjects(5, ths, FALSE, INFINITE);
#elif _use_futh
    lock_init(dt1->mutex);
    FUThread* th1 = fu_thread_new((FUThreadFunc)test_calc_add, dt1);
    FUThread* th2 = fu_thread_new((FUThreadFunc)test_calc_sub, dt1);
    FUThread* th3 = fu_thread_new((FUThreadFunc)test_calc_add, dt1);
    FUThread* th4 = fu_thread_new((FUThreadFunc)test_calc_sub, dt1);
    FUThread* th5 = fu_thread_new((FUThreadFunc)test_calc_add, dt1);
    FUThread* th6 = fu_thread_new((FUThreadFunc)test_calc_sub, dt1);
    printf("start\n");
    fu_thread_join(th1);
    fu_thread_join(th2);
    fu_thread_join(th3);
    fu_thread_join(th4);
    fu_thread_join(th5);
    fu_thread_join(th6);
    printf("result: %u time: %lu\n", dt1->val, fu_timer_measure(timer));
    lock_destroy(dt1->mutex);
    fu_object_unref(th1);
    fu_object_unref(th2);
    fu_object_unref(th3);
    fu_object_unref(th4);
    fu_object_unref(th5);
    fu_object_unref(th6);
#else
    // for (uint32_t i = 0; i < 1000000; i++) {
    dt1 = test_calc_add(dt1);
    dt1 = test_calc_sub(dt1);
    dt1 = test_calc_add(dt1);
    dt1 = test_calc_sub(dt1);
    dt1 = test_calc_add(dt1);
    dt1 = test_calc_sub(dt1);
    printf("result: %u time: %lu\n", dt1->val, fu_timer_measure(timer));
// dt2 = test_calc(dt2);
#endif
    fu_object_unref(timer);
    fu_free(dt1);
    printf("bye\n");
    return 0;
}