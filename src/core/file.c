/**
 * @file file.c
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2024-10-07
 *
 * @copyright Copyright (c) 2024
 *
 */

#include "../platform/vfs.linux.h"
#include "../platform/vfs.window.h"
#include "file.h"
#include "main.h"
#include "memory.h"
#define DEF_READ_BLOCK_LEN 2'000'000

struct _FUStream {
    FUObject parent;
    FUStreamNode* head;
    FUStreamNode* tail;
};
FU_DEFINE_TYPE(FUStream, fu_stream, FU_TYPE_OBJECT)

static void fu_stream_finalize(FUObject* obj)
{
    FUStream* stream = (FUStream*)obj;
    FUStreamNode* node = stream->head;
    while (node) {
        stream->head = node->next;
        fu_free(node);
        node = stream->head;
    }
}

static void fu_stream_class_init(FUObjectClass* oc)
{
    oc->finalize = fu_stream_finalize;
}

FUStream* fu_stream_new(FUStreamFunc fn)
{
    fu_return_val_if_fail(fn, NULL);
    FUStream* stream = (FUStream*)fu_object_new(FU_TYPE_STREAM);
    stream->tail = stream->head = fu_malloc0(sizeof(FUStreamNode));
    stream->head->func = fn;
    return stream;
}

FUObject* fu_stream_init(FUObject* obj, FUStreamFunc fn)
{
    fu_return_val_if_fail(obj && fn, NULL);
    FUStream* stream = (FUStream*)obj;
    stream->tail = stream->head = fu_malloc0(sizeof(FUStreamNode));
    stream->head->func = fn;
    return obj;
}
// #define fu_stream_init(obj, fnSource) ((typedef obj) fu_stream_init((FUObject*)obj, fnSource))

FUStream* fu_stream_append_node(FUStream* stream, FUStreamFunc fn)
{
    fu_return_val_if_fail(stream && fn, NULL);
    FUStreamNode* tail = fu_malloc0(sizeof(FUStreamNode));
    tail->prev = stream->tail;
    tail->func = fn;
    stream->tail->next = tail;
    stream->tail = tail;
    return stream;
}

FUStream* fu_stream_prepend_node(FUStream* stream, FUStreamFunc fn)
{
    fu_return_val_if_fail(stream && fn, NULL);
    FUStreamNode* head = fu_malloc0(sizeof(FUStreamNode));
    head->next = stream->head;
    head->func = fn;
    stream->head->prev = head;
    stream->head = head;
    return stream;
}

void* fu_stream_run(FUStream* stream, void* usd)
{
    FUStreamNode* node = stream->head;
    while (node) {
        usd = node->func(stream, usd);
        node = node->next;
    }
    return usd;
}

typedef enum _EFileState {
    E_FILE_STATE_NONE,
    E_FILE_STATE_OPENED,
    E_FILE_STATE_OPENED_ASYNC,
    E_FILE_STATE_CNT,
} EFileState;

// typedef enum _EFileOpenMode {
//     E_FILE_OPEN_NEW,
//     E_FILE_OPEN_EXIST,
//     E_FILE_OPEN_CNT,
// } EFileOpenMode;

struct _FUFile {
    FUObject parent;
    TVFSArgs* args;
    // TVFS* vfs;
    TVFSHwnd fd;
    EFileState state;
    EFUFileType type;
    // EFileOpenMode mode;
    // char* path;
};
FU_DEFINE_TYPE(FUFile, fu_file, FU_TYPE_OBJECT)

typedef enum _EFileStreamState {
    E_FILE_STREAM_STATE_NONE,
    E_FILE_STREAM_STATE_PENDING,
    E_FILE_STREAM_STATE_FAILED,
    E_FILE_STREAM_STATE_SUCCESS,
    E_FILE_STREAM_STATE_DONE,
    E_FILE_STREAM_STATE_CNT
} EFileStreamState;

struct _FUFileStream {
    FUObject parent;
    FUFile* file;
    // TOverlapped* overlapped;
    /** 已读取数据 */
    FUByteArray* asyncReaded;
    /** 当前读取位置 */
    // size_t offset;
    EFileStreamState state;
    // TVFSErrorCode lastError;
    FUAsyncReadyCallback callback;
    void* usd;
};
FU_DEFINE_TYPE(FUFileStream, fu_file_stream, FU_TYPE_OBJECT)

static void fu_file_finalize(FUObject* obj)
{
    FUFile* file = (FUFile*)obj;
    t_vfs_args_free(file->args);
    // fu_free(file->path);
}

static void fu_file_class_init(FUObjectClass* oc)
{
    oc->finalize = fu_file_finalize;
}

static inline FUFile* fu_file_new(const char* path)
{
    FUFile* file = (FUFile*)fu_object_new(FU_TYPE_FILE);
    file->args = t_vfs_args_new(path);
    // file->path = fu_strdup(path);

    t_vfs_query_type(file->args);
    file->type = file->args->type;
    return file;
}

static inline void fu_file_check_if_open(FUFile* file)
{
    if (file->state)
        return;
    // file->args->flags = E_FILE_OPEN_EXIST == file->mode ? OPEN_ALWAYS : CREATE_ALWAYS;
    fu_return_if_fail(t_vfs_open(file->args));
    file->state = E_FILE_STATE_OPENED;
}
/**
 * @brief 打开并截断或创建文件
 *
 * @param path
 * @return FUFile*
 */
FUFile* fu_file_new_for_path(const char* path)
{
    FUFile* file = fu_file_new(path);
    file->args->flags = E_VFS_FLAGS_CREATE_ALWAYS;
    return file;
}
/**
 * @brief 打开或创建文件
 *
 * @param path
 * @return FUFile*
 */
FUFile* fu_file_open_for_path(const char* path)
{
    FUFile* file = fu_file_new(path);
    file->args->flags = E_VFS_FLAGS_OPEN_ALWAYS;
    return file;
}

void* fu_file_read(FUFile* file, size_t* size)
{
    fu_return_val_if_fail(file && size && *size, NULL);
    fu_return_val_if_fail_with_message(EFU_FILE_TYPE_REGULAR == file->type, "File is not regular\n", NULL);
    fu_return_val_if_fail_with_message(E_FILE_STATE_OPENED >= file->state, "File has been open by ASYNC mode\n", NULL);
    fu_file_check_if_open(file);
    file->args->buffRead = fu_malloc0(*size);
    file->args->size = *size;
    if (!t_vfs_read(file->args)) {
        *size = 0;
        fu_free(file->args->buffRead);
        return NULL;
    }
    *size = file->args->size;
    return file->args->buffRead;
}

FUBytes* fu_file_read_all(FUFile* file)
{
    fu_return_val_if_fail(file, NULL);
    fu_return_val_if_fail_with_message(EFU_FILE_TYPE_REGULAR == file->type, "File is not regular\n", NULL);
    fu_return_val_if_fail_with_message(E_FILE_STATE_OPENED >= file->state, "File has been open by ASYNC mode\n", NULL);
    FUByteArray* arr = fu_byte_array_new();
    size_t size = DEF_READ_BLOCK_LEN;
    void* dt;
    fu_file_check_if_open(file);
    while ((dt = fu_file_read(file, &size))) {
        fu_byte_array_append(arr, dt, size);
        fu_free(dt);
        if (DEF_READ_BLOCK_LEN > size)
            break;
    }
    return fu_byte_array_free_to_bytes(arr);
}

bool fu_file_write(FUFile* file, const void* data, size_t size)
{
    fu_return_val_if_fail(file && data && size, false);
    fu_return_val_if_fail_with_message(EFU_FILE_TYPE_DIRECTORY != file->type, "File is not regular\n", NULL);
    fu_return_val_if_fail_with_message(E_FILE_STATE_OPENED >= file->state, "File has been open by ASYNC mode\n", NULL);
    fu_file_check_if_open(file);

    file->args->buffWrite = data;
    file->args->size = size;
    if (!t_vfs_write(file->args))
        return false;
    return size == file->args->size;
}

/**
 * Glib设计异步操作回调和_finish函数获取结果的原因:
 * 1 异步安全性。如果直接将结果作为参数传给回调,会暴露内部状态,可能存在线程安全问题。
 * 2 统一接口。通过_finish函数获取结果,提供了一致的使用模式。而不用考虑各个API的回调参数不同。
 * 3 行为一致。如果操作失败,回调中无法知道是否成功。但_finish会返回错误码。
 * 4 资源管理。可能需要通过_finish释放临时资源,比如分配的内存等。
 * 5 延迟响应。允许在回调后进行其他处理,如更新UI,而不立即获取结果。
 * 6 错误处理。能在单一地方集中处理各API的错误,而不是分散在各回调中。
 * 7 可选支持。_finish可以放在不同线程中调用,支持错误通知机制。
 * 所以这种设计可以提高安全性和一致性,同时也带来了灵活性,是Glib规范化异步操作的一种比较好的模式。
 *
 */
/**
 * Window 异步操作
 * 异步操作需要通过 FileStream 执行
 * 1 调用 fu_file_stream_new_from_file 将以异步模式实际打开文件
 * 2 调用 fu_file_stream_read_async 开始异步操作，保存用户回调 callback & usd
 *  2.1 调用 t_vfs_read_async 执行异步操作，设置回调函数 cb
 * 为了使异步操作完成后得到通知，需要线程进入可报警等待状态
 * 创建自定义事件源：
 *  2.2 事件源的 check 阶段调用 t_vfs_async_check(WaitForSingleObjectEx) 检查异步操作是否完成（进入可报警等待状态）
 *      如果事件已完成，系统将调用回调函数 cb
 *      在这个函数里识别操作是否成功，然后调用 t_vfs_async_finish(SetEvent) 通知主线程异步操作完成
 *  2.3 事件源的 check 返回 true，执行 dispatch 阶段
 *      dispatch 中调用用户定义的回调函数 callback，并设置状态为 E_FILE_STREAM_STATE_DONE
 *  3 用户在其回调函数中使用 _finish 获取异步操作的结果
 *  4 事件源的 cleanup 阶段识别状态为 E_FILE_STREAM_STATE_DONE，释放全部资源
 */

static void fu_file_stream_finalize(FUObject* obj)
{
    FUFileStream* stream = (FUFileStream*)obj;
    fu_byte_array_free(stream->asyncReaded, true);
    fu_object_unref(stream->file);
}

static void fu_file_stream_class_init(FUObjectClass* oc)
{
    oc->finalize = fu_file_stream_finalize;
}

FUFileStream* fu_file_stream_new_from_file(FUFile* file)
{
    fu_return_val_if_fail(file, NULL);
    fu_return_val_if_fail_with_message(EFU_FILE_TYPE_REGULAR == file->type, "File is not regular\n", NULL);
    fu_return_val_if_fail_with_message(!file->state, "File has been opened\n", NULL);

    file->args->flags |= E_VFS_FLAGS_ASYNC; // FILE_FLAG_OVERLAPPED;
    fu_file_check_if_open(file);

    FUFileStream* stream = (FUFileStream*)fu_object_new(FU_TYPE_FILE_STREAM);
    stream->file = fu_object_ref(file);
    stream->asyncReaded = fu_byte_array_new();
    return stream;
}

static bool fu_file_stream_callback(TVFSArgs* args, TVFSErrorCode error, void* usd)
{
    // printf("%s\n", __func__);
    FUFileStream* stream = (FUFileStream*)usd;
    args->lastError = error;
    stream->state = !error ? E_FILE_STREAM_STATE_SUCCESS : E_FILE_STREAM_STATE_FAILED;

    t_vfs_async_finish(args);
    return true;
}

static bool fu_file_stream_source_check(FUSource* src, void* usd)
{
    FUFileStream* stream = (FUFileStream*)usd;
    return t_vfs_async_check((TVFSArgs*)stream->file->args);
}

static bool fu_file_stream_source_dispatch(void* usd)
{
    FUFileStream* stream = (FUFileStream*)usd;
    if (FU_LIKELY(stream->callback))
        stream->callback((FUObject*)stream, (FUAsyncResult*)stream->file->args, stream->usd);
    stream->state = E_FILE_STREAM_STATE_DONE;
    return false;
}

static void fu_file_stream_source_cleanup(FUSource* src, void* usd)
{
    FUFileStream* stream = (FUFileStream*)usd;
    if (E_FILE_STREAM_STATE_DONE != stream->state)
        return;

    // printf("%s\n", __func__);
    t_vfs_async_cleanup(stream->file->args);
    fu_free(stream->file->args->buffRead);
    fu_object_unref(stream);
    fu_object_unref(src);
}

bool fu_file_stream_read_async(FUFileStream* fileStream, size_t size, FUAsyncReadyCallback callback, void* usd)
{
    static const FUSourceFuncs defAsyncSourceFuncs = {
        .check = fu_file_stream_source_check,
        .cleanup = fu_file_stream_source_cleanup
    };
    fu_return_val_if_fail(fileStream, false);
    if (FU_UNLIKELY(0 >= size || fileStream->state))
        return false;
    TVFSArgs* args = fileStream->file->args;
    args->buffRead = fu_malloc0(size);
    args->size = size;

    fileStream->callback = callback;
    fileStream->usd = usd;
    fu_os_return_val_if_fail(t_vfs_read_async(args, fu_file_stream_callback, fileStream), false);

    // 自动清理
    FUSource* src = fu_source_new(&defAsyncSourceFuncs, fileStream);
    fu_source_set_callback(src, fu_file_stream_source_dispatch, fu_object_ref(fileStream));
    fu_source_attach(src, NULL);
    fileStream->state = E_FILE_STREAM_STATE_PENDING;
    return true;
}

void* fu_file_stream_read_finish(FUFileStream* fileStream, FUAsyncResult* res, FUError** error)
{
    TVFSArgs* args = (TVFSArgs*)res;
    if (E_FILE_STREAM_STATE_SUCCESS != fileStream->state) {
        if (error)
            *error = fu_error_new_from_code(args->lastError);
        return NULL;
    }
    return fu_memdup(args->buffRead, args->size);
}

static bool fu_file_stream_read_all_callback(TVFSArgs* args, TVFSErrorCode error, void* usd)
{
    FUFileStream* stream = (FUFileStream*)usd;
    if (error) {
        args->lastError = error;
        stream->state = E_FILE_STREAM_STATE_FAILED;
        t_vfs_async_finish(args);
        return false;
    }

    fu_byte_array_append(stream->asyncReaded, args->buffRead, args->size);

    // read finish
    if (DEF_READ_BLOCK_LEN > args->size) {
        stream->state = E_FILE_STREAM_STATE_SUCCESS;
        t_vfs_async_finish(args);
        return true;
    }

    return !t_vfs_read_async_continue(args);
}

static bool fu_file_stream_read_all_source_check(FUSource* src, void* usd)
{
    FUFileStream* stream = (FUFileStream*)usd;
    return t_vfs_async_check((TVFSArgs*)stream->file->args);
}

// static bool fu_file_stream_read_all_source_dispatch(void* usd)
// {
//     FUFileStream* stream = (FUFileStream*)usd;
//     stream->callback((FUObject*)stream, (FUAsyncResult*)stream->file->args, stream->usd);
//     stream->state = E_FILE_STREAM_STATE_DONE;
//     return false;
// }

bool fu_file_stream_read_all_async(FUFileStream* fileStream, FUAsyncReadyCallback callback, void* usd)
{
    static const FUSourceFuncs defAsyncSourceFuncs = {
        .check = fu_file_stream_read_all_source_check,
        .cleanup = fu_file_stream_source_cleanup
    };
    fu_return_val_if_fail(fileStream, false);
    TVFSArgs* args = fileStream->file->args;
    args->buffRead = fu_malloc0(DEF_READ_BLOCK_LEN);
    args->size = DEF_READ_BLOCK_LEN;

    fileStream->callback = callback;
    fileStream->usd = usd;
    fu_os_return_val_if_fail(t_vfs_read_async(args, fu_file_stream_read_all_callback, fileStream), false);

    // 自动清理
    FUSource* src = fu_source_new(&defAsyncSourceFuncs, fileStream);
    fu_source_set_callback(src, fu_file_stream_source_dispatch, fu_object_ref(fileStream));
    fu_source_attach(src, NULL);
    fileStream->state = E_FILE_STREAM_STATE_PENDING;
    return true;
}

FUBytes* fu_file_stream_read_all_finish(FUFileStream* fileStream, FUAsyncResult* res, FUError** error)
{
    TVFSArgs* args = (TVFSArgs*)res;
    if (E_FILE_STREAM_STATE_SUCCESS != fileStream->state) {
        if (error)
            *error = fu_error_new_from_code(args->lastError);
        return NULL;
    }

    return fu_byte_array_to_bytes(fileStream->asyncReaded);
    // return fu_by(args->buffRead, args->size);
}

static void fu_file_stream_write_source_cleanup(FUSource* src, void* usd)
{
    FUFileStream* stream = (FUFileStream*)usd;
    if (E_FILE_STREAM_STATE_DONE != stream->state)
        return;

    t_vfs_async_cleanup(stream->file->args);
    fu_object_unref(stream);
    fu_object_unref(src);
}

/** 由于跨平台设计，目前只支持写入到末尾 */
bool fu_file_stream_write_async(FUFileStream* fileStream, const void* data, size_t size, FUAsyncReadyCallback callback, void* usd)
{
    static const FUSourceFuncs defAsyncSourceFuncs = {
        .check = fu_file_stream_source_check,
        .cleanup = fu_file_stream_write_source_cleanup
    };
    fu_return_val_if_fail(fileStream, false);
    if (FU_UNLIKELY(0 >= size || fileStream->state))
        return false;

    TVFSArgs* args = fileStream->file->args;
    args->buffWrite = data;
    args->size = size;
    fileStream->callback = callback;
    fileStream->usd = usd;

    fu_os_return_val_if_fail(t_vfs_write_async(args, fu_file_stream_callback, fileStream), false);

    // 自动清理
    FUSource* src = fu_source_new(&defAsyncSourceFuncs, fileStream);
    fu_source_set_callback(src, fu_file_stream_source_dispatch, fu_object_ref(fileStream));
    fu_source_attach(src, NULL);
    fileStream->state = E_FILE_STREAM_STATE_PENDING;
    return true;
}

size_t fu_file_stream_write_finish(FUFileStream* fileStream, FUAsyncResult* res, FUError** error)
{
    TVFSArgs* args = (TVFSArgs*)res;
    if (E_FILE_STREAM_STATE_SUCCESS != fileStream->state) {
        if (error)
            *error = fu_error_new_from_code(args->lastError);
        return 0;
    }
    return args->size;
}