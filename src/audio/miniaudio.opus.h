/*
This implements a data source that decodes Opus streams via libopus + libopusfile

This object can be plugged into any `ma_data_source_*()` API and can also be used as a custom
decoding backend. See the custom_decoder example.

You need to include this file after miniaudio.h.
*/
#ifdef miniaudio_opus_h
#define miniaudio_opus_h
#include <string.h>
#define MA_NO_OPUS /* Disable the (not yet implemented) built-in Opus decoder to ensure the libopus decoder is picked. */
// #define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include <opusfile.h>

typedef struct _MAOpus {
    ma_data_source_base ds; /* The libopus decoder can be used independently as a data source. */
    ma_read_proc onRead;
    ma_seek_proc onSeek;
    ma_tell_proc onTell;
    ma_format format; /* Will be either f32 or s16. */
    OggOpusFile* file;
    void* usd;
} MAOpus;

static ma_result ma_opus_read_pcm_frames(void*, void*, ma_uint64, ma_uint64*);
static ma_result ma_opus_seek_to_pcm_frame(void*, ma_uint64);
static ma_result ma_opus_get_data_format(void*, ma_format*, ma_uint32*, ma_uint32*, ma_channel*, size_t);
static ma_result ma_opus_get_cursor_in_pcm_frames(void*, ma_uint64*);
static ma_result ma_opus_get_length_in_pcm_frames(void*, ma_uint64*);

static ma_data_source_vtable def_ma_opus_ds_vtable = {
    ma_opus_read_pcm_frames,
    ma_opus_seek_to_pcm_frame,
    ma_opus_get_data_format,
    ma_opus_get_cursor_in_pcm_frames,
    ma_opus_get_length_in_pcm_frames
};

static int ma_opus_of_callback__read(void* usd, unsigned char* buff, int bytesToRead)
{
    size_t bytesRead;
    MAOpus* opus = (MAOpus*)usd;
    if (opus->onRead(opus->usd, buff, bytesToRead, &bytesRead))
        return -1;
    return bytesRead;
}

static int ma_opus_of_callback__seek(void* usd, ogg_int64_t offset, int whence)
{
    ma_seek_origin origin;
    MAOpus* opus = (MAOpus*)usd;
    if (SEEK_SET == whence)
        origin = ma_seek_origin_start;
    else if (SEEK_END == whence)
        origin = ma_seek_origin_end;
    else
        origin = ma_seek_origin_current;

    if (opus->onSeek(opus->usd, offset, origin))
        return -1;
    return 0;
}

static opus_int64 ma_opus_of_callback__tell(void* usd)
{
    ma_int64 cursor;
    MAOpus* opus = (MAOpus*)usd;
    if (opus->onTell(opus->usd, &cursor))
        return -1;

    return cursor;
}

static ma_result ma_opus_init_internal(const ma_decoding_backend_config* config, MAOpus* opus)
{
    ma_data_source_config dataSourceConfig = ma_data_source_config_init();
    dataSourceConfig.vtable = &def_ma_opus_ds_vtable;
    memset(opus, 0, sizeof(MAOpus));
    if (ma_format_f32 == config->preferredFormat || ma_format_s16 == config->preferredFormat)
        opus->format = config->preferredFormat;
    else
        opus->format = ma_format_f32;

    return ma_data_source_init(&dataSourceConfig, &opus->ds);
}

ma_result ma_opus_init(ma_read_proc onRead, ma_seek_proc onSeek, ma_tell_proc onTell, const ma_decoding_backend_config* config, const ma_allocation_callbacks* alloc, MAOpus* opus, void* usd)
{
    ma_result res;
    if ((res = ma_opus_init_internal(config, opus)))
        return res;
    opus->onRead = onRead;
    opus->onSeek = onSeek;
    opus->onTell = onTell;
    opus->usd = usd;

    OpusFileCallbacks opusCallbacks;
    /* We can now initialize the Opus decoder. This must be done after we've set up the callbacks. */
    opusCallbacks.read = ma_opus_of_callback__read;
    opusCallbacks.seek = ma_opus_of_callback__seek;
    opusCallbacks.close = NULL;
    opusCallbacks.tell = ma_opus_of_callback__tell;

    if (!(opus->file = op_open_callbacks(opus, &opusCallbacks, NULL, 0, NULL))) {
        ma_data_source_uninit(&opus->ds);
        memset(opus, 0, sizeof(MAOpus));
        return MA_INVALID_FILE;
    }
    return MA_SUCCESS;
}

ma_result ma_opus_init_file(const char* path, const ma_decoding_backend_config* config, const ma_allocation_callbacks* alloc, MAOpus* opus)
{
    ma_result res;
    if ((res = ma_opus_init_internal(config, opus)))
        return res;
    if (!(opus->file = op_open_file(path, NULL))) {
        ma_data_source_uninit(&opus->ds);
        memset(opus, 0, sizeof(MAOpus));
        return MA_INVALID_FILE;
    }
    return MA_SUCCESS;
}

void ma_opus_uninit(MAOpus* opus, const ma_allocation_callbacks* alloc)
{
    op_free(opus->file);
    ma_data_source_uninit(&opus->ds);
}

ma_result ma_opus_get_data_format(void* usd, ma_format* format, ma_uint32* channels, ma_uint32* sampleRate, ma_channel* channelMap, size_t channelMapCap)
{
    MAOpus* opus = (MAOpus*)usd;
    /* Defaults for safety. */
    if (format)
        *format = opus->format;
    if (channels)
        *channels = 0;
    if (sampleRate)
        *sampleRate = 44100;
    ma_uint32 _channels = op_channel_count(opus->file, -1);
    if (channels)
        *channels = _channels;
    if (channelMap) {
        memset(channelMap, 0, sizeof(ma_channel) * channelMapCap);
        ma_channel_map_init_standard(ma_standard_channel_map_vorbis, channelMap, channelMapCap, _channels);
    }

    return MA_SUCCESS;
}

ma_result ma_opus_read_pcm_frames(void* usd, void* framesOut, ma_uint64 framesCount, ma_uint64* framesRead)
{
    if (!framesCount)
        return MA_INVALID_ARGS;
    if (framesRead)
        *framesRead = 0;
    MAOpus* opus = (MAOpus*)usd;
    ma_result res = MA_SUCCESS; /* Must be initialized to MA_SUCCESS. */
    ma_uint32 channels;
    ma_uint64 totalFramesRead = 0, framesRemaining;
    ma_format format;
    long opusfileRes;
    int framesToRead;
    ma_opus_get_data_format(usd, &format, &channels, NULL, NULL, 0);

    while (totalFramesRead < framesCount) {
        framesRemaining = framesCount - totalFramesRead;
        framesToRead = 1024;
        if (framesToRead > framesRemaining)
            framesToRead = framesRemaining;

        if (ma_format_f32 == format)
            opusfileRes = op_read_float(opus->file, ma_offset_pcm_frames_ptr(framesOut, totalFramesRead, format, channels), framesToRead * channels, NULL);
        else
            opusfileRes = op_read(opus->file, ma_offset_pcm_frames_ptr(framesOut, totalFramesRead, format, channels), framesToRead * channels, NULL);

        if (0 > opusfileRes) {
            res = MA_ERROR; /* Error while decoding. */
            break;
        }
        totalFramesRead += opusfileRes;
        if (!opusfileRes) {
            res = MA_AT_END;
            break;
        }
    }

    if (framesRead)
        *framesRead = totalFramesRead;

    if (!res && !totalFramesRead)
        res = MA_AT_END;

    return res;
}

ma_result ma_opus_seek_to_pcm_frame(void* usd, ma_uint64 frameIndex)
{
    MAOpus* opus = (MAOpus*)usd;
    int opusfileRes = op_pcm_seek(opus->file, (ogg_int64_t)frameIndex);
    if (opusfileRes) {
        if (OP_ENOSEEK == opusfileRes)
            return MA_INVALID_OPERATION; /* Not seekable. */
        if (OP_EINVAL == opusfileRes)
            return MA_INVALID_ARGS;
        return MA_ERROR;
    }
    return MA_SUCCESS;
}

ma_result ma_opus_get_cursor_in_pcm_frames(void* usd, ma_uint64* cursor)
{
    if (!(cursor && usd))
        return MA_INVALID_ARGS;
    MAOpus* opus = (MAOpus*)usd;
    *cursor = 0; /* Safety. */

    ogg_int64_t offset = op_pcm_tell(opus->file);
    if (0 > offset)
        return MA_INVALID_FILE;
    *cursor = offset;
    return MA_SUCCESS;
}

ma_result ma_opus_get_length_in_pcm_frames(void* usd, ma_uint64* len)
{
    if (!(len && usd))
        return MA_INVALID_ARGS;
    MAOpus* opus = (MAOpus*)usd;
    *len = 0; /* Safety. */
    ogg_int64_t _len = op_pcm_total(opus->file, -1);
    if (0 > _len)
        return MA_ERROR;
    *len = _len;
    return MA_SUCCESS;
}

#endif