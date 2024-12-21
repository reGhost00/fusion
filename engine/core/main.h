#pragma once
#ifndef FU_COMPILER_C23
#include <stdbool.h>
#endif
#include "object.h"

/** 用于给其他数据源继承 */
typedef struct _FUSource {
    void* dummy[12];
} FUSource;
uint64_t fu_source_get_type();
#define FU_TYPE_SOURCE (fu_source_get_type())

typedef bool (*FUSourceCallback)(void* usd);
typedef bool (*FUSourceFunc)(FUSource* source, void* usd);
typedef void (*FUSourceCleanupFunc)(FUSource* source, void* usd);
typedef struct _FUSourceFuncs {
    const FUSourceFunc prepare;
    const FUSourceFunc check;
    const FUSourceCleanupFunc cleanup;
} FUSourceFuncs;

// Source
/** Use to derivable source */
FUSource* fu_source_init(FUObject* obj, const FUSourceFuncs* fns, void* usd);
/** Create a new source */
FUSource* fu_source_new(const FUSourceFuncs* fns, void* usd);
void fu_source_remove(FUSource* source);
void fu_source_set_callback(FUSource* source, const FUSourceCallback callback, void* usd);
void fu_source_destroy(FUSource* source);

FU_DECLARE_TYPE(FUApplication, fu_application)
#define FU_TYPE_APPLICATION (fu_application_get_type())

FUApplication* fu_application_new(void);
void fu_application_take_source(FUApplication* app, FUSource** source);
void fu_application_run(FUApplication* app);
void fu_application_quit(FUApplication* app);