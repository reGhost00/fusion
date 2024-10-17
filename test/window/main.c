#include <assert.h>
#include <locale.h>
#include <string.h>
// thrid party
#include <fusion.h>

// #include <stdio.h>
// #include <tchar.h>
// #include <windows.h>

typedef struct _TApp {
    FUApp parent;
    FUWindow* window;
    FUFile* file;
    uint32_t max;
} TApp;
FU_DEFINE_TYPE(TApp, t_app, FU_TYPE_APP)
#define T_TYPE_APP (t_app_get_type())
static char buff[1600];

static void t_app_finalize(TApp* self)
{
    printf("%s\n", __func__);
    fu_object_unref(self->file);
    // fu_object_unref(self->app);
}

static void t_app_class_init(FUObjectClass* oc)
{
    oc->finalize = (FUObjectFunc)t_app_finalize;
}

static TApp* t_app_new()
{
    TApp* app = (TApp*)fu_app_init(fu_object_new(T_TYPE_APP));
    app->file = fu_file_new_for_path("D:\\Documents\\test.txt");
    // TApp* app = (TApp*)fu_object_new(T_TYPE_APP);
    // app->app = fu_app_new();
    // app->rand = fu_rand_new(EFU_RAND_ALGORITHM_DEFAULT);
    // app->max = 20;
    return app;
}

static bool app_close_callback(FUWindow* win, TApp* usd)
{
    printf("%s\n", __func__);
    fu_object_unref(win);
    return false;
}

static bool app_resize_callback(FUWindow* win, const FUSize* size, TApp* usd)
{
    assert(win == usd->window);
    sprintf(buff, "%s %u %u\n", __func__, size->width, size->height);
    fu_file_write(usd->file, buff, strlen(buff));
    return true;
}

// static bool app_scale_callback(FUWindow* win, const FUVec2* scale, TApp* usd)
// {
//     assert(win == usd->window);
//     sprintf(buff, "%s %f %f\n", __func__, scale->x, scale->y);
//     fu_file_write(usd->file, buff, strlen(buff));
//     return true;
// }

static bool app_focus_callback(FUWindow* win, const bool* focus, TApp* usd)
{
    assert(win == usd->window);
    sprintf(buff, "%s %s\n", __func__, *focus ? "true" : "false");
    printf("%s\n", buff);
    fu_file_write(usd->file, buff, strlen(buff));
    return true;
}

static bool app_key_down_callback(FUWindow* win, const FUKeyboardEvent* ev, TApp* usd)
{
    assert(win == usd->window);
    sprintf(buff, "%s %s %d %d\n", __func__, ev->name, ev->key, ev->scanCode);
    printf("%s\n", buff);
    fu_file_write(usd->file, buff, strlen(buff));
    return true;
}

static bool app_key_up_callback(FUWindow* win, const FUKeyboardEvent* ev, TApp* usd)
{
    assert(win == usd->window);
    sprintf(buff, "%s %s %d %d\n", __func__, ev->name, ev->key, ev->scanCode);
    printf("%s\n", buff);
    fu_file_write(usd->file, buff, strlen(buff));
    return true;
}

static bool app_key_press_callback(FUWindow* win, const FUKeyboardEvent* ev, TApp* usd)
{
    assert(win == usd->window);
    sprintf(buff, "%s %s %d %d\n", __func__, ev->name, ev->key, ev->scanCode);
    printf("%s\n", buff);
    fu_file_write(usd->file, buff, strlen(buff));
    return true;
}

static bool app_mouse_move_callback(FUWindow* win, const FUMouseEvent* ev, TApp* usd)
{
    assert(win == usd->window);
    sprintf(buff, "%s pos: %d %d\n", __func__, ev->position.x, ev->position.y);
    printf("%s\n", buff);
    fu_file_write(usd->file, buff, strlen(buff));
    return true;
}

static bool app_mouse_up_callback(FUWindow* win, const FUMouseEvent* ev, TApp* usd)
{
    assert(win == usd->window);
    sprintf(buff, "%s pos: %d %f %f\n", __func__, ev->button, ev->position.x, ev->position.y);
    printf("%s\n", buff);
    fu_file_write(usd->file, buff, strlen(buff));
    return true;
}
static bool app_mouse_press_callback(FUWindow* win, const FUMouseEvent* ev, TApp* usd)
{
    assert(win == usd->window);
    sprintf(buff, "%s pos: %d %d %d\n", __func__, ev->button, ev->position.x, ev->position.y);
    printf("%s\n", buff);
    fu_file_write(usd->file, buff, strlen(buff));
    return true;
}
static bool app_mouse_down_callback(FUWindow* win, const FUMouseEvent* ev, TApp* usd)
{
    assert(win == usd->window);
    sprintf(buff, "%s pos: %d %d %d\n", __func__, ev->button, ev->position.x, ev->position.y);
    printf("%s\n", buff);
    fu_file_write(usd->file, buff, strlen(buff));
    return true;
}

static bool app_scroll_callback(FUWindow* win, const FUScrollEvent* ev, TApp* usd)
{
    assert(win == usd->window);
    sprintf(buff, "%s offset: %d %d detal: %d %d\n", __func__, ev->offset.x, ev->offset.y, ev->delta.x, ev->delta.y);
    printf("%s\n", buff);
    fu_file_write(usd->file, buff, strlen(buff));
    return true;
}

static void app_read_callback(FUFileStream* stream, FUAsyncResult* res)
{
    FUError* error = NULL;
    char* str = fu_file_stream_read_finish(stream, res, &error);
    if (error) {
        printf("%s FAILED: %s\n", __func__, error->message);
        fu_error_free(error);
    }
    printf("%s %s\n", __func__, str);
    fu_object_unref(stream);
    fu_free(str);
}

static void app_active_callback(FUObject* obj, void* usd)
{
    // int rav = fu_rand_int_range(usd->rand, 0, 10);

    printf("%s\n", __func__);
    TApp* app = (TApp*)usd;
    FUWindowConfig* cfg = fu_window_config_new(NULL, 640, 320);
    app->window = fu_window_new((FUApp*)app, cfg);

    fu_signal_connect((FUObject*)app->window, "close", (FUSignalCallback0)app_close_callback, app);
    fu_signal_connect_with_param((FUObject*)app->window, "resize", (FUSignalCallback1)app_resize_callback, app);
    fu_signal_connect_with_param((FUObject*)app->window, "focus", (FUSignalCallback1)app_focus_callback, app);
    // fu_signal_connect_with_param((FUObject*)app->window, "scale", (FUSignalCallback1)app_scale_callback, app);
    fu_signal_connect_with_param((FUObject*)app->window, "key-down", (FUSignalCallback1)app_key_down_callback, app);
    fu_signal_connect_with_param((FUObject*)app->window, "key-up", (FUSignalCallback1)app_key_up_callback, app);
    fu_signal_connect_with_param((FUObject*)app->window, "key-press", (FUSignalCallback1)app_key_press_callback, app);
    fu_signal_connect_with_param((FUObject*)app->window, "mouse-move", (FUSignalCallback1)app_mouse_move_callback, app);
    fu_signal_connect_with_param((FUObject*)app->window, "mouse-down", (FUSignalCallback1)app_mouse_down_callback, app);
    fu_signal_connect_with_param((FUObject*)app->window, "mouse-up", (FUSignalCallback1)app_mouse_up_callback, app);
    fu_signal_connect_with_param((FUObject*)app->window, "mouse-press", (FUSignalCallback1)app_mouse_press_callback, app);
    fu_signal_connect_with_param((FUObject*)app->window, "scroll", (FUSignalCallback1)app_scroll_callback, app);
    fu_window_config_free(cfg);
}

static void app_quit_callback(FUObject* obj, void* usd)
{
    printf("%s\n", __func__);
}

int main(int argc, char* argv[])
{
    setlocale(LC_ALL, ".UTF-8");
    TApp* app = t_app_new();
    fu_signal_connect((FUObject*)app, "active", app_active_callback, app);
    fu_signal_connect((FUObject*)app, "quit", app_quit_callback, app);

    // int* uu = NULL;
    // int* aa = fu_steal_pointer(&uu);
    // printf("uu %p %d aa %p %d", uu, *uu, aa, *aa);

    fu_app_run((FUApp*)app);
    fu_object_unref(app);

    printf("bye\n");
    return 0;
}
