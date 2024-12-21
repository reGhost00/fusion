#ifdef FU_OS_WINDOW
#pragma once
#include <windows.h>
// custom
#include "core/object.h"

//
// 字符编码相关
// window 专有
#ifdef __cplusplus
extern "C" {
#endif
char* fu_wchar_to_utf8(const wchar_t* wstr, size_t* len);
wchar_t* fu_utf8_to_wchar(const char* str, size_t* len);

typedef HANDLE FUMutex;
#define fu_mutex_lock(m) WaitForSingleObject(m, INFINITE)
#define fu_mutex_unlock(m) ReleaseMutex(m)
#define fu_mutex_destroy(m) CloseHandle(m)
#define fu_mutex_init(mtx) (mtx = CreateMutex(NULL, FALSE, NULL))

typedef void* (*FUThreadFunc)(void* usd);
FU_DECLARE_TYPE(FUThread, fu_thread)
#define FU_TYPE_THREAD (fu_thread_get_type())

FUThread* fu_thread_new(FUThreadFunc func, void* arg);
void* fu_thread_join(FUThread* thread);
void fu_thread_detach(FUThread* thread);

uint64_t fu_os_get_stmp();
#ifdef __cplusplus
}
#endif
#endif