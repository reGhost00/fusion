#pragma once

#include "core/array.h"
#include "core/object.h"
#include "misc.h"
#include "shader.h"


typedef struct _FUContext {
    FUObject parent;
    FUCommandBuffer* commands;
    FUSize size;
    uint32_t idx;
    FUArray* vertices;
    FUArray* indices;
} FUContext;

typedef struct _TCommandBuffer TCommandBuffer;
struct _TCommandBuffer {
    TCommandBuffer* prev;
    TCommandBuffer* next;
    FUShader* shader;
    FUVertex* vertices;
    FUPoint3* indices;
    uint32_t vertexCount, indexCount;
};

void t_surface_update_size(FUWindow* window, FUSize* size);
