#pragma once

#ifdef FU_OS_LINUX
#define _GNU_SOURCE
#include <fcntl.h>
#include <liburing.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
// custom
#include "../core/file.h"
#include "../core/memory.h"
#include "../core/misc.h"

#define DEF_QUEUE_DEPTH 9

typedef int TVFSHwnd;
typedef int TVFSErrorCode;
typedef enum _EVFSFlags {
    E_VFS_FLAGS_OPEN_ALWAYS = 0x001,
    E_VFS_FLAGS_CREATE_ALWAYS = 0x010,
    E_VFS_FLAGS_ASYNC = 0x100
} EVFSFlags;

typedef struct _TVFSArgs TVFSArgs;
typedef bool (*TVFSAsyncCallback)(TVFSArgs* args, TVFSErrorCode error, void* usd);

typedef struct _TOverlapped {
    struct io_uring ring;
    struct io_uring_sqe* sqe;
    struct io_uring_cqe* cqe;
    TVFSArgs* args;
    TVFSAsyncCallback cb;
    int cqeCnt;
    void* usd;
} TOverlapped;

struct _TVFSArgs {
    EVFSFlags flags;
    /** file path */
    char* path;
    /** file handle */
    TVFSHwnd hwnd;
    /** file type */
    EFUFileType type;

    int lastError;

    size_t size;
    size_t offset;
    void* buffRead;
    const void* buffWrite;

    TOverlapped* ovd;
};

static TVFSArgs* t_vfs_args_new(const char* path)
{
    TVFSArgs* args = fu_malloc0(sizeof(TVFSArgs));
    args->path = fu_strdup(path);
    return args;
}

static void t_vfs_args_free(TVFSArgs* args)
{
    fu_free(args->path);
    fu_free(args);
}

static void t_vfs_query_type(TVFSArgs* args)
{
    struct stat st;
    if (stat(args->path, &st)) {
        // perror("stat");
        args->type = EFU_FILE_TYPE_NOT_EXIST;
        return;
    }

    if (S_ISREG(st.st_mode))
        args->type = EFU_FILE_TYPE_REGULAR;
    else if (S_ISDIR(st.st_mode))
        args->type = EFU_FILE_TYPE_DIRECTORY;
    else
        args->type = EFU_FILE_TYPE_OTHER;
}

static bool t_vfs_open(TVFSArgs* args)
{
    int flags = O_RDWR;
    if (E_VFS_FLAGS_CREATE_ALWAYS & args->flags)
        flags |= O_TRUNC | O_CREAT;
    if (0 > (args->hwnd = open(args->path, flags, S_IRUSR | S_IWUSR))) {
        perror("open");
        return false;
    }
    return true;
}

static void t_vfs_close(TVFSArgs* args)
{
    if (0 < args->hwnd)
        close(args->hwnd);
}

static bool t_vfs_read(TVFSArgs* args)
{
    ssize_t bytes = read(args->hwnd, args->buffRead, args->size);
    if (0 > bytes) {
        perror("read");
        return false;
    }
    args->size = bytes;
    return true;
}

static bool t_vfs_async_check(TVFSArgs* args)
{
    int err = 0;
    if (0 > (args->ovd->cqeCnt = io_uring_peek_cqe(&args->ovd->ring, &args->ovd->cqe))) {
        err = -args->ovd->cqeCnt;
        args->size = 0;
    } else if (!args->ovd->cqeCnt)
        args->size = args->ovd->cqe->res;
    if (!args->ovd->cqeCnt) {
        io_uring_cqe_seen(&args->ovd->ring, args->ovd->cqe);
        return args->ovd->cb(args, err, args->ovd->usd);
    }
    return false; //! args->ovd->cqeCnt;
}

#define t_vfs_async_finish(args) ((void)0)
static void t_vfs_async_cleanup(TVFSArgs* args)
{
    io_uring_queue_exit(&args->ovd->ring);
    fu_clear_pointer((void**)&args->ovd, fu_free);
}

static bool t_vfs_async_init(TVFSArgs* args, TVFSAsyncCallback cb, void* usd)
{
    args->ovd = fu_malloc0(sizeof(TOverlapped));
    args->ovd->args = args;
    args->ovd->cb = cb;
    args->ovd->usd = usd;

    if (0 > io_uring_queue_init(DEF_QUEUE_DEPTH, &args->ovd->ring, 0)) {
        perror("io_uring_queue_init");
        fu_free(args->ovd);
        return false;
    }

    if (!(args->ovd->sqe = io_uring_get_sqe(&args->ovd->ring))) {
        perror("io_uring_get_sqe");
        io_uring_queue_exit(&args->ovd->ring);
        fu_free(args->ovd);
        return false;
    }

    return true;
}

static bool t_vfs_read_async(TVFSArgs* args, TVFSAsyncCallback cb, void* usd)
{
    if (!t_vfs_async_init(args, cb, usd))
        return false;

    io_uring_prep_read(args->ovd->sqe, args->hwnd, args->buffRead, args->size, args->offset);
    io_uring_sqe_set_data(args->ovd->sqe, args->ovd);
    if (0 > io_uring_submit(&args->ovd->ring)) {
        perror("io_uring_submit");
        io_uring_queue_exit(&args->ovd->ring);
        fu_free(args->ovd);
        return false;
    }
    return true;
}

static bool t_vfs_read_async_continue(TVFSArgs* args)
{
    args->offset += args->size;
    args->ovd->sqe = io_uring_get_sqe(&args->ovd->ring);
    io_uring_prep_read(args->ovd->sqe, args->hwnd, args->buffRead, args->size, args->offset);
    if (0 > io_uring_submit(&args->ovd->ring)) {
        perror("io_uring_submit");
        io_uring_queue_exit(&args->ovd->ring);
        fu_free(args->ovd);
        return false;
    }
    return true;
}

static bool t_vfs_write(TVFSArgs* args)
{
    ssize_t bytes = write(args->hwnd, args->buffWrite, args->size);
    if (0 > bytes) {
        perror("write");
        return false;
    }
    args->size = bytes;
    return true;
}

static bool t_vfs_write_async(TVFSArgs* args, TVFSAsyncCallback cb, void* usd)
{
    if (!t_vfs_async_init(args, cb, usd))
        return false;

    if (0 > lseek(args->hwnd, 0, SEEK_END)) {
        perror("lseek");
        io_uring_queue_exit(&args->ovd->ring);
        fu_free(args->ovd);
        return false;
    }

    io_uring_prep_write(args->ovd->sqe, args->hwnd, args->buffWrite, args->size, -1);
    io_uring_sqe_set_data(args->ovd->sqe, args->ovd);
    if (0 > io_uring_submit(&args->ovd->ring)) {
        perror("io_uring_submit");
        io_uring_queue_exit(&args->ovd->ring);
        fu_free(args->ovd);
        return false;
    }
    return true;
}
#endif // FU_OS_LINUX