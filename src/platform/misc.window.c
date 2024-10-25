#ifdef FU_OS_WINDOW
#include "core/logger.h"
#include "core/memory.h"
#include "misc.window.inner.h"

char* fu_wchar_to_utf8(const wchar_t* wstr, size_t* len)
{
    if (!(*len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL)))
        return NULL;
    char* str = fu_malloc(*len);
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, *len, NULL, NULL);
    return str;
}

wchar_t* fu_utf8_to_wchar(const char* str, size_t* len)
{
    if (!(*len = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0)))
        return NULL;
    wchar_t* wstr = fu_malloc(*len * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, str, -1, wstr, *len);
    return wstr;
}

struct _FUThread {
    FUObject parent;
    FUThreadFunc func;
    HANDLE hwnd;
    DWORD id;
    void* arg;
    void* rev;
};
FU_DEFINE_TYPE(FUThread, fu_thread, FU_TYPE_OBJECT)

static void fu_thread_finalize(FUObject* obj)
{
    FUThread* th = (FUThread*)obj;
    if (th->hwnd) {
        DWORD lpExitCode = 0;
        if (!GetExitCodeThread(th->hwnd, &lpExitCode) || lpExitCode) {
            fu_warning("Object are going to free but thread %d are still alive. Terminate it.", th->id);
            TerminateThread(th->hwnd, 0);
        }
        CloseHandle(th->hwnd);
    }
}

static void fu_thread_class_init(FUObjectClass* oc)
{
    oc->finalize = fu_thread_finalize;
}

static DWORD WINAPI fu_thread_runner(LPVOID param)
{
    FUThread* th = (FUThread*)param;
    fu_info("Thread %d start", th->id);
    th->rev = th->func(th->arg);
    return 0;
}

FUThread* fu_thread_new(FUThreadFunc func, void* arg)
{
    FUThread* th = (FUThread*)fu_object_new(FU_TYPE_THREAD);
    th->func = func;
    th->arg = arg;
    th->hwnd = CreateThread(NULL, 0, fu_thread_runner, th, 0, &th->id);
    return th;
}

void* fu_thread_join(FUThread* thread)
{
    fu_return_val_if_fail(thread, NULL);
    WaitForSingleObject(thread->hwnd, INFINITE);
    return thread->rev;
}

void fu_thread_detach(FUThread* thread)
{
    fu_return_if_fail(thread);
    CloseHandle(thread->hwnd);
    thread->hwnd = NULL;
}

/** 获取系统时间戳，单位：微秒 */
uint64_t fu_os_get_stmp()
{
    uint64_t rev;
    QueryUnbiasedInterruptTime(&rev);
    return rev / 10ULL;
}

void fu_os_print_error_from_code(const char* prefix, const DWORD code)
{
    wchar_t* strW = NULL;
    size_t len;
    if (FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&strW, 0, NULL)) {
        char* strU = fu_wchar_to_utf8(strW, &len);
        // fprintf(stderr, "%s %ld %s\n", prefix, code, strU);
        fu_error("%s %ld %s\n", prefix, code, strU);
        LocalFree(strW);
        fu_free(strU);
    } else {
        fu_error("%s %ld\n", prefix, code);
    }
    // fprintf(stderr, "%s %ld\n", prefix, code);
}

#endif // FU_OS_WINDOW