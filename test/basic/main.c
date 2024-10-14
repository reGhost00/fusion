#include <fusion.h>
#include <locale.h>
#include <stdio.h>
#ifdef FU_OS_WINDOW
#include <tchar.h>
#include <windows.h>
#endif
typedef struct _TApp {
    FUObject parent;
    FUMainLoop* loop;
    FUTimer* timer;
    FURand* rand;
    FUSource* tm1;
    FUSource* tm2;
} TApp;
FU_DEFINE_TYPE(TApp, t_app, FU_TYPE_OBJECT)
#define T_TYPE_APP (t_app_get_type())

static void t_app_dispose(TApp* self)
{
    printf("%s\n", __func__);
    fu_object_unref(self->rand);
    fu_object_unref(self->tm1);
    fu_object_unref(self->tm2);
    fu_object_unref(self->timer);
    fu_object_unref(self->loop);
}

static void t_app_class_init(FUObjectClass* oc)
{
    oc->dispose = (FUObjectFunc)t_app_dispose;
}

static TApp* t_app_new()
{
    TApp* app = (TApp*)fu_object_new(T_TYPE_APP);
    app->loop = fu_main_loop_new();
    app->rand = fu_rand_new(1);
    app->timer = fu_timer_new();
    app->tm1 = fu_timeout_source_new(1000);
    app->tm2 = fu_timeout_source_new(500);
    return app;
}

static bool app_tm1_callback(TApp* usd)
{
    static int i = 0;
    unsigned diff = fu_timer_measure(usd->timer);
    char* str = fu_get_current_time_UTC();
    int rav = fu_rand_int_range(usd->rand, 0, 30);
    printf("%s %d, %d\n", str, diff, rav);
    fu_free(str);
    // if (11 > i++)
    // return true;
    return 10 > i++ || rav > 3;
}

static bool app_tm2_callback(TApp* usd)
{

    int rav = fu_rand_int_range(usd->rand, 0, 10);
    printf("%s %d\n", __func__, rav);
    return rav > 3;
}

int main(int argc, char* argv[])
{
    setlocale(LC_ALL, ".UTF-8");
    TApp* app = t_app_new();
    fu_source_set_callback(app->tm1, (FUSourceCallback)app_tm1_callback, app);
    fu_source_set_callback(app->tm2, (FUSourceCallback)app_tm2_callback, app);
    fu_source_attach(app->tm1, app->loop);
    fu_source_attach(app->tm2, app->loop);
    fu_timer_start(app->timer);

    fu_main_loop_run(app->loop);
    fu_object_unref(app);
    printf("bye\n");
    return 0;
}
