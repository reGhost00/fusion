#ifdef FU_OS_LINUX
#pragma once
#include <pthread.h>
#include <stdint.h>

// custom
#include "core/object.h"

typedef pthread_mutex_t FUMutex;
#define fu_mutex_init(m) pthread_mutex_init(&m, NULL)
#define fu_mutex_lock(m) pthread_mutex_lock(&m)
#define fu_mutex_unlock(m) pthread_mutex_unlock(&m)
#define fu_mutex_destroy(m) pthread_mutex_destroy(&m)

typedef void* (*FUThreadFunc)(void* usd);
FU_DECLARE_TYPE(FUThread, fu_thread)
#define FU_TYPE_THREAD (fu_thread_get_type())

FUThread* fu_thread_create(FUThreadFunc func, void* arg);
void* fu_thread_join(FUThread* thread);
void fu_thread_detach(FUThread* thread);

uint64_t fu_os_get_stmp();
#endif