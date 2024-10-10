#ifndef __FU_THREAD_H__
#define __FU_THREAD_H__

#include "object.h"
#include "types.h"

#ifdef FU_OS_WINDOW
#include <windows.h>
#define callback_env_t TP_CALLBACK_ENVIRON
#else
#define FU_THREAD_NOT_SUPPORT
#endif

typedef struct _FUAsyncResult FUAsyncResult;
typedef void (*FUAsyncReadyCallback)(FUObject* obj, FUAsyncResult* res, void* usd);

typedef struct _FUMutex {
    HANDLE handle;
} FUMutex;

FU_DECLARE_TYPE(FUThread, fu_thread)
#define FU_TYPE_THREAD (fu_thread_get_type())
typedef void (*FUThreadFunc)(FUThread* thread, void* usd);

FU_DECLARE_TYPE(FUThreadPool, fu_thread_pool)
#define FU_TYPE_THREAD_POOL (fu_thread_pool_get_type())

FU_DECLARE_TYPE(FUTask, fu_task_pool)
#define FU_TYPE_TASK (fu_task_get_type())
typedef char* (*FUTaskFunc)(FUTask* task, void* usd, void* res);
//
// mutex
#define fu_mutex_init(m)                              \
    do {                                              \
        (m)->handle = CreateMutex(NULL, FALSE, NULL); \
    } while (0)
#define fu_mutex_lock(m) WaitForSingleObject((m)->handle, INFINITE)
#define fu_mutex_unlock(m) ReleaseMutex((m)->handle)
#define fu_mutex_destroy(m) CloseHandle((m)->handle)
//
// thread
bool fu_thread_new_run(FUThreadPool* pool, FUThreadFunc fn, void* usd);
FUThread* fu_thread_new(FUThreadFunc fn, void* usd);
bool fu_thread_run(FUThread* thread, FUThreadPool* pool, void* usd);
//
// thread pool
FUThreadPool* fu_thread_pool_new();
FUThreadPool* fu_thread_pool_get_default();
void fu_thread_pool_wait(FUThreadPool* pool);
// //
// // task
// FUTask* fu_task_new(void* host, FUAsyncReadyCallback callback, void* usd);
// void fu_task_set_finish_data(FUTask* task, void* data);
// void fu_task_run(FUTask* task, FUTaskFunc fn, void* usd);
// void* fu_task_run_finish(FUAsyncResult* res, char** msg);
#ifdef FU_OS_WINDOW
callback_env_t* fu_thread_pool_get_callback_env(FUThreadPool* pool);
#endif

#endif