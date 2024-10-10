#ifndef __FU_MAIN_H__
#define __FU_MAIN_H__

// #include "_def.h"
#include "../renderer/gl2.h"
#include "object.h"

/** 用于给其他数据源继承 */
typedef struct _FUSource {
    void* dummy[12];
} FUSource;
uint64_t fu_source_get_type();
#define FU_TYPE_SOURCE (fu_source_get_type())
// // #endif
// // #ifndef _FU_SRC_CBS_DEC_
// // #define _FU_SRC_CBS_DEC_
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
    // FUObject parent;
    // FUMainLoop* loop;
    void* dummy[5];
} FUApp;
uint64_t fu_app_get_type();
#define FU_TYPE_APP (fu_app_get_type())

// #endif
// FU_DECLARE_TYPE(FUWindow, fu_window)
// #define FU_TYPE_WINDOW (fu_window_get_type())
//
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

// FUWindow* fu_window_new(FUApp* app, const char* title, const uint32_t width, const uint32_t height);

// FURenderer* fu_app_get_renderer(FUApp* app);
// void fu_app_swap_buffers(FUApp* app);
#endif /* __GLIB_TYPEOF_H__ */
