#include "core/hash2.h"
#include <fusion.h>
#include <locale.h>

typedef struct _TApp {
    FUApp parent;
    FURand* rand;
    uint32_t max;
} TApp;
FU_DEFINE_TYPE(TApp, t_app, FU_TYPE_APP)
#define T_TYPE_APP (t_app_get_type())

static bool t_app_timeout_callback(TApp* usd)
{
    int rav = fu_rand_int_range(usd->rand, 0, usd->max--);
    printf("%s %d\n", __func__, rav);
    return rav > 3;
}

static void t_app_finalize(TApp* self)
{
    printf("%s\n", __func__);
    fu_object_unref(self->rand);
}

static void t_app_class_init(FUObjectClass* oc)
{
    oc->finalize = (FUObjectFunc)t_app_finalize;
}

static TApp* t_app_new()
{
    TApp* app = (TApp*)fu_app_init(fu_object_new(T_TYPE_APP));
    app->rand = fu_rand_new(EFU_RAND_ALGORITHM_DEFAULT);
    app->max = 20;
    return app;
}

static void app_active_callback(FUObject* obj, void* usd)
{
    printf("%s\n", __func__);
    TApp* app = (TApp*)usd;
    FUSource* src = fu_timeout_source_new(100);
    fu_source_set_callback(src, (FUSourceCallback)t_app_timeout_callback, app);
    fu_app_take_source((FUApp*)app, &src);
}

static void app_quit_callback(FUObject* obj, void* usd)
{
    printf("%s %p\n", __func__, usd);
}

typedef struct _TData {
    char* name;
    int n;
} TData;

static int i = 0;

static TData* test_new(const char* name)
{
    TData* dt = fu_malloc0(sizeof(TData));
    dt->name = fu_strdup(name);
    dt->n = i++;
    return dt;
}

static void test_free(TData* dt)
{
    printf("%s %s %d\n", __func__, dt->name, dt->n);
    fu_free(dt->name);
    fu_free(dt);
}

static void test_hash_table()
{
    // FUHashTable* table = fu_hash_table_new_full((FUHashFunc)fu_str_bkdr_hash, fu_str_equal, NULL, (FUNotify)test_free);
    FUHashTable* tb2 = fu_hash_table_new((FUHashFunc)fu_str_bkdr_hash, (FUNotify)test_free);
    TData* dt = test_new("open");
    fu_hash_table_insert(tb2, dt->name, dt);

    dt = test_new("close");
    fu_hash_table_insert(tb2, dt->name, dt);

    dt = test_new("focus");
    fu_hash_table_insert(tb2, dt->name, dt);

    dt = test_new("resize");
    fu_hash_table_insert(tb2, dt->name, dt);

    dt = test_new("scale");
    fu_hash_table_insert(tb2, dt->name, dt);

    dt = test_new("key-down");
    fu_hash_table_insert(tb2, dt->name, dt);

    dt = test_new("key-up");
    fu_hash_table_insert(tb2, dt->name, dt);

    dt = test_new("key-press");
    fu_hash_table_insert(tb2, dt->name, dt);

    dt = test_new("mouse-move");
    fu_hash_table_insert(tb2, dt->name, dt);

    dt = test_new("mouse-down");
    fu_hash_table_insert(tb2, dt->name, dt);

    dt = fu_hash_table_lookup(tb2, "focus");
    if (dt) {
        printf("table: key: %s name %s %d\n", dt->name, dt->name, dt->n);
    } else {
        printf("table: focus not found\n");
    }

    dt = fu_hash_table_lookup(tb2, "clos");
    if (dt) {
        printf("table: key: %s name %s %d\n", dt->name, dt->name, dt->n);
    } else {
        printf("table: clos not found\n");
    }

    dt = fu_hash_table_lookup(tb2, "key-up");
    if (dt) {
        printf("table: key: %s name %s %d\n", dt->name, dt->name, dt->n);
    } else {
        printf("table: clos not found\n");
    }

    dt = test_new("key-down");
    fu_hash_table_insert(tb2, dt->name, dt);

    dt = test_new("key-up");
    fu_hash_table_insert(tb2, dt->name, dt);

    dt = fu_hash_table_lookup(tb2, "close");
    if (dt) {
        printf("table: key: %s name %s %d\n", dt->name, dt->name, dt->n);
    } else {
        printf("table: close not found\n");
    }

    dt = fu_hash_table_lookup(tb2, "key-up");
    if (dt) {
        printf("table: key: %s name %s %d\n", dt->name, dt->name, dt->n);
    } else {
        printf("table: clos not found\n");
    }

    FUHashTableIter iter;
    fu_hash_table_iter_init(&iter, tb2);
    while (fu_hash_table_iter_next(&iter, (void**)&dt)) {
        printf("table: name %s %d\n", dt->name, dt->n);
    }

    fu_hash_table_unref(tb2);
}

int main(int argc, char* argv[])
{
    setlocale(LC_ALL, ".UTF-8");
    TApp* app = t_app_new();
    fu_signal_connect((FUObject*)app, "active", app_active_callback, app);
    fu_signal_connect((FUObject*)app, "quit", app_quit_callback, app);

    test_hash_table();

    fu_app_run((FUApp*)app);
    fu_object_unref(app);

    printf("bye\n");
    return 0;
}
