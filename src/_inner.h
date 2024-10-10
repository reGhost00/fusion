#ifdef _FUSION_H_
#error "Do not include this header directly. Use fusion.h instead."
#else
#ifdef _FU_CORE_MAIN_ENABLE_
#include <stdatomic.h>
#include <stdint.h>
// custom header
#include "core/array.h"
#include "core/object.h"
// main 模块实现
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

struct _FUMainLoop {
    FUObject parent;
    TSource* srcs;
    atomic_int cnt;
    EMainLoopState state;
    bool active;
};

typedef struct _TApp {
    FUObject parent;
    /** 主循环 */
    FUMainLoop* loop;
    /** 用户数据源 */
    FUPtrArray* sources;
    // /** 窗口数组 */
    // // FUPtrArray* windows;
    // TSource* windows;
} TApp;

#endif // _FU_CORE_MAIN_ENABLE_DEF_

#ifdef _FU_PLATFORM_GLFW_ENABLE_
#include <stdatomic.h>
#include <stdint.h>

#include "core/array.h"
#include "core/object.h"
#include "renderer/types.h"

typedef enum _EWindowEvent {
    E_WINDOW_EVENT_NONE,
    E_WINDOW_EVENT_CLOSE,
    E_WINDOW_EVENT_FOCUS,
    E_WINDOW_EVENT_RESIZE,
    E_WINDOW_EVENT_SCALE,
    E_WINDOW_EVENT_SCROLL,
    E_WINDOW_EVENT_MOUSE,
    E_WINDOW_EVENT_KEYBOARD,
    E_WINDOW_EVENT_GAMEPAD,
    E_WINDOW_EVENT_CNT
} EWindowEvent;

#ifndef _FU_PLATFORM_EVENT_WRAP_
typedef struct _FUKeyboardEvent FUKeyboardEvent;
typedef struct _FUMouseEvent FUMouseEvent;
typedef struct _FUScrollEvent FUScrollEvent;
#endif
#ifndef _FU_CORE_MAIN_ENABLE_
typedef struct _TApp TApp;
#endif
typedef struct _TWindowEvent TWindowEvent;
struct _TWindowEvent {
    TWindowEvent* next;
    EWindowEvent type;
    FUVec2* scale;
    FUSize* resize;
    FUKeyboardEvent* keyboard;
    FUMouseEvent* mouse;
    FUScrollEvent* scroll;
    bool focused;
    uint64_t stmp;
};

typedef struct _FUWindow FUWindow;
struct _FUWindow {
    FUObject parent;
    FUWindow* child;
    FUWindow* prev;
    FUWindow* next;
    TApp* app;
    // FURenderer* renderer;
    // FUSurface* surface;
    TWindowEvent* event;
    /** 用户数据源 */
    FUPtrArray* sources;
    GLFWwindow* window;
    // bool active;
    atomic_bool active;
    // uint32_t idx;
};

#endif // _FU_PLATFORM_GLFW_ENABLE_
#ifdef _FU_RENDERER_GL_ENABLE_
#ifdef FU_NO_DEBUG
#define fu_retrun_if_shader_compile_failed(shader, source, rev, msg) \
    do {                                                             \
        glShaderSource(shader, 1, &source, NULL);                    \
        glCompileShader(shader);                                     \
    } while (0)

#define fu_return_if_program_link_failed(program, vertex, fragment, rev, msg) \
    do {                                                                      \
        glAttachShader(program, vertex);                                      \
        glAttachShader(program, fragment);                                    \
        glLinkProgram(program);                                               \
    } while (0)

#define fu_gl_check_error() (void)0
#else
#include <stdio.h>
#define fu_retrun_if_shader_compile_failed(shader, source, rev, msg)              \
    do {                                                                          \
        glShaderSource(shader, 1, &source, NULL);                                 \
        glCompileShader(shader);                                                  \
        glGetShaderiv(shader, GL_COMPILE_STATUS, &rev);                           \
        if (!rev) {                                                               \
            glGetShaderInfoLog(shader, DEF_MSG_BUFF_LEN, NULL, msg);              \
            fprintf(stderr, "%s Shader compilation failed: %s\n", __func__, msg); \
            return NULL;                                                          \
        }                                                                         \
    } while (0)

#define fu_return_if_program_link_failed(program, vertex, fragment, rev, msg) \
    do {                                                                      \
        glAttachShader(program, vertex);                                      \
        glAttachShader(program, fragment);                                    \
        glLinkProgram(program);                                               \
        glGetProgramiv(program, GL_LINK_STATUS, &rev);                        \
        if (!rev) {                                                           \
            glGetProgramInfoLog(program, DEF_MSG_BUFF_LEN, NULL, msg);        \
            fprintf(stderr, "%s Shader link failed: %s\n", __func__, msg);    \
            return NULL;                                                      \
        }                                                                     \
    } while (0)

static void fu_gl_check_error()
{
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        switch (err) {
        case GL_INVALID_ENUM:
            fprintf(stderr, "OpenGL error: GL_INVALID_ENUM\n");
            break;
        case GL_INVALID_VALUE:
            fprintf(stderr, "OpenGL error: GL_INVALID_VALUE\n");
            break;
        case GL_INVALID_OPERATION:
            fprintf(stderr, "OpenGL error: GL_INVALID_OPERATION\n");
            break;
        case GL_OUT_OF_MEMORY:
            fprintf(stderr, "OpenGL error: GL_OUT_OF_MEMORY\n");
            break;
        default:
            fprintf(stderr, "OpenGL error: %d\n", err);
            break;
        }
    }
}

#endif
#endif
#endif