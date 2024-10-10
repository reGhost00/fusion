#ifndef _FU_FILE_H_
#define _FU_FILE_H_

#include <stdint.h>

#include "array.h"
#include "object.h"
#include "thread.h"
#include "utils.h"

typedef enum _EFUFileType {
    EFU_FILE_TYPE_NOT_EXIST,
    EFU_FILE_TYPE_DIRECTORY,
    EFU_FILE_TYPE_OTHER,
    EFU_FILE_TYPE_REGULAR,
    EFU_FILE_TYPE_CNT
} EFUFileType;

// FU_DECLARE_TYPE(FUStream, fu_stream)
// #define FU_TYPE_STREAM (fu_stream_get_type())

// typedef void* (*FUStreamFunc)(FUStream* stream, void* usd);
// typedef struct _FUStreamNode FUStreamNode;
// struct _FUStreamNode {
//     FUStreamNode* prev;
//     FUStreamNode* next;
//     FUStreamFunc func;
// };

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
#endif // _FU_FILE_H_
