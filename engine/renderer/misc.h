#pragma once
#include <stdint.h>

#ifndef FU_COMPILER_C23
#include <stdbool.h>
#ifndef static_assert
#define static_assert _Static_assert
#endif
#ifndef typeof
#define typeof(t) __typeof__(t)
#endif
#ifndef offsetof
#define offsetof(s, m) __builtin_offsetof(s, m)
#endif
#ifndef alignof
#define alignof(x) __alignof__(x)
#endif
#ifndef alignas
#define alignas(x) __attribute__((aligned(x)))
#endif
#endif
#ifdef NDEBUG
#define DEF_WINDOW_TITLE_DEBUG_SUFFIX "[RELEASE]"
#else
#define DEF_WINDOW_TITLE_DEBUG_SUFFIX "[DEBUG]"
#endif
#ifdef FU_RENDERER_TYPE_GL
#define DEF_WINDOW_TITLE_RENDER_SUFFIX "[GL]"
#elif defined(FU_RENDERER_TYPE_VK)
#include <vulkan/vulkan.h>
#define DEF_WINDOW_TITLE_RENDER_SUFFIX "[VK]"
#define DEF_APP_VER VK_MAKE_VERSION(0, 0, 1)
#endif
#define DEF_WINDOW_TITLE "fusion"
#define DEF_APP_NAME "reghost.fusion.app"

typedef struct _FURect {
    uint32_t width, height, x, y;
} FURect;

typedef struct _FUSize {
    uint32_t width, height;
} FUSize;

typedef struct _FUOffset2 {
    int32_t x, y;
} FUOffset2;

typedef struct _FUOffset3 {
    int32_t x, y, z;
} FUOffset3;

typedef struct _FUVec2 {
    float x, y;
} FUVec2;

typedef struct _FUVec3 {
    float x, y, z;
} FUVec3;

typedef struct _FUVec4 {
    float x, y, z, w;
} FUVec4;

typedef struct _FURGB {
    uint8_t r, g, b;
} FURGB;

typedef struct _FURGBA {
    uint8_t r, g, b, a;
} FURGBA;

typedef struct _FUPoint2 {
    uint32_t x, y;
} FUPoint2;

typedef struct _FUPoint3 {
    uint32_t x, y, z;
} FUPoint3;

typedef struct _FUVertex {
    FUVec3 position;
    FUVec4 color;
    FUVec2 texture;
} FUVertex;

typedef struct _FUShape {
    FUVertex* vertices;
    FUPoint3* indices;
    uint32_t vertexCount, indexCount;
} FUShape;
