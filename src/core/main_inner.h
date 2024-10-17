#pragma once
#include <stdatomic.h>
// custom
#include "array.h"
#include "main.h"
#include "object.h"

//
// 事件源
// 事件源本身是一个双链表

//
// Source
// 数据源有下面 4 种状态
// 在事件循环中的 Check 和 Dispatch 状态下，如果数据原不是对应的状态
// 则不触发对应的事件
typedef enum _ESourceState {
    E_SOURCE_STATE_NONE,
    E_SOURCE_STATE_PREPAR,
    E_SOURCE_STATE_CHECK,
    E_SOURCE_STATE_ACTIVE,
    E_SOURCE_STATE_CLEANUP,
    E_SOURCE_STATE_CNT
} ESourceState;

typedef struct _TSource TSource;
struct _TSource {
    FUObject parent;
    TSource* prev;
    TSource* next;
    FUMainLoop* loop;
    uint64_t id;
    ESourceState state;
    const FUSourceFuncs* fns;
    FUSourceCallback cb;
    void* usdFN;
    void* usdCB;
};

//
// MainLoop
// 事件循环在下面 4 种状态中轮回
// 每个状态执行事件源中对应的事件

typedef enum _EMainLoopState {
    E_MAIN_LOOP_STATE_PREPAR,
    E_MAIN_LOOP_STATE_CHECK,
    E_MAIN_LOOP_STATE_DISPATCH,
    E_MAIN_LOOP_STATE_CLEANUP,
    E_MAIN_LOOP_STATE_CNT
} EMainLoopState;
typedef void (*FUMainLoopPollEvent)();

struct _FUMainLoop {
    FUObject parent;
    TSource* srcs;
    EMainLoopState state;
    atomic_int cnt;
    bool active;
};

typedef struct _TApp {
    FUObject parent;
    /** 主循环 */
    FUMainLoop* loop;
    /** 用户数据源 */
    FUPtrArray* sources;
} TApp;

// typedef FUApp TApp;