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
#pragma once

#ifdef FU_OS_WINDOW
#include <windows.h>
// custom
#include "../core/file.h"
#include "../core/utils.h"

typedef HANDLE TVFSHwnd;
typedef DWORD TVFSErrorCode;
typedef enum _EVFSFlags {
    E_VFS_FLAGS_OPEN_ALWAYS = 0x001,
    E_VFS_FLAGS_CREATE_ALWAYS = 0x010,
    E_VFS_FLAGS_ASYNC = 0x100
} EVFSFlags;

typedef struct _TVFSArgs TVFSArgs;
typedef bool (*TVFSAsyncCallback)(TVFSArgs* args, TVFSErrorCode error, void* usd);
typedef struct _TOverlapped {
    OVERLAPPED overlapped;
    TVFSArgs* args;
    TVFSAsyncCallback cb;
    void* usd;
} TOverlapped;

struct _TVFSArgs {
    /** (打开)对存在或不存在的文件或设备执行的操作 */
    EVFSFlags flags;
    /** (打开)文件或设备 */
    wchar_t* path;
    /** 文件句柄 */
    HANDLE hwnd;

    /** (查询)文件类型 */
    EFUFileType type;

    DWORD lastError;
    /** (读写)文件大小 */
    size_t size;
    size_t offset;
    /** 读缓冲区 */
    void* buffRead;
    /** 写缓冲区 */
    const void* buffWrite;

    // 异步相关参数
    TOverlapped* ovd;
};

static TVFSArgs* t_vfs_args_new(const char* path)
{
    size_t len;
    TVFSArgs* args = fu_malloc0(sizeof(TVFSArgs));
    args->path = fu_utf8_to_wchar(path, &len);
    return args;
}

static void t_vfs_args_free(TVFSArgs* args)
{
    if (args->hwnd)
        CloseHandle(args->hwnd);
    fu_free(args->path);
    fu_free(args);
}

static void t_vfs_query_type(TVFSArgs* args)
{
    DWORD attr = GetFileAttributesW(args->path);
    if (INVALID_FILE_ATTRIBUTES != attr) {
        if (FILE_ATTRIBUTE_DIRECTORY != attr)
            args->type = EFU_FILE_TYPE_REGULAR;
        else
            args->type = EFU_FILE_TYPE_DIRECTORY;
    } else
        args->type = EFU_FILE_TYPE_NOT_EXIST;
    return;
}

static bool t_vfs_open(TVFSArgs* args)
{
    DWORD flags;
    DWORD mode = FILE_ATTRIBUTE_NORMAL;
    if (E_VFS_FLAGS_OPEN_ALWAYS & args->flags)
        flags = OPEN_ALWAYS;
    else if (E_VFS_FLAGS_CREATE_ALWAYS & args->flags)
        flags = CREATE_ALWAYS;
    else
        flags = OPEN_EXISTING;
    if (E_VFS_FLAGS_ASYNC & args->flags)
        mode |= FILE_FLAG_OVERLAPPED;
    if (INVALID_HANDLE_VALUE != (args->hwnd = CreateFileW(args->path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, flags, mode, NULL)))
        return true;
    args->lastError = GetLastError();
    fu_winapi_print_error_from_code(__func__, args->lastError);
    return false;
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

static bool t_vfs_write(TVFSArgs* args)
{
    DWORD bytesWritten = 0;
    fu_winapi_return_val_if_fail(WriteFile(args->hwnd, args->buffWrite, args->size, &bytesWritten, NULL), false);
    args->size = bytesWritten;
    return bytesWritten;
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
//
// 异步操作
// 异步读取指定大小的数据
// 1 调用 CreateFileW 打开文件，参数 flags 包含 FILE_FLAG_OVERLAPPED
//     t_vfs_open(E_VFS_FLAGS_ASYNC)
//
// 2 创建事件对象并保存在 OVERLAPPED.hEvent 中用于等待异步操作完成，调用 ReadFileEx 开始异步读取
//      t_vfs_read_async(args, cb, usd)
//          args->buffRead: 读取缓冲区
//          args->size: 读取大小（缓冲区大小）
//          args->offset: 读取偏移（可选）
//          cb: 当读取成功且线程在可报警状态时调用此函数 （调用 t_vfs_async_check）
//          usd: 用户数据
// 3 用户调用 WaitForSingleObjectEx 使线程进入可报警状态，如何此时读取完成，系统会调用 OVERLAPPED 中的回调函数
//      t_vfs_async_check
// 4 OVERLAPPED 中的回调函数在其它线程中执行。可以调用 SetEvent 通知启动的线程异步读取操作完成

/**
 * @brief 使线程进入可报警等待状态
 * 如果此时有异步操作已经完成，系统会调用 OVERLAPPED 中的回调函数
 * @param args
 * @return true
 * @return false
 */
static bool t_vfs_async_check(TVFSArgs* args)
{
    // return WAIT_OBJECT_0 == WaitForSingleObjectEx(args->ovd->overlapped.hEvent, 10, true);
    DWORD rev = WaitForSingleObjectEx(args->ovd->overlapped.hEvent, 0, true);
    if (FU_UNLIKELY(WAIT_FAILED == rev)) {
        fu_winapi_print_error(__func__);
        return true;
    }
    return !rev;
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

static void t_vfs_read_write_finish(DWORD error, DWORD bytes, LPOVERLAPPED overlapped)
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
    overlapped->overlapped.Offset = args->offset;
    overlapped->args = args;
    overlapped->cb = cb;
    overlapped->usd = usd;
    if (!ReadFileEx(args->hwnd, args->buffRead, args->size, (LPOVERLAPPED)overlapped, t_vfs_read_write_finish)) {
        CloseHandle(overlapped->overlapped.hEvent);
        fu_free(overlapped);
        return false;
    }
    return true;
}

static bool t_vfs_read_async_continue(TVFSArgs* args)
{
    TOverlapped* overlapped = args->ovd;
    args->offset += args->size;
    overlapped->overlapped.Offset = args->offset;
    if (!ReadFileEx(args->hwnd, args->buffRead, args->size, (LPOVERLAPPED)overlapped, t_vfs_read_write_finish)) {
        CloseHandle(overlapped->overlapped.hEvent);
        fu_free(overlapped);
        return false;
    }
    return true;
}

/** 异步写入到文件末尾 */
static bool t_vfs_write_async(TVFSArgs* args, TVFSAsyncCallback cb, void* usd)
{
    TOverlapped* overlapped = args->ovd = fu_malloc0(sizeof(TOverlapped));
    overlapped->overlapped.hEvent = CreateEvent(NULL, true, false, NULL);
    overlapped->overlapped.Offset = 0xffffffff;
    overlapped->overlapped.OffsetHigh = 0xffffffff;
    overlapped->args = args;
    overlapped->cb = cb;
    overlapped->usd = usd;
    if (!WriteFileEx(args->hwnd, args->buffWrite, args->size, (LPOVERLAPPED)overlapped, t_vfs_read_write_finish)) {
        CloseHandle(overlapped->overlapped.hEvent);
        fu_free(overlapped);
        return false;
    }
    return true;
}

#endif // FU_OS_WINDOW