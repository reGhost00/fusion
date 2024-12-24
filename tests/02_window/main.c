#include <assert.h>
#include <stdio.h>

#include "libs/stb_image.h"
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>
#include <fusion.h>

#define DEF_IMG "E:\\Pictures\\bz_ladfdfdg_0011.jpg"
typedef struct _TApp {
    FUApplication* app;
    FUWindow* window;
    FUContext* context;
    FUSource* timerout;
    char* title;
    uint32_t uboIdx;
} TApp;
static char buff[1600];
// #define _depth
#ifdef _depth
typedef struct _TVertex {
    float position[3];
    float color[4];
    float uv[2];
} TVertex;

typedef struct _TUniformBufferObject {
    alignas(16) mat4 model;
    alignas(16) mat4 view;
    alignas(16) mat4 proj;
} TUniformBufferObject;

static const TVertex vertices[] = {
    { { -0.5f, -0.5f }, { 1.0f, 1.0f, 0.0f, 1.0f }, { 1.0f } },
    { { -0.5f, 0.5f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { { 0.5f, 0.5f }, { 0.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
    { { 0.5f, -0.5f }, { 1.0f, 0.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },

    { { -0.5f, -0.5f, -0.5f }, { 1.0f, 1.0f, 0.0f, 1.0f }, { 1.0f } },
    { { -0.5f, 0.5f, -0.5f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { { 0.5f, 0.5f, -0.5f }, { 0.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
    { { 0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } }
};

static const uint32_t indices[] = {
    0, 1, 2, 0, 2, 3,
    4, 5, 6, 4, 6, 7
};

#else
typedef struct _TVertex {
    float position[3];
    float color[4];
} TVertex;

static const TVertex vertices[] = {
    { { -0.5f, -0.5f }, { 1.0f, 1.0f, 0.0f, 1.0f } },
    { { -0.5f, 0.5f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { { 0.5f, 0.5f }, { 0.0f, 1.0f, 1.0f, 1.0f } },
    { { 0.5f, -0.5f }, { 1.0f, 0.0f, 1.0f, 1.0f } }
};

static const uint32_t indices[] = {
    0, 1, 2,
    0, 2, 3
};
#endif

static TApp* t_app_new()
{
    TApp* app = (TApp*)fu_malloc(sizeof(TApp));
    app->app = fu_application_new();
    app->title = "02_window";
    return app;
}

static void t_app_free(TApp* app)
{
    fu_object_unref(app->app);
    fu_free(app);
}

static bool app_timer_tick_callback(TApp* usd)
{
#ifdef _depth
    TUniformBufferObject ubo = {
        .model = GLM_MAT4_IDENTITY_INIT,
        // .view = GLM_MAT4_IDENTITY_INIT,
        // .proj = GLM_MAT4_IDENTITY_INIT,
    };

    glm_rotate(ubo.model, glfwGetTime() / glm_rad(10.0f), (vec3) { 0.0f, 0.0f, 1.0f });
    glm_lookat((vec3) { 2.0f, 2.0f, 2.0f }, GLM_VEC3_ZERO, (vec3) { 0.0f, 0.0f, 1.0f }, ubo.view);
    glm_perspective(glm_rad(30.0f), (float)0.7, 0.1f, 10.0f, ubo.proj);
    ubo.proj[1][1] *= -1;
    fu_context_update_uniform_buffer(usd->context, usd->uboIdx, &ubo, sizeof(ubo));
#endif
    fu_window_present(usd->window);
    return true;
}

#define app_destroy_surface(app) (fu_object_unref(app->context))

static void app_init_surface(TApp* app)
{
#ifdef _depth
    const uint32_t _depth_vert[] = {
        0x07230203, 0x00010600, 0x0008000b, 0x00000035, 0x00000000, 0x00020011, 0x00000001, 0x0006000b,
        0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001,
        0x000c000f, 0x00000000, 0x00000004, 0x6e69616d, 0x00000000, 0x0000000d, 0x00000013, 0x00000021,
        0x0000002b, 0x0000002d, 0x00000031, 0x00000033, 0x00030003, 0x00000002, 0x000001cc, 0x00040005,
        0x00000004, 0x6e69616d, 0x00000000, 0x00060005, 0x0000000b, 0x505f6c67, 0x65567265, 0x78657472,
        0x00000000, 0x00060006, 0x0000000b, 0x00000000, 0x505f6c67, 0x7469736f, 0x006e6f69, 0x00070006,
        0x0000000b, 0x00000001, 0x505f6c67, 0x746e696f, 0x657a6953, 0x00000000, 0x00070006, 0x0000000b,
        0x00000002, 0x435f6c67, 0x4470696c, 0x61747369, 0x0065636e, 0x00070006, 0x0000000b, 0x00000003,
        0x435f6c67, 0x446c6c75, 0x61747369, 0x0065636e, 0x00030005, 0x0000000d, 0x00000000, 0x00070005,
        0x00000011, 0x66696e55, 0x426d726f, 0x65666675, 0x6a624f72, 0x00746365, 0x00050006, 0x00000011,
        0x00000000, 0x65646f6d, 0x0000006c, 0x00050006, 0x00000011, 0x00000001, 0x77656976, 0x00000000,
        0x00050006, 0x00000011, 0x00000002, 0x6a6f7270, 0x00000000, 0x00030005, 0x00000013, 0x006f6275,
        0x00050005, 0x00000021, 0x6f506e69, 0x69746973, 0x00006e6f, 0x00050005, 0x0000002b, 0x67617266,
        0x6f6c6f43, 0x00000072, 0x00040005, 0x0000002d, 0x6f436e69, 0x00726f6c, 0x00040005, 0x00000031,
        0x67617266, 0x00005655, 0x00040005, 0x00000033, 0x56556e69, 0x00000000, 0x00050048, 0x0000000b,
        0x00000000, 0x0000000b, 0x00000000, 0x00050048, 0x0000000b, 0x00000001, 0x0000000b, 0x00000001,
        0x00050048, 0x0000000b, 0x00000002, 0x0000000b, 0x00000003, 0x00050048, 0x0000000b, 0x00000003,
        0x0000000b, 0x00000004, 0x00030047, 0x0000000b, 0x00000002, 0x00040048, 0x00000011, 0x00000000,
        0x00000005, 0x00050048, 0x00000011, 0x00000000, 0x00000023, 0x00000000, 0x00050048, 0x00000011,
        0x00000000, 0x00000007, 0x00000010, 0x00040048, 0x00000011, 0x00000001, 0x00000005, 0x00050048,
        0x00000011, 0x00000001, 0x00000023, 0x00000040, 0x00050048, 0x00000011, 0x00000001, 0x00000007,
        0x00000010, 0x00040048, 0x00000011, 0x00000002, 0x00000005, 0x00050048, 0x00000011, 0x00000002,
        0x00000023, 0x00000080, 0x00050048, 0x00000011, 0x00000002, 0x00000007, 0x00000010, 0x00030047,
        0x00000011, 0x00000002, 0x00040047, 0x00000013, 0x00000022, 0x00000000, 0x00040047, 0x00000013,
        0x00000021, 0x00000000, 0x00040047, 0x00000021, 0x0000001e, 0x00000000, 0x00040047, 0x0000002b,
        0x0000001e, 0x00000000, 0x00040047, 0x0000002d, 0x0000001e, 0x00000001, 0x00040047, 0x00000031,
        0x0000001e, 0x00000001, 0x00040047, 0x00000033, 0x0000001e, 0x00000002, 0x00020013, 0x00000002,
        0x00030021, 0x00000003, 0x00000002, 0x00030016, 0x00000006, 0x00000020, 0x00040017, 0x00000007,
        0x00000006, 0x00000004, 0x00040015, 0x00000008, 0x00000020, 0x00000000, 0x0004002b, 0x00000008,
        0x00000009, 0x00000001, 0x0004001c, 0x0000000a, 0x00000006, 0x00000009, 0x0006001e, 0x0000000b,
        0x00000007, 0x00000006, 0x0000000a, 0x0000000a, 0x00040020, 0x0000000c, 0x00000003, 0x0000000b,
        0x0004003b, 0x0000000c, 0x0000000d, 0x00000003, 0x00040015, 0x0000000e, 0x00000020, 0x00000001,
        0x0004002b, 0x0000000e, 0x0000000f, 0x00000000, 0x00040018, 0x00000010, 0x00000007, 0x00000004,
        0x0005001e, 0x00000011, 0x00000010, 0x00000010, 0x00000010, 0x00040020, 0x00000012, 0x00000002,
        0x00000011, 0x0004003b, 0x00000012, 0x00000013, 0x00000002, 0x0004002b, 0x0000000e, 0x00000014,
        0x00000002, 0x00040020, 0x00000015, 0x00000002, 0x00000010, 0x0004002b, 0x0000000e, 0x00000018,
        0x00000001, 0x00040017, 0x0000001f, 0x00000006, 0x00000003, 0x00040020, 0x00000020, 0x00000001,
        0x0000001f, 0x0004003b, 0x00000020, 0x00000021, 0x00000001, 0x0004002b, 0x00000006, 0x00000023,
        0x3f800000, 0x00040020, 0x00000029, 0x00000003, 0x00000007, 0x0004003b, 0x00000029, 0x0000002b,
        0x00000003, 0x00040020, 0x0000002c, 0x00000001, 0x00000007, 0x0004003b, 0x0000002c, 0x0000002d,
        0x00000001, 0x00040017, 0x0000002f, 0x00000006, 0x00000002, 0x00040020, 0x00000030, 0x00000003,
        0x0000002f, 0x0004003b, 0x00000030, 0x00000031, 0x00000003, 0x00040020, 0x00000032, 0x00000001,
        0x0000002f, 0x0004003b, 0x00000032, 0x00000033, 0x00000001, 0x00050036, 0x00000002, 0x00000004,
        0x00000000, 0x00000003, 0x000200f8, 0x00000005, 0x00050041, 0x00000015, 0x00000016, 0x00000013,
        0x00000014, 0x0004003d, 0x00000010, 0x00000017, 0x00000016, 0x00050041, 0x00000015, 0x00000019,
        0x00000013, 0x00000018, 0x0004003d, 0x00000010, 0x0000001a, 0x00000019, 0x00050092, 0x00000010,
        0x0000001b, 0x00000017, 0x0000001a, 0x00050041, 0x00000015, 0x0000001c, 0x00000013, 0x0000000f,
        0x0004003d, 0x00000010, 0x0000001d, 0x0000001c, 0x00050092, 0x00000010, 0x0000001e, 0x0000001b,
        0x0000001d, 0x0004003d, 0x0000001f, 0x00000022, 0x00000021, 0x00050051, 0x00000006, 0x00000024,
        0x00000022, 0x00000000, 0x00050051, 0x00000006, 0x00000025, 0x00000022, 0x00000001, 0x00050051,
        0x00000006, 0x00000026, 0x00000022, 0x00000002, 0x00070050, 0x00000007, 0x00000027, 0x00000024,
        0x00000025, 0x00000026, 0x00000023, 0x00050091, 0x00000007, 0x00000028, 0x0000001e, 0x00000027,
        0x00050041, 0x00000029, 0x0000002a, 0x0000000d, 0x0000000f, 0x0003003e, 0x0000002a, 0x00000028,
        0x0004003d, 0x00000007, 0x0000002e, 0x0000002d, 0x0003003e, 0x0000002b, 0x0000002e, 0x0004003d,
        0x0000002f, 0x00000034, 0x00000033, 0x0003003e, 0x00000031, 0x00000034, 0x000100fd, 0x00010038
    };
    const uint32_t _depth_frag[] = {
        0x07230203, 0x00010600, 0x0008000b, 0x00000018, 0x00000000, 0x00020011, 0x00000001, 0x0006000b,
        0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001,
        0x0009000f, 0x00000004, 0x00000004, 0x6e69616d, 0x00000000, 0x00000009, 0x0000000d, 0x00000011,
        0x00000015, 0x00030010, 0x00000004, 0x00000007, 0x00030003, 0x00000002, 0x000001cc, 0x00040005,
        0x00000004, 0x6e69616d, 0x00000000, 0x00050005, 0x00000009, 0x4374756f, 0x726f6c6f, 0x00000000,
        0x00050005, 0x0000000d, 0x53786574, 0x6c706d61, 0x00007265, 0x00040005, 0x00000011, 0x67617266,
        0x00005655, 0x00050005, 0x00000015, 0x67617266, 0x6f6c6f43, 0x00000072, 0x00040047, 0x00000009,
        0x0000001e, 0x00000000, 0x00040047, 0x0000000d, 0x00000022, 0x00000000, 0x00040047, 0x0000000d,
        0x00000021, 0x00000001, 0x00040047, 0x00000011, 0x0000001e, 0x00000001, 0x00040047, 0x00000015,
        0x0000001e, 0x00000000, 0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002, 0x00030016,
        0x00000006, 0x00000020, 0x00040017, 0x00000007, 0x00000006, 0x00000004, 0x00040020, 0x00000008,
        0x00000003, 0x00000007, 0x0004003b, 0x00000008, 0x00000009, 0x00000003, 0x00090019, 0x0000000a,
        0x00000006, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x0003001b,
        0x0000000b, 0x0000000a, 0x00040020, 0x0000000c, 0x00000000, 0x0000000b, 0x0004003b, 0x0000000c,
        0x0000000d, 0x00000000, 0x00040017, 0x0000000f, 0x00000006, 0x00000002, 0x00040020, 0x00000010,
        0x00000001, 0x0000000f, 0x0004003b, 0x00000010, 0x00000011, 0x00000001, 0x00040020, 0x00000014,
        0x00000001, 0x00000007, 0x0004003b, 0x00000014, 0x00000015, 0x00000001, 0x00050036, 0x00000002,
        0x00000004, 0x00000000, 0x00000003, 0x000200f8, 0x00000005, 0x0004003d, 0x0000000b, 0x0000000e,
        0x0000000d, 0x0004003d, 0x0000000f, 0x00000012, 0x00000011, 0x00050057, 0x00000007, 0x00000013,
        0x0000000e, 0x00000012, 0x0004003d, 0x00000007, 0x00000016, 0x00000015, 0x00050085, 0x00000007,
        0x00000017, 0x00000013, 0x00000016, 0x0003003e, 0x00000009, 0x00000017, 0x000100fd, 0x00010038
    };
    FUShaderCode shaderVertCode = {
        .code = _depth_vert,
        .size = sizeof(_depth_vert),
    };
    FUShaderCode shaderFragCode = {
        .code = _depth_frag,
        .size = sizeof(_depth_frag),
    };
#else
    const uint32_t _basic_vert[] = {
        0x07230203, 0x00010600, 0x0008000b, 0x0000001f, 0x00000000, 0x00020011, 0x00000001, 0x0006000b,
        0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001,
        0x0009000f, 0x00000000, 0x00000004, 0x6e69616d, 0x00000000, 0x0000000d, 0x00000012, 0x0000001b,
        0x0000001d, 0x00030003, 0x00000002, 0x000001cc, 0x00040005, 0x00000004, 0x6e69616d, 0x00000000,
        0x00060005, 0x0000000b, 0x505f6c67, 0x65567265, 0x78657472, 0x00000000, 0x00060006, 0x0000000b,
        0x00000000, 0x505f6c67, 0x7469736f, 0x006e6f69, 0x00070006, 0x0000000b, 0x00000001, 0x505f6c67,
        0x746e696f, 0x657a6953, 0x00000000, 0x00070006, 0x0000000b, 0x00000002, 0x435f6c67, 0x4470696c,
        0x61747369, 0x0065636e, 0x00070006, 0x0000000b, 0x00000003, 0x435f6c67, 0x446c6c75, 0x61747369,
        0x0065636e, 0x00030005, 0x0000000d, 0x00000000, 0x00050005, 0x00000012, 0x6f506e69, 0x69746973,
        0x00006e6f, 0x00050005, 0x0000001b, 0x67617266, 0x6f6c6f43, 0x00000072, 0x00040005, 0x0000001d,
        0x6f436e69, 0x00726f6c, 0x00050048, 0x0000000b, 0x00000000, 0x0000000b, 0x00000000, 0x00050048,
        0x0000000b, 0x00000001, 0x0000000b, 0x00000001, 0x00050048, 0x0000000b, 0x00000002, 0x0000000b,
        0x00000003, 0x00050048, 0x0000000b, 0x00000003, 0x0000000b, 0x00000004, 0x00030047, 0x0000000b,
        0x00000002, 0x00040047, 0x00000012, 0x0000001e, 0x00000000, 0x00040047, 0x0000001b, 0x0000001e,
        0x00000000, 0x00040047, 0x0000001d, 0x0000001e, 0x00000001, 0x00020013, 0x00000002, 0x00030021,
        0x00000003, 0x00000002, 0x00030016, 0x00000006, 0x00000020, 0x00040017, 0x00000007, 0x00000006,
        0x00000004, 0x00040015, 0x00000008, 0x00000020, 0x00000000, 0x0004002b, 0x00000008, 0x00000009,
        0x00000001, 0x0004001c, 0x0000000a, 0x00000006, 0x00000009, 0x0006001e, 0x0000000b, 0x00000007,
        0x00000006, 0x0000000a, 0x0000000a, 0x00040020, 0x0000000c, 0x00000003, 0x0000000b, 0x0004003b,
        0x0000000c, 0x0000000d, 0x00000003, 0x00040015, 0x0000000e, 0x00000020, 0x00000001, 0x0004002b,
        0x0000000e, 0x0000000f, 0x00000000, 0x00040017, 0x00000010, 0x00000006, 0x00000003, 0x00040020,
        0x00000011, 0x00000001, 0x00000010, 0x0004003b, 0x00000011, 0x00000012, 0x00000001, 0x0004002b,
        0x00000006, 0x00000014, 0x3f800000, 0x00040020, 0x00000019, 0x00000003, 0x00000007, 0x0004003b,
        0x00000019, 0x0000001b, 0x00000003, 0x00040020, 0x0000001c, 0x00000001, 0x00000007, 0x0004003b,
        0x0000001c, 0x0000001d, 0x00000001, 0x00050036, 0x00000002, 0x00000004, 0x00000000, 0x00000003,
        0x000200f8, 0x00000005, 0x0004003d, 0x00000010, 0x00000013, 0x00000012, 0x00050051, 0x00000006,
        0x00000015, 0x00000013, 0x00000000, 0x00050051, 0x00000006, 0x00000016, 0x00000013, 0x00000001,
        0x00050051, 0x00000006, 0x00000017, 0x00000013, 0x00000002, 0x00070050, 0x00000007, 0x00000018,
        0x00000015, 0x00000016, 0x00000017, 0x00000014, 0x00050041, 0x00000019, 0x0000001a, 0x0000000d,
        0x0000000f, 0x0003003e, 0x0000001a, 0x00000018, 0x0004003d, 0x00000007, 0x0000001e, 0x0000001d,
        0x0003003e, 0x0000001b, 0x0000001e, 0x000100fd, 0x00010038
    };

    const uint32_t _basic_frag[] = {
        0x07230203, 0x00010600, 0x0008000b, 0x0000000d, 0x00000000, 0x00020011, 0x00000001, 0x0006000b,
        0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001,
        0x0007000f, 0x00000004, 0x00000004, 0x6e69616d, 0x00000000, 0x00000009, 0x0000000b, 0x00030010,
        0x00000004, 0x00000007, 0x00030003, 0x00000002, 0x000001cc, 0x00040005, 0x00000004, 0x6e69616d,
        0x00000000, 0x00050005, 0x00000009, 0x4374756f, 0x726f6c6f, 0x00000000, 0x00050005, 0x0000000b,
        0x67617266, 0x6f6c6f43, 0x00000072, 0x00040047, 0x00000009, 0x0000001e, 0x00000000, 0x00040047,
        0x0000000b, 0x0000001e, 0x00000000, 0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002,
        0x00030016, 0x00000006, 0x00000020, 0x00040017, 0x00000007, 0x00000006, 0x00000004, 0x00040020,
        0x00000008, 0x00000003, 0x00000007, 0x0004003b, 0x00000008, 0x00000009, 0x00000003, 0x00040020,
        0x0000000a, 0x00000001, 0x00000007, 0x0004003b, 0x0000000a, 0x0000000b, 0x00000001, 0x00050036,
        0x00000002, 0x00000004, 0x00000000, 0x00000003, 0x000200f8, 0x00000005, 0x0004003d, 0x00000007,
        0x0000000c, 0x0000000b, 0x0003003e, 0x00000009, 0x0000000c, 0x000100fd, 0x00010038
    };
    FUShaderCode shaderVertCode = {
        .code = _basic_vert,
        .size = sizeof(_basic_vert),
    };
    FUShaderCode shaderFragCode = {
        .code = _basic_frag,
        .size = sizeof(_basic_frag),
    };
#endif
    FUShader* shaderVert = fu_shader_new(&shaderVertCode, FU_SHADER_STAGE_FLAG_VERTEX);
    FUShader* shaderFrag = fu_shader_new(&shaderFragCode, FU_SHADER_STAGE_FLAG_FRAGMENT);
    FUShaderGroup shaderGroup = { .vertex = shaderVert, .fragment = shaderFrag };

#ifdef _depth
    FUContextArgs args = {
        // .depthTest = true
    };
    int width, height, channels;
    fu_shader_set_attributes(shaderVert, 3, VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_R32G32_SFLOAT);
    stbi_uc* pixels = stbi_load(DEF_IMG, &width, &height, &channels, STBI_rgb_alpha);
    app->context = fu_context_new_take(app->window, &shaderGroup, &args);
    app->uboIdx = fu_context_add_uniform_descriptor(app->context, sizeof(TUniformBufferObject), FU_SHADER_STAGE_FLAG_VERTEX);
    fu_context_add_sampler_descriptor(app->context, width, height, pixels);
    stbi_image_free(pixels);
#else
    fu_shader_set_attributes(shaderVert, 2, VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT);
    app->context = fu_context_new_take(app->window, &shaderGroup, NULL);
#endif
    fu_context_update_input_data2(app->context, sizeof(vertices), (float*)vertices, sizeof(indices), indices);
    fu_window_add_context(app->window, app->context);
}

static void app_close_callback(FUObject* obj, void* usd)
{
    TApp* app = (TApp*)usd;
    app_destroy_surface(app);
    fu_object_unref(app->window);
    printf("%s %s\n", __func__, app->title);
}

static void app_resize_callback(FUObject* obj, void* dt, void* usd)
{
    TApp* app = (TApp*)usd;
    printf("%s %s\n", __func__, app->title);
}

static void app_focus_callback(FUObject* obj, void* dt, void* usd)
{
    TApp* app = (TApp*)usd;
    printf("%s %s\n", __func__, app->title);
}

static void app_key_down_callback(FUWindow* win, const FUKeyboardEvent* ev, TApp* usd)
{
    assert(win == usd->window);
    sprintf(buff, "%s %s %d %d\n", __func__, ev->name, ev->key, ev->scancode);
    printf("%s\n", buff);
}

static void app_key_up_callback(FUWindow* win, const FUKeyboardEvent* ev, TApp* usd)
{
    assert(win == usd->window);
    sprintf(buff, "%s %s %d %d\n", __func__, ev->name, ev->key, ev->scancode);
    printf("%s\n", buff);
}

static void app_key_press_callback(FUWindow* win, const FUKeyboardEvent* ev, TApp* usd)
{
    assert(win == usd->window);
    sprintf(buff, "%s %s %d %d\n", __func__, ev->name, ev->key, ev->scancode);
    printf("%s\n", buff);
}

static void app_mouse_move_callback(FUWindow* win, const FUMouseEvent* ev, TApp* usd)
{
    assert(win == usd->window);
    sprintf(buff, "%s pos: %d %d\n", __func__, ev->position.x, ev->position.y);
    printf("%s\n", buff);
    // fu_file_write(usd->file, buff, strlen(buff));
}

static void app_mouse_up_callback(FUWindow* win, const FUMouseEvent* ev, TApp* usd)
{
    assert(win == usd->window);
    sprintf(buff, "%s pos: %d %d %d\n", __func__, ev->button, ev->position.x, ev->position.y);
    printf("%s\n", buff);
}
static void app_mouse_press_callback(FUWindow* win, const FUMouseEvent* ev, TApp* usd)
{
    assert(win == usd->window);
    sprintf(buff, "%s pos: %d %d %d\n", __func__, ev->button, ev->position.x, ev->position.y);
    printf("%s\n", buff);
}

static void app_mouse_down_callback(FUWindow* win, const FUMouseEvent* ev, TApp* usd)
{
    assert(win == usd->window);
    sprintf(buff, "%s pos: %d %d %d\n", __func__, ev->button, ev->position.x, ev->position.y);
    printf("%s\n", buff);
}

static void on_active(FUObject* obj, void* usd)
{
    TApp* app = (TApp*)usd;
    printf("%s %s\n", __func__, app->title);

    FUSource* tm = fu_timeout_source_new(10);
    fu_source_set_callback(tm, (FUSourceCallback)app_timer_tick_callback, usd);

    app->window = fu_window_new((FUApplication*)obj, "test", 800, 600, true);
    app_init_surface(app);

    fu_signal_connect(app->window, "close", app_close_callback, app);
    fu_signal_connect(app->window, "resize", app_resize_callback, app);
    fu_signal_connect(app->window, "focus", app_focus_callback, app);
    fu_signal_connect(app->window, "key-down", app_key_down_callback, app);
    fu_signal_connect(app->window, "key-up", app_key_up_callback, app);
    fu_signal_connect(app->window, "key-press", app_key_press_callback, app);
    fu_signal_connect(app->window, "mouse-move", app_mouse_move_callback, app);
    fu_signal_connect(app->window, "mouse-down", app_mouse_down_callback, app);
    fu_signal_connect(app->window, "mouse-up", app_mouse_up_callback, app);
    fu_signal_connect(app->window, "mouse-press", app_mouse_press_callback, app);
    // fu_signal_connect_with_param(app->window, "scroll", app_scroll_callback, app);
    fu_window_take_source(app->window, &tm);
}

static void on_quit(FUObject* obj, void* usd)
{
    TApp* app = (TApp*)usd;
    printf("%s %s\n", __func__, app->title);
}

int main()
{
    TApp* app = t_app_new();

    fu_signal_connect(app->app, "active", (FUCallback)on_active, app);
    fu_signal_connect(app->app, "quit", (FUCallback)on_quit, app);

    fu_application_run(app->app);

    t_app_free(app);

    printf("bye\n");
    return 0;
}