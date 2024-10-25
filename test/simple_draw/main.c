
#include <fusion.h>
#include <locale.h>
#include <math.h>

typedef struct _TApp {
    FUApp parent;
    FUWindow* window;
    FUContext2* context;
    FUMatrix2 matrix;
} TApp;
FU_DEFINE_TYPE(TApp, t_app, FU_TYPE_APP)
#define T_TYPE_APP (t_app_get_type())

static void t_app_finalize(FUObject* obj)
{
    TApp* app = (TApp*)obj;
    fu_object_unref(app->context);
}

static void t_app_class_init(FUObjectClass* oc)
{
    oc->finalize = (FUObjectFunc)t_app_finalize;
}
/**
static bool t_app_tick_callback(TApp* usd)
{
    static FURGBA color = { 0x9c, 0x9c, 0x9c, 0xff };
    FUSurface* srf = fu_surface_new(usd->renderer);
    FUContext* ctx = fu_context_new(srf);
    fu_context_path2_start(ctx);
    fu_context_moveto2(ctx, 100, 100);
    fu_context_lineto2(ctx, 100, 200);
    fu_context_lineto2(ctx, 200, 200);
    fu_context_lineto2(ctx, 200, 100);

    color.r += color.g += color.b += 1;
    color.r %= 255;
    color.g %= 255;
    color.b %= 255;
    fu_context_path2_fill(ctx, &color);

    fu_context_path2_start(ctx);
    fu_context_moveto2(ctx, 300, 300);
    fu_context_lineto2(ctx, 300, 400);
    fu_context_lineto2(ctx, 400, 400);
    fu_context_lineto2(ctx, 400, 300);
    fu_context_path2_stroke(ctx, &color);
    fu_context_submit(ctx);

    fu_renderer_clean(usd->renderer);
    fu_surface_draw(srf);

    // draw_triangle();
    fu_renderer_swap(usd->renderer);
    fu_object_unref(ctx);
    fu_object_unref(srf);
    // fu_free(color);
    return usd->ifContinue;
}
*/
static TApp* t_app_new()
{
    return (TApp*)fu_app_init(fu_object_new(T_TYPE_APP));
}

static void app_resize_callback(FUObject* obj, const FUSize* size, TApp* usd)
{
    // fu_surface_update_viewport_size(usd->surface);
}

static bool app_timer_tick_callback(TApp* usd)
{
    static FURGBA color = { 0x7a, 0x9c, 0xae, 0xff };
    static double radians = 0;
    color.r += color.g += color.b += 1;
    color.r %= 255;
    color.g %= 255;
    color.b %= 255;
    fu_matrix_rotate(&usd->matrix, radians = (radians + 0.1) * M_PI / 180);
    // FUContext2* ctx = fu_context2_new(usd->window);
    fu_context_set_color(usd->context, &color);
    fu_context_set_transform2(usd->context, &usd->matrix);
    fu_window_present(usd->window);
    // fu_command_buffer_set_color(usd->cmd, &color);
    // t_surface_present(usd->window);
    // fu_window_remove_context(usd->window, (FUContext*)ctx);
    // fu_object_unref(ctx);
    return true;
}

static void app_draw_triangle(TApp* app)
{
    FURand* rand = fu_rand_new(EFU_RAND_ALGORITHM_DEFAULT);
    FURGBA color = { .a = 0xff };
    color.r = fu_rand_int_range(rand, 0, 0xff);
    color.g = fu_rand_int_range(rand, 0, 0xff);
    color.b = fu_rand_int_range(rand, 0, 0xff);
    FUContext2* ctx = app->context = fu_context2_new(app->window);
    fu_context2_path_start(ctx);
    fu_context2_movetoxy(ctx, 100, 100);
    fu_context2_linetoxy(ctx, 100, 200);
    fu_context2_linetoxy(ctx, 200, 200);
    fu_context2_linetoxy(ctx, 200, 100);
    fu_context2_path_fill(ctx, &color);

    color.r = fu_rand_int_range(rand, 0, 0xff);
    color.g = fu_rand_int_range(rand, 0, 0xff);
    color.b = fu_rand_int_range(rand, 0, 0xff);
    fu_context2_path_start(ctx);
    fu_context2_movetoxy(ctx, 300, 300);
    fu_context2_linetoxy(ctx, 200, 400);
    fu_context2_linetoxy(ctx, 300, 500);
    fu_context2_linetoxy(ctx, 400, 400);
    fu_context2_path_fill(ctx, &color);

    fu_context2_submit(ctx);
    fu_window_push_context(app->window, (FUContext*)ctx);

    fu_object_unref(rand);
}

static bool app_window_close_callback(FUObject* obj, TApp* usd)
{
    printf("%s\n", __func__);
    // fu_object_unref(usd->surface);
    fu_object_unref(obj);
    return false;
}

static void app_active_callback(FUObject* obj, TApp* usd)
{
    printf("%s\n", __func__);
    FUWindowConfig* cfg = fu_window_config_new("测试简单绘图", 800, 600);
    FUSource* tm = fu_timeout_source_new(10);
    usd->window = fu_window_new((FUApp*)usd, cfg);
    fu_matrix_init_identity(&usd->matrix);
    app_draw_triangle(usd);

    fu_signal_connect((FUObject*)usd->window, "close", (FUSignalCallback0)app_window_close_callback, usd);
    fu_signal_connect_with_param((FUObject*)usd->window, "resize", (FUSignalCallback1)app_resize_callback, usd);
    fu_source_set_callback(tm, (FUSourceCallback)app_timer_tick_callback, usd);
    fu_window_take_source(usd->window, &tm);
    fu_window_config_free(cfg);
}

int main()
{
    setlocale(LC_ALL, ".UTF-8");

    TApp* app = t_app_new();
    fu_signal_connect((FUObject*)app, "active", (FUSignalCallback0)app_active_callback, app);

    fu_app_run((FUApp*)app);

    fu_object_unref(app);
    printf("bye");
    return 0;
}
