#include <assert.h>
#include <fusion.h>
#include <locale.h>
#include <stdio.h>
#ifdef FU_OS_WINDOW
#include <tchar.h>

typedef struct _TApp {
    FUApp* app;
    FURand* rand;
} TApp;

static TApp* t_app_new()
{
    TApp* app = fu_malloc0(sizeof(TApp));
    app->app = fu_app_new();
    app->rand = fu_rand_new(EFU_RAND_ALGORITHM_MT);
    return app;
}

static void t_app_free(TApp* app)
{
    fu_object_unref(app->app);
    fu_object_unref(app->rand);
    fu_free(app);
}

static void app_gen_rand(FUThread* th, TApp* usd)
{
    char* str = fu_malloc0(1600);
    for (uint32_t i = 0; i < 100; i++) {
        if (!(i % 10))
            sprintf(str, "%s\n", str);
        sprintf(str, "%s%-5u", str, fu_rand_int_range(usd->rand, 0, 99));
    }
    printf("\n thread: \n%s\n", str);
    fu_free(str);
}

static void app_active_callback(FUObject* obj, void* usd)
{
    // int rav = fu_rand_int_range(usd->rand, 0, 10);
    assert((FUApp*)obj == ((TApp*)usd)->app);
    printf("%s\n", __func__);
    FUThread* th = fu_thread_new((FUThreadFunc)app_gen_rand, usd);
    for (uint32_t i = 0; i < 9; i++)
        fu_thread_run(th, NULL, usd);
    fu_thread_pool_wait(NULL);
    fu_object_unref((FUObject*)th);
}

static void app_quit_callback(FUObject* obj, void* usd)
{
    // static int i = 0;
    // unsigned diff = fu_timer_measure(usd->timer);
    // char* str = fu_get_current_time_UTC();
    // int rav = fu_rand_int_range(usd->rand, 0, 30);
    assert((FUApp*)obj == ((TApp*)usd)->app);
    printf("%s\n", __func__);
    // fu_free(str);
    // // if (11 > i++)
    // // return true;
    // return 10 > i++ || rav > 3;
}

int main(int argc, char* argv[])
{
    setlocale(LC_ALL, ".UTF-8");
    TApp* app = t_app_new();
    fu_signal_connect((FUObject*)app->app, "active", app_active_callback, app);
    fu_signal_connect((FUObject*)app->app, "quit", app_quit_callback, app);

    fu_app_run(app->app);

    t_app_free(app);

    printf("bye\n");
    return 0;
}
#else
int main(int argc, char* argv[])
{
    printf("bye\n");
    return 0;
}
#endif