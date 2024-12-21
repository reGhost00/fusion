#include <mimalloc.h>
#define NK_IMPLEMENTATION
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_BOOL
#include "nuklear.h"

#define PORTABLEGL_IMPLEMENTATION
#define PGL_MALLOC(s) mi_malloc(s)
#define PGL_REALLOC(p, s) mi_realloc(p, s)
#define PGL_FREE(p) mi_free(p)
#include "portablegl.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_MALLOC(s) mi_malloc(s)
#define STBI_REALLOC(p, s) mi_realloc(p, s)
#define STBI_FREE(p) mi_free(p)
#include "stb_image.h"

// #define STB_TRUETYPE_IMPLEMENTATION
// #include "stb_truetype.h"