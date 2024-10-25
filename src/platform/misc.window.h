#ifdef FU_OS_WINDOW
#pragma once

#include <stdint.h>
#include <windows.h>
// custom
#include "../core/object.h"

typedef HANDLE FUMutex;
#define fu_mutex_init(m) ((void)(m = CreateMutex(NULL, FALSE, NULL)))
#define fu_mutex_lock(m) WaitForSingleObject(m, INFINITE)
#define fu_mutex_unlock(m) ReleaseMutex(m)
#define fu_mutex_destroy(m) CloseHandle(m)

typedef void* (*FUThreadFunc)(void* usd);
FU_DECLARE_TYPE(FUThread, fu_thread)
#define FU_TYPE_THREAD (fu_thread_get_type())

FUThread* fu_thread_create(FUThreadFunc func, void* arg);
void* fu_thread_join(FUThread* thread);
void fu_thread_detach(FUThread* thread);

uint64_t fu_os_get_stmp();
#endif