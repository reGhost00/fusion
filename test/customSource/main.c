#include <assert.h>
#include <locale.h>
#include <stdlib.h>

#include "naett.h"
#include <fusion.h>

typedef enum _EHTTPState {
    E_HTTP_STATE_NONE,
    E_HTTP_STATE_WAITING,
    E_HTTP_STATE_DONE,
    E_HTTP_STATE_CNT
} EHTTPState;

typedef struct _TApp {
    FUMainLoop* loop;
    FUSource* http;
    naettReq* req;
    naettRes* res;
    EHTTPState state;
} TApp;

static bool naet_src_prepare(FUSource* src, void* usd)
{
    // 主循环中的 prepare 阶段
    // 如果检查数据源需要初始化一些资源，在这个函数中初始化
    // 如果返回 false， 表示数据源未准备好，跳过后续阶段
    // 在本示例中没有用，可以省略这个函数
    return true;
}

static bool naet_src_check(FUSource* src, void* usd)
{
    // 主循环的 check 阶段
    // 检查数据源是否应该触发
    // - 返回 true 表示数据源应该触发，将调用绑定在数据源上的回调函数
    // - 返回 false 表示无需触发，跳过后续阶段
    // - 如果没有提供此函数，默认触发
    static int idx = 0;
    static const char* const waiting = "......";
    TApp* app = (TApp*)usd;
    printf("\r正在等待响应%s", waiting + (idx++ % 5));
    return naettComplete(app->res);
}

static bool t_app_src_callback(TApp* usd)
{
    // 数据源的回调函数
    // 如果返回 false，表示不再处理此数据源，下一阶段将从主循环中分离
    // 如果这是主循环中最后一个数据源，分离后主循环自动退出
    int bodyLength = 0;
    const char* body = naettGetBody(usd->res, &bodyLength);

    printf("\nGot %d bytes of type '%s':\n", bodyLength, naettGetHeader(usd->res, "Content-Type"));
    printf("%.100s\n...\n", body);
    return false;
}

static void naet_src_cleanup(FUSource* src, void* usd)
{
    // 主循环的 cleanup 阶段
    // 如果之前初始化了资源，在这个函数中释放
    // 此函数无条件调用
}

static TApp* t_app_new()
{
    static const FUSourceFuncs naettSrcFuncs = {
        .prepare = naet_src_prepare,
        .check = naet_src_check,
        .cleanup = naet_src_cleanup
    };
    TApp* app = fu_malloc0(sizeof(TApp));
    app->loop = fu_main_loop_new();
    app->http = fu_source_new(&naettSrcFuncs, app);
    fu_source_set_callback(app->http, (FUSourceCallback)t_app_src_callback, app);
    fu_source_attach(app->http, app->loop);
    return app;
}

static void t_app_free(TApp* app)
{
    fu_object_unref(app->loop);
    fu_object_unref(app->http);
    fu_free(app);
}

int main(int argc, char* argv[])
{
    naettInit(NULL);
    setlocale(LC_ALL, ".UTF-8");
    TApp* app = t_app_new();
    app->req = naettRequest("http://aider.meizu.com/app/weather/listWeather?cityIds=101280101", naettMethod("GET"), naettHeader("accept", "application/json"));
    app->res = naettMake(app->req);
    app->state = E_HTTP_STATE_WAITING;
    fu_main_loop_run(app->loop);

    t_app_free(app);

    printf("bye\n");
    return 0;
}

// static void app_active_cb(FUObject* obj, void* usd)
// {
//     assert(obj == usd);
//     printf("%s\n", __func__);

// }

// static void app_quit_cb(FUObject* obj, void* usd)
// {
//     assert(obj == usd);
//     printf("%s\n", __func__);
// }

// int main(int argc, char* argv[])
// {
//     setlocale(LC_ALL, ".UTF-8");
//     // srand((unsigned)time(NULL));
//     FUApp* app = fu_app_new();
//     fu_signal_connect((FUObject*)app, "active", app_active_cb, app);
//     fu_signal_connect((FUObject*)app, "quit", app_quit_cb, app);

//     fu_app_run(app);

//     fu_object_unref(app);
//     printf("bye\n");
//     return 0;
// }
