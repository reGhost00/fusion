/*
This implements a data source that decodes Vorbis streams via libvorbis + libvorbisfile

This object can be plugged into any `ma_data_source_*()` API and can also be used as a custom
decoding backend. See the custom_decoder example.

You need to include this file after miniaudio.h.
*/
#ifdef miniaudio_vorbis_h
#define miniaudio_vorbis_h

#include <string.h>
#ifndef OV_EXCLUDE_STATIC_CALLBACKS
#define OV_EXCLUDE_STATIC_CALLBACKS
#endif
#include "miniaudio.h"
#include <vorbis/vorbisfile.h>

typedef struct _MAVorbis {
    ma_data_source_base ds; /* The libvorbis decoder can be used independently as a data source. */
    ma_read_proc onRead;
    ma_seek_proc onSeek;
    ma_tell_proc onTell;
    ma_format format; /* Will be either f32 or s16. */
    OggVorbis_File file;
    void* usd;
} MAVorbis;

static ma_result ma_vorbis_read_pcm_frames(void*, void*, ma_uint64, ma_uint64*);
static ma_result ma_vorbis_seek_to_pcm_frame(void*, ma_uint64);
static ma_result ma_vorbis_get_data_format(void*, ma_format*, ma_uint32*, ma_uint32*, ma_channel*, size_t);
static ma_result ma_vorbis_get_cursor_in_pcm_frames(void*, ma_uint64*);
static ma_result ma_vorbis_get_length_in_pcm_frames(void*, ma_uint64*);

static ma_data_source_vtable def_ma_vorbis_ds_vtable = {
    ma_vorbis_read_pcm_frames,
    ma_vorbis_seek_to_pcm_frame,
    ma_vorbis_get_data_format,
    ma_vorbis_get_cursor_in_pcm_frames,
    ma_vorbis_get_length_in_pcm_frames
};

static size_t ma_vorbis_vf_callback__read(void* buff, size_t size, size_t count, void* usd)
{
    /* For consistency with fread(). If `size` of `count` is 0, return 0 immediately without changing anything. */
    if (!(size && count))
        return 0;
    size_t bytesRead;
    MAVorbis* vorbis = (MAVorbis*)usd;
    size_t bytesToRead = size * count;
    if (vorbis->onRead(vorbis->usd, buff, bytesToRead, &bytesRead))
        return 0;

    return bytesRead / size;
}

static int ma_vorbis_vf_callback__seek(void* usd, ogg_int64_t offset, int whence)
{
    ma_seek_origin origin;
    MAVorbis* vorbis = (MAVorbis*)usd;
    if (SEEK_SET == whence)
        origin = ma_seek_origin_start;
    else if (SEEK_END == whence)
        origin = ma_seek_origin_end;
    else
        origin = ma_seek_origin_current;

    if (vorbis->onSeek(vorbis->usd, offset, origin))
        return -1;
    return 0;
}

static long ma_vorbis_vf_callback__tell(void* usd)
{
    ma_int64 cursor;
    MAVorbis* vorbis = (MAVorbis*)usd;

    if (vorbis->onTell(vorbis->usd, &cursor))
        return -1;

    return cursor;
}

static ma_result ma_vorbis_init_internal(const ma_decoding_backend_config* config, MAVorbis* vorbis)
{
    ma_data_source_config dataSourceConfig = ma_data_source_config_init();
    dataSourceConfig.vtable = &def_ma_vorbis_ds_vtable;
    memset(vorbis, 0, sizeof(MAVorbis));

    if (ma_format_f32 == config->preferredFormat || ma_format_s16 == config->preferredFormat)
        vorbis->format = config->preferredFormat;
    else
        vorbis->format = ma_format_f32;

    return ma_data_source_init(&dataSourceConfig, &vorbis->ds);
}

ma_result ma_vorbis_init(ma_read_proc onRead, ma_seek_proc onSeek, ma_tell_proc onTell, const ma_decoding_backend_config* config, const ma_allocation_callbacks* alloc, MAVorbis* vorbis, void* usd)
{
    ma_result res;
    if ((res = ma_vorbis_init_internal(config, vorbis)))
        return res;

    vorbis->onRead = onRead;
    vorbis->onSeek = onSeek;
    vorbis->onTell = onTell;
    vorbis->usd = usd;

    ov_callbacks libvorbisCallbacks;

    /* We can now initialize the vorbis decoder. This must be done after we've set up the callbacks. */
    libvorbisCallbacks.read_func = ma_vorbis_vf_callback__read;
    libvorbisCallbacks.seek_func = ma_vorbis_vf_callback__seek;
    libvorbisCallbacks.close_func = NULL;
    libvorbisCallbacks.tell_func = ma_vorbis_vf_callback__tell;
    if (0 > ov_open_callbacks(vorbis, &vorbis->file, NULL, 0, libvorbisCallbacks)) {
        ma_data_source_uninit(&vorbis->ds);
        memset(vorbis, 0, sizeof(MAVorbis));
        return MA_INVALID_FILE;
    }

    return MA_SUCCESS;
}

ma_result ma_vorbis_init_file(const char* path, const ma_decoding_backend_config* config, const ma_allocation_callbacks* alloc, MAVorbis* vorbis)
{
    ma_result res;
    if ((res = ma_vorbis_init_internal(config, vorbis)))
        return res;
    if (0 > ov_fopen(path, &vorbis->file))
        return MA_INVALID_FILE;

    return MA_SUCCESS;
}

void ma_vorbis_uninit(MAVorbis* vorbis, const ma_allocation_callbacks* alloc)
{
    ov_clear(&vorbis->file);
    ma_data_source_uninit(&vorbis->ds);
}

static ma_result ma_vorbis_get_data_format(void* usd, ma_format* format, ma_uint32* channels, ma_uint32* sampleRate, ma_channel* channelMap, size_t channelMapCap)
{
    MAVorbis* vorbis = (MAVorbis*)usd;
    /* Defaults for safety. */
    if (format)
        *format = vorbis->format;
    if (channels)
        *channels = 0;
    if (sampleRate)
        *sampleRate = 44100;
    vorbis_info* info;
    if (!(info = ov_info(&vorbis->file, 0)))
        return MA_INVALID_OPERATION;
    if (channels)
        *channels = info->channels;
    if (sampleRate)
        *sampleRate = info->rate;
    if (channelMap) {
        memset(channelMap, 0, sizeof(ma_channel) * channelMapCap);
        ma_channel_map_init_standard(ma_standard_channel_map_vorbis, channelMap, channelMapCap, info->channels);
    }

    return MA_SUCCESS;
}

static ma_result ma_vorbis_read_pcm_frames(void* usd, void* framesOut, ma_uint64 framesCount, ma_uint64* framesRead)
{
    if (!framesCount)
        return MA_INVALID_ARGS;
    if (framesRead)
        *framesRead = 0;
    /* We always use floating point format. */
    MAVorbis* vorbis = (MAVorbis*)usd;
    ma_result res = MA_SUCCESS; /* Must be initialized to MA_SUCCESS. */
    ma_uint32 channels;
    ma_uint64 totalFramesRead = 0, framesRemaining;
    ma_format format;
    long vorbisfileRes;
    int framesToRead;
    float** ppFramesF32;
    ma_vorbis_get_data_format(usd, &format, &channels, NULL, NULL, 0);

    while (totalFramesRead < framesCount) {
        framesRemaining = framesCount - totalFramesRead;
        framesToRead = 1024;
        if (framesToRead > framesRemaining)
            framesToRead = framesRemaining;

        if (ma_format_f32 == format) {
            if (0 > (vorbisfileRes = ov_read_float(&vorbis->file, &ppFramesF32, framesToRead, NULL))) {
                res = MA_ERROR; /* Error while decoding. */
                break;
            }
            /* Frames need to be interleaved. */
            ma_interleave_pcm_frames(format, channels, vorbisfileRes, (const void**)ppFramesF32, ma_offset_pcm_frames_ptr(framesOut, totalFramesRead, format, channels));
            totalFramesRead += vorbisfileRes;

            if (!vorbisfileRes) {
                res = MA_AT_END;
                break;
            }
        } else {
            if (0 > (vorbisfileRes = ov_read(&vorbis->file, (char*)ma_offset_pcm_frames_ptr(framesOut, totalFramesRead, format, channels), framesToRead * ma_get_bytes_per_frame(format, channels), 0, 2, 1, NULL))) {
                res = MA_ERROR; /* Error while decoding. */
                break;
            }
            /* Conveniently, there's no need to interleaving when using ov_read(). I'm not sure why ov_read_float() is different in that regard... */
            totalFramesRead += vorbisfileRes / ma_get_bytes_per_frame(format, channels);

            if (!vorbisfileRes) {
                res = MA_AT_END;
                break;
            }
        }
    }

    if (framesRead)
        *framesRead = totalFramesRead;

    if (!res && !totalFramesRead)
        res = MA_AT_END;

    return res;
}

static ma_result ma_vorbis_seek_to_pcm_frame(void* usd, ma_uint64 frameIndex)
{
    MAVorbis* vorbis = (MAVorbis*)usd;
    int vorbisfileRes = ov_pcm_seek(&vorbis->file, (ogg_int64_t)frameIndex);
    if (vorbisfileRes) {
        if (OV_ENOSEEK == vorbisfileRes)
            return MA_INVALID_OPERATION; /* Not seekable. */
        if (OV_EINVAL == vorbisfileRes)
            return MA_INVALID_ARGS;
        return MA_ERROR;
    }
    return MA_SUCCESS;
}

static ma_result ma_vorbis_get_cursor_in_pcm_frames(void* usd, ma_uint64* cursor)
{
    if (!(cursor && usd))
        return MA_INVALID_ARGS;
    MAVorbis* vorbis = (MAVorbis*)usd;
    *cursor = 0; /* Safety. */
    ogg_int64_t offset = ov_pcm_tell(&vorbis->file);
    if (offset < 0)
        return MA_INVALID_FILE;
    *cursor = (ma_uint64)offset;
    return MA_SUCCESS;
}

static ma_result ma_vorbis_get_length_in_pcm_frames(void* usd, ma_uint64* len)
{
    if (!(len && usd))
        return MA_INVALID_ARGS;
    // MAVorbis* vorbis = (MAVorbis*)usd;
    *len = 0; /* Safety. */
    /* I don't know how to reliably retrieve the length in frames using libvorbis, so returning 0 for now. */

    return MA_SUCCESS;
}

#endif