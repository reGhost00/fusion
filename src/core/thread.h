#pragma once

#include "object.h"

#ifdef FU_OS_WINDOW
#include <windows.h>
#define callback_env_t TP_CALLBACK_ENVIRON
typedef HANDLE FUMutex;

#define fu_mutex_init(m) ((void)(m = CreateMutex(NULL, FALSE, NULL)))
#define fu_mutex_lock(m) WaitForSingleObject(m, INFINITE)
#define fu_mutex_unlock(m) ReleaseMutex(m)
#define fu_mutex_destroy(m) CloseHandle(m)
#else
#include <pthread.h>
typedef pthread_mutex_t FUMutex;
#define fu_mutex_init(m) pthread_mutex_init(&m, NULL)
#define fu_mutex_lock(m) pthread_mutex_lock(&m)
#define fu_mutex_unlock(m) pthread_mutex_unlock(&m)
#define fu_mutex_destroy(m) pthread_mutex_destroy(&m)
#endif

typedef struct _FUAsyncResult FUAsyncResult;
typedef void (*FUAsyncReadyCallback)(FUObject* obj, FUAsyncResult* res, void* usd);

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
