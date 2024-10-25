#include "shader.h"
#ifdef FU_RENDERER_TYPE_GL
#ifdef FU_USE_SPIRV_COMPILER
typedef struct _FUShader {
    FUObject parent;
    uint32_t* vertexCode;
    uint32_t* fragmentCode;
    int vertexCodeSize;
    int fragmentCodeSize;
} FUShader;
#else // FU_USE_SPIRV_COMPILER
typedef struct _FUShader {
    FUObject parent;
    uint32_t program;
} FUShader;
#endif
#elif FU_RENDERER_TYPE_VK
typedef struct _FUShader {
    FUObject parent;
    uint32_t* vertexCode;
    uint32_t* fragmentCode;
    int vertexCodeSize;
    int fragmentCodeSize;
} FUShader;
#endif
