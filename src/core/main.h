#pragma once

#include "object.h"

/** 用于给其他数据源继承 */
typedef struct _FUSource {
    void* dummy[12];
} FUSource;
uint64_t fu_source_get_type();
#define FU_TYPE_SOURCE (fu_source_get_type())

typedef bool (*FUSourceFuncRevBool)(FUSource* source, void* usd);
typedef void (*FUSourceFuncRevVoid)(FUSource* source, void* usd);
typedef bool (*FUSourceCallback)(void* usd);

typedef struct _FUSourceFuncs {
    const FUSourceFuncRevBool prepare;
    const FUSourceFuncRevBool check;
    const FUSourceFuncRevVoid cleanup;
} FUSourceFuncs;

FU_DECLARE_TYPE(FUMainLoop, fu_main_loop)
#define FU_TYPE_MAIN_LOOP (fu_main_loop_get_type())

typedef struct _FUApp {
    void* dummy[5];
} FUApp;
uint64_t fu_app_get_type();
#define FU_TYPE_APP (fu_app_get_type())

// Source
/** Use to derivable source */
FUSource*
fu_source_init(FUObject* obj, const FUSourceFuncs* fns, void* usd);
/** Create a new source */
FUSource* fu_source_new(const FUSourceFuncs* fns, void* usd);
void fu_source_remove(FUSource* source);
void fu_source_set_callback(FUSource* source, const FUSourceCallback callback, void* usd);
void fu_source_attach(FUSource* source, FUMainLoop* loop);
void fu_source_destroy(FUSource* source);
//
// MainLoop
/** Create a new main loop */
FUMainLoop* fu_main_loop_new(void);
void fu_main_loop_quit(FUMainLoop* loop);
void fu_main_loop_run(FUMainLoop* loop);
//
// App
/** Use to derivable app */
FUApp* fu_app_init(FUObject* app);
/** Create a new app */
FUApp* fu_app_new(void);
void fu_app_take_source(FUApp* app, FUSource** source);
void fu_app_run(FUApp* app);
void fu_app_quit(FUApp* app);
