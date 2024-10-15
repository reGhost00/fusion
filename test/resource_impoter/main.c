#include <fusion.h>

#define  DEF_FILE "/home/warmsnow/shared-drives/D:/dev/Game/GameBox_cairo/src/generic/miniaudio_opus.h"
typedef struct _TApp {
    FUMainLoop* loop;
    FUTimer* timer;
} TApp;

TApp* t_app_new()
{
    TApp* app = fu_malloc0(sizeof(TApp));
    app->loop = fu_main_loop_new();
    return app;
}

void t_app_free(TApp* app)
{
    fu_object_unref(app->loop);
    fu_free(app);
}

static void read_file_sync(TApp* app) {
    FUFile* file = fu_file_open_for_path(DEF_FILE);
    app->timer = fu_timer_new();
    fu_timer_start(app->timer);
    
    size_t size;
    FUBytes* rev = fu_file_read_all(file);
    char* str = fu_bytes_unref_to_data(rev, &size);
    printf("sync read finish: %lu\n", fu_timer_measure(app->timer));
    // printf("%s\n", str);

// size_t size;
//     char* str = fu_bytes_unref_to_data(rev, &size);

    // FUFile* f2 = fu_file_new_for_path("/home/warmsnow/文档/fusion/src\\asset\\res.h");
    // fu_file_write(f2, str, size);

    fu_free(str);
    fu_object_unref(app->timer);
    fu_object_unref(file);
    // fu_object_unref(f2);
}


static void app_read_finish(FUFileStream* stream, FUAsyncResult* res , void* usd) {
    TApp* app = (TApp*)usd;
    FUError* error = NULL;
    FUBytes* rev = fu_file_stream_read_all_finish(stream, res, &error);
    printf("async read finish: %lu\n", fu_timer_measure(app->timer));
    if (error) {
        printf("%s FAILED: %s\n", __func__, error->message);
        fu_error_free(error);
    }
    size_t size;
    char* str = fu_bytes_unref_to_data(rev, &size);

    FUFile* f2 = fu_file_open_for_path("/home/warmsnow/文档/fusion/src\\asset\\res.h");
    fu_file_write(f2, str, size);

    fu_object_unref(app->timer);
    fu_object_unref(stream);
    fu_object_unref(f2);
    fu_free(str);
}

static void read_file_async(TApp* app) {
    FUFile* file = fu_file_open_for_path(DEF_FILE);
    FUFileStream* stream = fu_file_stream_new_from_file(file);
    if (!fu_file_stream_read_all_async(stream, (FUAsyncReadyCallback)app_read_finish, app)) {
        printf("async read failed\n");
        fu_object_unref(stream);
        fu_object_unref(file);
        return;
    }
    app->timer = fu_timer_new();
    fu_timer_start(app->timer);
    // size_t size;
    // // char* str = fu_bytes_unref_to_data(rev, &size);
    // printf("async read finish: %lu\n", fu_timer_measure(app->timer));
    // // printf("%s\n", str);
    // fu_free(str);
    // fu_object_unref(app->timer);
    fu_object_unref(file);
}

int main(int argc, char* argv[])
{
    TApp* app = t_app_new();
    read_file_sync(app);
    read_file_async(app);
    fu_main_loop_run(app->loop);
    t_app_free(app);
    printf("bye\n");
    return 0;
}