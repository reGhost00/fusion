/**
 * @file vfs.window.h
 * @author your name (you@domain.com)
 * @brief window 下的虚拟文件系统，文件处理相关操作
 * @version 0.1
 * @date 2024-10-07
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "../core/types.h"
#ifdef FU_OS_WINDOW
#ifndef _TVFS_WINDOW_H_
#define _TVFS_WINDOW_H_
#include <windows.h>

#include "../core/file.h"
#include "../core/utils.h"

typedef HANDLE TVFSHwnd;
typedef DWORD TVFSErrorCode;
typedef struct _TVFSArgs TVFSArgs;
typedef void (*TVFSAsyncCallback)(TVFSArgs* args, TVFSErrorCode error, void* usd);
typedef struct _TOverlapped {
    OVERLAPPED overlapped;
    TVFSArgs* args;
    TVFSAsyncCallback cb;
    void* usd;
} TOverlapped;

struct _TVFSArgs {
    /** (打开)对存在或不存在的文件或设备执行的操作 */
    DWORD flags;
    /** (打开)文件或设备属性和标志 */
    DWORD mode;
    /** (打开)文件或设备 */
    wchar_t* path;
    /** 文件句柄 */
    TVFSHwnd hwnd;

    /** (查询)文件类型 */
    EFUFileType type;

    /** (读写)文件大小 */
    size_t size;
    /** 读缓冲区 */
    void* buffRead;
    /** 写缓冲区 */
    const void* buffWrite;

    // 异步相关参数
    // TVFSAsyncCallback callback;
    TOverlapped* ovd;
    // void* usd;
    // TOverlapped* asd;
};

// typedef bool (*TVFSFunc)(TVFSArgs* args);
// typedef struct _TVFS {
//     TVFSFunc query_type;
//     TVFSFunc open;
//     TVFSFunc read;
//     TVFSFunc read_async;
//     TVFSFunc write;
// } TVFS;

static TVFSArgs* t_vfs_args_new(const char* path)
{
    size_t len;
    TVFSArgs* args = fu_malloc0(sizeof(TVFSArgs));
    args->path = fu_utf8_to_wchar(path, &len);
    args->mode = FILE_ATTRIBUTE_NORMAL;
    return args;
}

static void t_vfs_args_free(TVFSArgs* args)
{
    fu_free(args->path);
    fu_free(args);
}

static bool t_vfs_query_type(TVFSArgs* args)
{
    DWORD attr = GetFileAttributesW(args->path);
    if (INVALID_FILE_ATTRIBUTES != attr) {
        if (FILE_ATTRIBUTE_DIRECTORY == attr)
            args->type = EFU_FILE_TYPE_DIRECTORY;
        // else if (FILE_ATTRIBUTE_NORMAL != attr)
        //     args->type = EFU_FILE_TYPE_OTHER;
        else
            args->type = EFU_FILE_TYPE_REGULAR;
    }
    return true;
}

static bool t_vfs_open(TVFSArgs* args)
{
    bool rev = 0 < (args->hwnd = CreateFileW(args->path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, args->flags, args->mode, NULL));
    if (!rev) {
        fu_winapi_print_error(__func__);
    }

    return rev;
}

static void t_vfs_close(TVFSArgs* args)
{
    if (0 < args->hwnd)
        CloseHandle(args->hwnd);
}

static bool t_vfs_read(TVFSArgs* args)
{
    DWORD bytesRead = 0;
    fu_winapi_return_val_if_fail(ReadFile(args->hwnd, args->buffRead, args->size, &bytesRead, NULL), false);
    args->size = bytesRead;
    return bytesRead;
}
/**
 * @brief 在 window 下异步读取文件
 * 启动异步操作的线程需要调用 t_vfs_async_check 进入可报警等待状态
 * 此时如果异步读取操作已完成，系统将调用在 OVERLAPPED 中的回调函数
 * 这个回调函数在 TVFSArgs 中指定
 * 在此回调函数中需要调用 t_vfs_async_finish 通知主线程异步读取操作完成
 * @param args
 * @return true
 * @return false
 */

static void t_vfs_read_finish(DWORD error, DWORD bytes, LPOVERLAPPED overlapped)
{
    TOverlapped* ovd = (TOverlapped*)overlapped;
    TVFSArgs* args = ovd->args;
    args->size = bytes;
    ovd->cb(args, error, ovd->usd);
}

static bool t_vfs_read_async(TVFSArgs* args, TVFSAsyncCallback cb, void* usd)
{
    TOverlapped* overlapped = args->ovd = fu_malloc0(sizeof(TOverlapped));
    overlapped->overlapped.hEvent = CreateEvent(NULL, true, false, NULL);
    overlapped->args = args;
    overlapped->cb = cb;
    overlapped->usd = usd;
    if (!ReadFileEx(args->hwnd, args->buffRead, args->size, (LPOVERLAPPED)overlapped, t_vfs_read_finish)) {
        CloseHandle(overlapped->overlapped.hEvent);
        fu_free(overlapped);
        return false;
    }
    return true;
}

static bool t_vfs_async_check(TVFSArgs* args)
{
    // return WAIT_OBJECT_0 == WaitForSingleObjectEx(args->ovd->overlapped.hEvent, 10, true);
    DWORD rev = WaitForSingleObjectEx(args->ovd->overlapped.hEvent, 0, true);
    if (FU_UNLIKELY(WAIT_FAILED == rev)) {
        fu_winapi_print_error(__func__);
        return true;
    }
    return WAIT_OBJECT_0 == rev;
}

static void t_vfs_async_finish(TVFSArgs* args)
{
    SetEvent(args->ovd->overlapped.hEvent);
}

static void t_vfs_async_cleanup(TVFSArgs* args)
{
    CloseHandle(args->ovd->overlapped.hEvent);
    fu_free(args->ovd);
}

static bool t_vfs_write(TVFSArgs* args)
{
    DWORD bytesWritten = 0;
    fu_winapi_return_val_if_fail(WriteFile(args->hwnd, args->buffWrite, args->size, &bytesWritten, NULL), false);
    args->size = bytesWritten;
    return bytesWritten;
}

// static void t_vfs_class_init(FUObjectClass* oc) { }

// static TVFS* t_vfs_new()
// {
//     // TVFS* vfs = (TVFS*)fu_object_new(T_TYPE_VFS);
//     TVFS* vfs = (TVFS*)fu_malloc(sizeof(TVFS));
//     vfs->open = t_vfs_open;
//     vfs->query_type = t_vfs_query_type;
//     vfs->read = t_vfs_read;
//     vfs->read_async = t_vfs_read_async;
//     vfs->write = t_vfs_write;
//     return vfs;
// }

// static void t_vfs_free(TVFS* vfs, TFd fd)
// {
//     if (fd)
//         CloseHandle(fd);
//     fu_free(vfs);
// }

#endif // _TVFS_WINDOW_H_
#endif // FU_OS_WINDOW