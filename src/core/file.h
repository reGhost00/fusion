#ifndef _FU_FILE_H_
#define _FU_FILE_H_

#include <stdint.h>

#include "array.h"
#include "object.h"
// #include "thread.h"
#include "misc.h"

typedef enum _EFUFileType {
    EFU_FILE_TYPE_NOT_EXIST,
    EFU_FILE_TYPE_DIRECTORY,
    EFU_FILE_TYPE_OTHER,
    EFU_FILE_TYPE_REGULAR,
    EFU_FILE_TYPE_CNT
} EFUFileType;

FU_DECLARE_TYPE(FUStream, fu_stream)
#define FU_TYPE_STREAM (fu_stream_get_type())

typedef void* (*FUStreamFunc)(FUStream* stream, void* usd);
typedef struct _FUStreamNode FUStreamNode;
struct _FUStreamNode {
    FUStreamNode* prev;
    FUStreamNode* next;
    FUStreamFunc func;
};

FUStream* fu_stream_new(FUStreamFunc fn);
FUObject* fu_stream_init(FUObject* obj, FUStreamFunc fn);
FUStream* fu_stream_append_node(FUStream* stream, FUStreamFunc fn);
FUStream* fu_stream_prepend_node(FUStream* stream, FUStreamFunc fn);
void* fu_stream_run(FUStream* stream, void* usd);

FU_DECLARE_TYPE(FUFileStream, fu_file_stream)
#define FU_TYPE_FILE_STREAM (fu_file_stream_get_type())

FU_DECLARE_TYPE(FUFile, fu_file)
#define FU_TYPE_FILE (fu_file_get_type())

FUFile* fu_file_new_for_path(const char* path);
FUFile* fu_file_open_for_path(const char* path);

bool fu_file_write(FUFile* file, const void* data, size_t size);
void* fu_file_read(FUFile* file, size_t* size);
FUBytes* fu_file_read_all(FUFile* file);

FUFileStream* fu_file_stream_new_from_file(FUFile* file);
bool fu_file_stream_read_async(FUFileStream* fileStream, size_t size, FUAsyncReadyCallback cb, void* usd);
void* fu_file_stream_read_finish(FUFileStream* fileStream, FUAsyncResult* res, FUError** error);

bool fu_file_stream_read_all_async(FUFileStream* fileStream, FUAsyncReadyCallback cb, void* usd);
FUBytes* fu_file_stream_read_all_finish(FUFileStream* fileStream, FUAsyncResult* res, FUError** error);

bool fu_file_stream_write_async(FUFileStream* fileStream, const void* data, size_t size, FUAsyncReadyCallback callback, void* usd);
size_t fu_file_stream_write_finish(FUFileStream* fileStream, FUAsyncResult* res, FUError** error);
#endif // _FU_FILE_H_
