#include <minwindef.h>
#include <string.h>
#define _use_vk
#ifdef _use_gl
#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "GLFW/glfw3.h"

#define DEF_WIN_WIDTH 800
#define DEF_WIN_HEIGHT 600
#include "res.h"
// static const char* vertexShaderSource = "#version 330 core\n"
//                                         "layout (location = 0) in vec3 aPos;\n"
//                                         "void main()\n"
//                                         "{\n"
//                                         "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
//                                         "}\0";
static const char* vertexShaderSource = "#version 460 core\n"
                                        "layout (location = 0) in vec3 aPos;\n"
                                        "uniform mat4 transform;\n"
                                        "void main()\n"
                                        "{\n"
                                        "   gl_Position = transform * vec4(aPos, 1.0);\n"
                                        "}\0";
static const char* fragmentShaderSource = "#version 460 core\n"
                                          "out vec4 FragColor;\n"
                                          "void main()\n"
                                          "{\n"
                                          "   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
                                          "}\n\0";

void draw_triangle()
{
    // build and compile our shader program
    // ------------------------------------
    // vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    // check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        printf("ERROR::SHADER::VERTEX::COMPILATION_FAILED\n%s\n", infoLog);
    }
    // fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    // check for shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        printf("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n%s\n", infoLog);
    }
    // link shaders
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        printf("ERROR::SHADER::PROGRAM::COMPILATION_FAILED\n%s\n", infoLog);
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    unsigned int VBO, VAO;
    float vertices[] = {
        -0.5f, -0.5f, 0.0f, // left
        0.5f, -0.5f, 0.0f, // right
        0.0f, 0.5f, 0.0f // top
    };
    float angle = (float)glfwGetTime(); // Rotate over time
    float cosTheta = cos(angle);
    float sinTheta = sin(angle);

    float transform[] = {
        cosTheta, -sinTheta, 0.0f, 0.0f,
        sinTheta, cosTheta, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    int transformLoc = glGetUniformLocation(shaderProgram, "transform");

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    glBindVertexArray(0);
    glUseProgram(shaderProgram);
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, transform);

    glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

int main(void)
{
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(DEF_WIN_WIDTH, DEF_WIN_HEIGHT, "[glad] GL with GLFW", NULL, NULL);
    glfwMakeContextCurrent(window);

    glfwSetKeyCallback(window, key_callback);

    int version = gladLoadGL(glfwGetProcAddress);
    printf("GL %d.%d\n", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        glClearColor(0.1f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        draw_triangle();

        glfwSwapBuffers(window);
    }

    glfwTerminate();

    return 0;
}

#endif

#ifdef _use_vk
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <mimalloc.h>
#include <stdio.h>
#include <stdlib.h>

#define DEF_WIN_WIDTH 800
#define DEF_WIN_HEIGHT 600
#define DEF_WIN_TITLE "Vulkan Glfw Test"

#define fu_vk_return_if_fail(expr, ec)                                             \
    do {                                                                           \
        if (VK_SUCCESS != (expr)) {                                                \
            fprintf(stderr, "Vulkan Error %d at %s:%d\n", ec, __FILE__, __LINE__); \
            return;                                                                \
        }                                                                          \
    } while (0)
#define fu_vk_return_val_if_fail(expr, ec, val)                                    \
    do {                                                                           \
        if (VK_SUCCESS != (expr)) {                                                \
            fprintf(stderr, "Vulkan Error %d at %s:%d\n", ec, __FILE__, __LINE__); \
            return val;                                                            \
        }                                                                          \
    } while (0)

typedef struct _TCommandBuffer {
    VkDevice device;
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    uint32_t commandBufferCount;
    VkSemaphore* imageAvailableSemaphore;
    VkSemaphore* renderFinishedSemaphore;
    VkFence inFlightFences;
} TCommandBuffer;

typedef struct _TRendererPass {
    VkDevice device;
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    VkFramebuffer* framebuffers;
    uint32_t framebuffersCount;
} TRendererPass;

typedef struct _TSwapchain {
    VkDevice device;
    VkSwapchainKHR swapchain;
    VkExtent2D* extent;
    VkImage* images;
    VkImageView* imageViews;

    uint32_t imageCount;
    VkSurfaceFormatKHR format;
} TSwapchain;

typedef struct _TPhysicalDriver {
    VkPhysicalDevice* drivers;
    VkPhysicalDeviceProperties* properties;
    VkQueueFamilyProperties** queues;
    uint32_t* queueCount;
    uint32_t driverCount;
    int selectedDeviceIndex;
    int selectedQueueIndex;
    VkDevice driver;
    VkQueue queue;
} TPhysicalDriver;

typedef struct _TApp {
    VkInstance instance;
    VkExtensionProperties* extensions;
    uint32_t extensionCount;
    VkLayerProperties* layers;
    uint32_t layerCnt;

    GLFWwindow* window;
    VkSurfaceKHR surface;

    VkDebugUtilsMessengerEXT debugMessenger;
    TPhysicalDriver* physicalDriver;
    TSwapchain* swapchain;
    TRendererPass* renderPass;
    TCommandBuffer* commandBuffer;
    uint32_t currentFrame;
} TApp;

const int MAX_FRAMES_IN_FLIGHT = 3;

static uint32_t load_shader(const char* path, char** dest)
{
    FILE* file = fopen(path, "rb");
    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    fseek(file, 0, SEEK_SET);
    *dest = mi_malloc(size);
    fread(*dest, 1, size, file);

    fclose(file);
    return size;
}

#ifndef NODEBUG
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    fprintf(stderr, "validation layer: %s\n", pCallbackData->pMessage);
    return VK_FALSE;
}

static bool t_app_init_debug_info(TApp* app)
{
    PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(app->instance, "vkCreateDebugUtilsMessengerEXT");
    if (!func) {
        fprintf(stderr, "vkCreateDebugUtilsMessengerEXT not found\n");
        return false;
    }
    VkDebugUtilsMessengerCreateInfoEXT createInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debugCallback,
        .pUserData = app
    };
    VkResult ret;
    fu_vk_return_val_if_fail(ret = func(app->instance, &createInfo, NULL, &app->debugMessenger), ret, false);
    return true;
}

#endif

static void t_app_command_buffer_free(TCommandBuffer* commandBuffer)
{
    // vkFreeCommandBuffers(commandBuffer->device, commandBuffer->commandPool, commandBuffer->commandBufferCount, &commandBuffer->commandBuffer);
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(commandBuffer->device, commandBuffer->imageAvailableSemaphore[i], NULL);
        vkDestroySemaphore(commandBuffer->device, commandBuffer->renderFinishedSemaphore[i], NULL);
    }
    vkDestroyFence(commandBuffer->device, commandBuffer->inFlightFences, NULL);
    vkDestroyCommandPool(commandBuffer->device, commandBuffer->commandPool, NULL);

    mi_free(commandBuffer->imageAvailableSemaphore);
    mi_free(commandBuffer->renderFinishedSemaphore);
    mi_free(commandBuffer->inFlightFences);
    mi_free(commandBuffer);
}

static TCommandBuffer* t_app_command_buffer_new(TApp* app)
{
    printf("%s\n", __func__);
    VkCommandPoolCreateInfo poolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = app->physicalDriver->selectedQueueIndex
    };
    VkCommandPool commandPool;
    VkResult ret;
    fu_vk_return_val_if_fail(ret = vkCreateCommandPool(app->physicalDriver->driver, &poolCreateInfo, NULL, &commandPool), ret, NULL);
    VkCommandBufferAllocateInfo allocateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1 // app->swapchain->imageCount
    };
    // VkCommandBuffer commandBuffer;
    // fu_vk_return_val_if_fail(ret = vkAllocateCommandBuffers(app->physicalDriver->driver, &allocateInfo, &commandBuffer), ret, NULL);

    TCommandBuffer* rev = mi_malloc(sizeof(TCommandBuffer));
    rev->imageAvailableSemaphore = mi_malloc(MAX_FRAMES_IN_FLIGHT * sizeof(VkSemaphore));
    rev->renderFinishedSemaphore = mi_malloc(MAX_FRAMES_IN_FLIGHT * sizeof(VkSemaphore));
    // rev->inFlightFences = mi_malloc(MAX_FRAMES_IN_FLIGHT * sizeof(VkFence));
    rev->commandBufferCount = app->swapchain->imageCount;
    rev->device = app->physicalDriver->driver;
    rev->commandPool = commandPool;

    // rev->commandBuffer = mi_calloc(rev->commandBufferCount, sizeof(VkCommandBuffer));
    fu_vk_return_val_if_fail(ret = vkAllocateCommandBuffers(app->physicalDriver->driver, &allocateInfo, &rev->commandBuffer), ret, NULL);

    VkSemaphoreCreateInfo semaphoreCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };

    VkFenceCreateInfo fenceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        fu_vk_return_val_if_fail(ret = vkCreateSemaphore(app->physicalDriver->driver, &semaphoreCreateInfo, NULL, &rev->renderFinishedSemaphore[i]), ret, NULL);
        fu_vk_return_val_if_fail(ret = vkCreateSemaphore(app->physicalDriver->driver, &semaphoreCreateInfo, NULL, &rev->imageAvailableSemaphore[i]), ret, NULL);
    }
    fu_vk_return_val_if_fail(ret = vkCreateFence(app->physicalDriver->driver, &fenceCreateInfo, NULL, &rev->inFlightFences), ret, NULL);
    return rev;
}

static void t_app_swapchain_free(TSwapchain* swapchain)
{
    for (uint32_t i = 0; i < swapchain->imageCount; i++) {
        vkDestroyImageView(swapchain->device, swapchain->imageViews[i], NULL);
    }
    vkDestroySwapchainKHR(swapchain->device, swapchain->swapchain, NULL);
    mi_free(swapchain->images);
    mi_free(swapchain->imageViews);
    mi_free(swapchain->extent);
    mi_free(swapchain);
}

static TSwapchain* t_app_swapchain_new(TApp* app)
{
    printf("%s\n", __func__);
    TPhysicalDriver* device = app->physicalDriver;
    VkSurfaceCapabilitiesKHR capabilities;
    VkResult ret;
    fu_vk_return_val_if_fail(ret = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->drivers[device->selectedDeviceIndex], app->surface, &capabilities), ret, NULL);

    uint32_t formatCount;
    fu_vk_return_val_if_fail(ret = vkGetPhysicalDeviceSurfaceFormatsKHR(device->drivers[device->selectedDeviceIndex], app->surface, &formatCount, NULL), ret, NULL);
    VkSurfaceFormatKHR* formats = mi_malloc(sizeof(VkSurfaceFormatKHR) * formatCount);
    fu_vk_return_val_if_fail(ret = vkGetPhysicalDeviceSurfaceFormatsKHR(device->drivers[device->selectedDeviceIndex], app->surface, &formatCount, formats), ret, NULL);

    uint32_t presentModeCount;
    fu_vk_return_val_if_fail(ret = vkGetPhysicalDeviceSurfacePresentModesKHR(device->drivers[device->selectedDeviceIndex], app->surface, &presentModeCount, NULL), ret, NULL);
    VkPresentModeKHR* presentModes = mi_malloc(sizeof(VkPresentModeKHR) * presentModeCount);
    fu_vk_return_val_if_fail(ret = vkGetPhysicalDeviceSurfacePresentModesKHR(device->drivers[device->selectedDeviceIndex], app->surface, &presentModeCount, presentModes), ret, NULL);

    VkSurfaceFormatKHR format = formats[0];
    for (uint32_t i = 0; i < formatCount; i++) {
        if (formats[i].format == VK_FORMAT_R8G8B8A8_SRGB && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            format = formats[i];
            break;
        }
    }

    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (uint32_t i = 0; i < presentModeCount; i++) {
        if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
    }

    TSwapchain* swapchain = mi_malloc(sizeof(TSwapchain));
    swapchain->extent = mi_malloc(sizeof(VkExtent2D));
    swapchain->device = device->driver;
    swapchain->format = format;
    memcpy(swapchain->extent, &capabilities.currentExtent, sizeof(VkExtent2D));

    if (capabilities.currentExtent.width == 0xFFFFFFFF) {
        swapchain->extent->width = min(max(capabilities.minImageExtent.width, DEF_WIN_WIDTH), capabilities.maxImageExtent.width);
        swapchain->extent->height = min(max(capabilities.minImageExtent.height, DEF_WIN_HEIGHT), capabilities.maxImageExtent.height);
    }

    VkSwapchainCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = app->surface,
        .minImageCount = capabilities.minImageCount,
        .imageFormat = format.format,
        .imageColorSpace = format.colorSpace,
        .imageExtent = *swapchain->extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = presentMode,
        .clipped = VK_TRUE,
        .oldSwapchain = 0
    };
    fu_vk_return_val_if_fail(ret = vkCreateSwapchainKHR(device->driver, &createInfo, NULL, &swapchain->swapchain), ret, NULL);

    vkGetSwapchainImagesKHR(swapchain->device, swapchain->swapchain, &swapchain->imageCount, NULL);
    swapchain->images = mi_malloc(sizeof(VkImage) * swapchain->imageCount);
    vkGetSwapchainImagesKHR(swapchain->device, swapchain->swapchain, &swapchain->imageCount, swapchain->images);
    swapchain->imageViews = mi_malloc(sizeof(VkImageView) * swapchain->imageCount);

    VkImageViewCreateInfo viewInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format.format,
        .components = {
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY },
        .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1 }
    };

    for (uint32_t i = 0; i < swapchain->imageCount; i++) { // create views for all images
        viewInfo.image = swapchain->images[i];
        vkCreateImageView(device->driver, &viewInfo, NULL, &swapchain->imageViews[i]);
    }
    return swapchain;
}

static void t_app_physical_device_free(TPhysicalDriver* drivers)
{
    if (!drivers)
        return;
    for (uint32_t i = 0; i < drivers->driverCount; i++) {
        mi_free(drivers->queues[i]);
    }
    mi_free(drivers->drivers);
    mi_free(drivers->queues);
    mi_free(drivers->queueCount);
    mi_free(drivers->properties);
    mi_free(drivers);
}

// static

static TPhysicalDriver* t_app_physical_device_new(TApp* app)
{
    printf("%s\n", __func__);
    uint32_t physicalDeviceCount;
    VkResult ret;
    fu_vk_return_val_if_fail(ret = vkEnumeratePhysicalDevices(app->instance, &physicalDeviceCount, NULL), ret, NULL);
    if (!physicalDeviceCount) {
        fprintf(stderr, "No physical device found\n");
        return NULL;
    }
    TPhysicalDriver* drivers = mi_malloc(sizeof(TPhysicalDriver) * physicalDeviceCount);
    drivers->drivers = mi_malloc(sizeof(VkPhysicalDevice) * physicalDeviceCount);
    drivers->properties = mi_malloc(sizeof(VkPhysicalDeviceProperties) * physicalDeviceCount);
    drivers->queues = mi_malloc(sizeof(VkQueueFamilyProperties*) * physicalDeviceCount);
    drivers->queueCount = mi_malloc(sizeof(uint32_t) * physicalDeviceCount);
    drivers->driverCount = physicalDeviceCount;
    fu_vk_return_val_if_fail(ret = vkEnumeratePhysicalDevices(app->instance, &physicalDeviceCount, drivers->drivers), ret, NULL);

    //
    // 枚举设备
    // 按照设备特征计分
    uint32_t i;
    uint32_t* score = mi_malloc(sizeof(uint32_t) * physicalDeviceCount);
    VkPhysicalDeviceFeatures features;
    for (i = 0; i < physicalDeviceCount; i++) {
        vkGetPhysicalDeviceProperties(drivers->drivers[i], &drivers->properties[i]);
        vkGetPhysicalDeviceFeatures(drivers->drivers[i], &features);
        if (!features.geometryShader)
            continue; // 此特性表示设备支持几何着色器（geometry shaders)

        vkGetPhysicalDeviceQueueFamilyProperties(drivers->drivers[i], &drivers->queueCount[i], NULL);
        drivers->queues[i] = mi_malloc(sizeof(VkQueueFamilyProperties) * drivers->queueCount[i]);
        vkGetPhysicalDeviceQueueFamilyProperties(drivers->drivers[i], &drivers->queueCount[i], drivers->queues[i]);

        score[i] = drivers->queueCount[i] * 10;
        if (features.multiDrawIndirect)
            score[i] += 100; // 当此特性被支持时，表示设备支持多重绘制（多重绘制）
        if (features.robustBufferAccess)
            score[i] += 100; // 当此特性被支持时，即使缓冲区访问越界，应用程序也能保持稳定运行，不会导致崩溃或未定义行为。这为应用程序提供了一定程度的安全性
        if (features.fullDrawIndexUint32)
            score[i] += 100; // 当此特性被支持时，表示设备支持使用32位无符号整数作为索引缓冲区中的索引值，这对于处理大型模型或需要大量索引的场景很有用
        if (features.tessellationShader)
            score[i] += 50; // 此特性表示设备支持细分着色器（tessellation shaders），包括细分控制着色器（tessellation control shaders）和细分评估着色器（tessellation evaluation shaders）。这允许对几何体进行动态细分，用于创建平滑的曲面
        if (features.imageCubeArray)
            score[i] += 50; // 当此特性被支持时，表示设备支持立方体贴图（cube maps）
        if (features.sampleRateShading)
            score[i] += 50; // 当此特性被支持时，表示设备支持采样率着色（sample rate shading）
        if (features.multiViewport)
            score[i] += 10; // 当此特性被支持时，表示设备支持多视口（多视口渲染）
        if (features.independentBlend)
            score[i] += 10; // 当此特性被支持时，表示设备支持独立混合（independent blending），即每个颜色附件可以有独立的混合状态。这对于复杂的渲染技术（如多重渲染目标）非常有用
        if (features.dualSrcBlend)
            score[i] += 10; // 此特性表示设备支持双源混合（dual-source blending），允许使用两个源颜色进行混合操作，这对于某些高级渲染技术（如某些类型的透明度处理）很有用
        if (features.logicOp)
            score[i] += 10; // 当此特性被支持时，表示设备支持逻辑运算（logic operations）
        if (VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU == drivers->properties[i].deviceType)
            score[i] += 500;
        score[i] += drivers->properties[i].limits.maxComputeSharedMemorySize / 3000;
        score[i] += drivers->properties[i].limits.maxImageDimension2D / 1000;
    }
    //
    // Pick the best driver
    uint32_t bestScore = score[0];
    for (i = 0; i < physicalDeviceCount; i++) {
        if (score[i] > bestScore) {
            bestScore = score[i];
            drivers->selectedDeviceIndex = i;
        }
    }
    //
    // Pick the best queue
    drivers->selectedQueueIndex = -1;
    VkBool32 ifSupportPresent = VK_FALSE;
    for (i = 0; i < drivers->queues[drivers->selectedDeviceIndex]->queueCount; i++) {
        if (!(drivers->queues[drivers->selectedDeviceIndex][i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
            continue;
        if (!(drivers->queues[drivers->selectedDeviceIndex][i].queueFlags & VK_QUEUE_COMPUTE_BIT))
            continue;

        vkGetPhysicalDeviceSurfaceSupportKHR(drivers->drivers[drivers->selectedDeviceIndex], i, app->surface, &ifSupportPresent);
        if (!ifSupportPresent)
            continue;
        drivers->selectedQueueIndex = i;
        break;
    }

    if (0 > drivers->selectedQueueIndex) {
        fprintf(stderr, "No suitable queue found\n");
        t_app_physical_device_free(drivers);
        return NULL;
    }
    const float priority = 1.0;
    VkDeviceQueueCreateInfo queueCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = drivers->selectedQueueIndex,
        .queueCount = 1,
        .pQueuePriorities = &priority
    };

    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(drivers->drivers[drivers->selectedDeviceIndex], &deviceFeatures);

    static const char* deviceExtensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    static const char* deviceLayers[] = {
        "VK_LAYER_KHRONOS_validation"
    };

    VkDeviceCreateInfo deviceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueCreateInfo,
        .pEnabledFeatures = &deviceFeatures,
        .enabledExtensionCount = sizeof(deviceExtensions) / sizeof(deviceExtensions[0]),
        .ppEnabledExtensionNames = deviceExtensions,
        .enabledLayerCount = sizeof(deviceLayers) / sizeof(deviceLayers[0]),
        .ppEnabledLayerNames = deviceLayers
    };
    fu_vk_return_val_if_fail(ret = vkCreateDevice(drivers->drivers[drivers->selectedDeviceIndex], &deviceCreateInfo, NULL, &drivers->driver), ret, NULL);
    vkGetDeviceQueue(drivers->driver, drivers->selectedQueueIndex, 0, &drivers->queue);

    printf("Device: %s\n", drivers->properties[drivers->selectedDeviceIndex].deviceName);
    return drivers;
}

static void t_renderer_pass_free(TRendererPass* pass)
{
    for (uint32_t i = 0; i < pass->framebuffersCount; i++) {
        vkDestroyFramebuffer(pass->device, pass->framebuffers[i], NULL);
        // vkDestroyImageView(pass->device, pass->attachments[i].view, NULL);
        // vkDestroyImage(pass->device, pass->attachments[i].image, NULL);
        // vkFreeMemory(pass->device, pass->attachments[i].memory, NULL);
    }
    vkDestroyPipeline(pass->device, pass->graphicsPipeline, NULL);
    vkDestroyPipelineLayout(pass->device, pass->pipelineLayout, NULL);
    vkDestroyRenderPass(pass->device, pass->renderPass, NULL);
    // vkDestroyDevice(pass->device, NULL);
    mi_free(pass);
}

static TRendererPass* t_renderer_pass_new(TApp* app)
{
    printf("%s\n", __func__);
    VkAttachmentDescription colorAttchment = {
        .format = app->swapchain->format.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };
    VkAttachmentReference colorAttchmentRef = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttchmentRef
    };

    VkSubpassDependency dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
    };

    VkRenderPassCreateInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &colorAttchment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    VkResult ret;
    TRendererPass* pass = mi_calloc(1, sizeof(TRendererPass));
    pass->framebuffers = mi_calloc(app->swapchain->imageCount, sizeof(VkFramebuffer));
    pass->framebuffersCount = app->swapchain->imageCount;
    pass->device = app->physicalDriver->driver;

    fu_vk_return_val_if_fail(ret = vkCreateRenderPass(pass->device, &renderPassInfo, NULL, &pass->renderPass), ret, NULL);

    static char* vertShader = NULL; // = load_shader("D:/dev/C/fusion/test/gl1/vert.spv");
    static char* fragShader = NULL; // = load_shader("D:/dev/C/fusion/test/gl1/frag.spv");

    VkShaderModuleCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        // .codeSize = strlen(vertShader),
        .pCode = (uint32_t*)vertShader,
        .codeSize = load_shader("D:/dev/C/fusion/test/gl1/vert.spv", &vertShader)
    };
    // createInfo.pCode = (uint32_t*)vertShader;

    VkShaderModule vertShaderModule;
    fu_vk_return_val_if_fail(ret = vkCreateShaderModule(app->physicalDriver->driver, &createInfo, NULL, &vertShaderModule), ret, NULL);

    createInfo.codeSize = load_shader("D:/dev/C/fusion/test/gl1/frag.spv", &fragShader);
    createInfo.pCode = (uint32_t*)fragShader;
    VkShaderModule fragShaderModule;
    fu_vk_return_val_if_fail(ret = vkCreateShaderModule(app->physicalDriver->driver, &createInfo, NULL, &fragShaderModule), ret, NULL);

    VkPipelineShaderStageCreateInfo vertStage = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vertShaderModule,
        .pName = "main"
    };
    VkPipelineShaderStageCreateInfo fragStage = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fragShaderModule,
        .pName = "main"
    };

    VkPipelineShaderStageCreateInfo shaderStages[] = {
        vertStage,
        fragStage
    };

    VkPipelineVertexInputStateCreateInfo vertexInput = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = NULL,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = NULL
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)app->swapchain->extent->width,
        .height = (float)app->swapchain->extent->height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    VkRect2D scissor = {
        .offset = { 0, 0 },
        .extent = *app->swapchain->extent
    };

    VkPipelineViewportStateCreateInfo viewportState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor
    };

    VkPipelineRasterizationStateCreateInfo rasterizationState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .lineWidth = 1.0f
    };

    VkPipelineMultisampleStateCreateInfo multisampleState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
    };

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };

    VkPipelineColorBlendStateCreateInfo colorBlendState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment
    };

    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = 2,
        .pDynamicStates = dynamicStates
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 0,
        .pSetLayouts = NULL,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = NULL
    };

    fu_vk_return_val_if_fail(ret = vkCreatePipelineLayout(pass->device, &pipelineLayoutInfo, NULL, &pass->pipelineLayout), ret, NULL);

    VkGraphicsPipelineCreateInfo pipelineInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInput,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizationState,
        .pMultisampleState = &multisampleState,
        .pColorBlendState = &colorBlendState,
        .pDynamicState = &dynamicState,
        .layout = pass->pipelineLayout,
        .renderPass = pass->renderPass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };

    fu_vk_return_val_if_fail(ret = vkCreateGraphicsPipelines(pass->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &pass->graphicsPipeline), ret, NULL);

    VkFramebufferCreateInfo framebuffersInfo = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = pass->renderPass,
        .attachmentCount = 1,
        // .pAttachments = app->swapchain->imageViews,
        .width = app->swapchain->extent->width,
        .height = app->swapchain->extent->height,
        .layers = 1
    };
    for (uint32_t i = 0; i < app->swapchain->imageCount; i++) {
        framebuffersInfo.pAttachments = &app->swapchain->imageViews[i];
        fu_vk_return_val_if_fail(ret = vkCreateFramebuffer(pass->device, &framebuffersInfo, NULL, &pass->framebuffers[i]), ret, NULL);
    }

    vkDestroyShaderModule(pass->device, vertShaderModule, NULL);
    vkDestroyShaderModule(pass->device, fragShaderModule, NULL);
    return pass;
}

static void t_app_free(TApp* app)
{
#ifndef NODEBUG
    PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(app->instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func)
        func(app->instance, app->debugMessenger, NULL);
#endif
    t_app_command_buffer_free(app->commandBuffer);
    t_app_swapchain_free(app->swapchain);
    t_renderer_pass_free(app->renderPass);
    vkDestroyDevice(app->physicalDriver->driver, NULL);
    vkDestroySurfaceKHR(app->instance, app->surface, NULL);
    vkDestroyInstance(app->instance, NULL);
    glfwDestroyWindow(app->window);
    glfwTerminate();

    t_app_physical_device_free(app->physicalDriver);
    mi_free(app->extensions);
    mi_free(app->layers);
    mi_free(app);
}

static TApp* t_app_new()
{
    printf("%s\n", __func__);
    glfwInit();
    if (!glfwVulkanSupported()) {
        fprintf(stderr, "Vulkan not supported\n");
        return NULL;
    }

    if (!vkGetInstanceProcAddr(NULL, "vkEnumerateInstanceVersion")) {
        fprintf(stderr, "Vulkan version too old\n");
        return NULL;
    }

    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = DEF_WIN_TITLE,
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "Fusion VK",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        // .apiVersion = VK_API_VERSION_1_3
    };

    VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .ppEnabledExtensionNames = glfwGetRequiredInstanceExtensions(&createInfo.enabledExtensionCount)
    };
    vkEnumerateInstanceVersion(&appInfo.apiVersion);
    TApp* app = (TApp*)mi_malloc(sizeof(TApp));

    vkEnumerateInstanceExtensionProperties(NULL, &app->extensionCount, NULL);
    app->extensions = mi_malloc(app->extensionCount * sizeof(VkExtensionProperties));
    vkEnumerateInstanceExtensionProperties(NULL, &app->extensionCount, app->extensions);

    vkEnumerateInstanceLayerProperties(&app->layerCnt, NULL);
    app->layers = mi_malloc(app->layerCnt * sizeof(VkLayerProperties));
    vkEnumerateInstanceLayerProperties(&app->layerCnt, app->layers);

#ifndef NODEBUG
    static const char* enableLayers[] = {
        "VK_LAYER_KHRONOS_validation"
    };
    static const char* enableExtensions[] = {
        "VK_KHR_surface",
        "VK_EXT_debug_utils",
#ifdef __WIN64
        "VK_KHR_win32_surface",
#endif
    };
    // memcpy(enableExtensions[1], createInfo.ppEnabledExtensionNames, createInfo.enabledExtensionCount - 1);

    createInfo.ppEnabledExtensionNames = enableExtensions;
    createInfo.enabledExtensionCount = sizeof(enableExtensions) / sizeof(char*);
    createInfo.ppEnabledLayerNames = enableLayers;
    createInfo.enabledLayerCount = sizeof(enableLayers) / sizeof(char*);

    printf("VK %d.%d\nAvailable extensions:\n", VK_VERSION_MAJOR(appInfo.apiVersion), VK_VERSION_MINOR(appInfo.apiVersion));
    for (uint32_t i = 0; i < app->extensionCount; i++) {
        printf("\t%s\n", app->extensions[i].extensionName);
    }

    printf("Available layers:\n");
    for (uint32_t i = 0; i < app->layerCnt; i++) {
        printf("\t%s\n", app->layers[i].layerName);
    }
    VkResult ret;
    fu_vk_return_val_if_fail(ret = vkCreateInstance(&createInfo, NULL, &app->instance), ret, NULL);
    t_app_init_debug_info(app);
#else
    VkResult ret;
    fu_vk_return_val_if_fail(ret = vkCreateInstance(&createInfo, NULL, &app->instance), ret, NULL);
#endif

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    app->window = glfwCreateWindow(DEF_WIN_WIDTH, DEF_WIN_HEIGHT, DEF_WIN_TITLE, NULL, NULL);

    fu_vk_return_val_if_fail(ret = glfwCreateWindowSurface(app->instance, app->window, NULL, &app->surface), ret, NULL);
    glfwSetWindowUserPointer(app->window, app);
    app->physicalDriver = t_app_physical_device_new(app);
    app->swapchain = t_app_swapchain_new(app);
    app->renderPass = t_renderer_pass_new(app);
    app->commandBuffer = t_app_command_buffer_new(app);
    app->currentFrame = 0;
    return app;
}

static void t_command_buffer_record(TApp* app, uint32_t imageIndex)
{
    // printf("%s\n", __func__);
    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT
    };

    VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    VkRenderPassBeginInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = app->renderPass->renderPass,
        .renderArea.offset = { 0, 0 },
        .renderArea.extent = *app->swapchain->extent,
        .clearValueCount = 1,
        .pClearValues = &clearColor,
        .framebuffer = app->renderPass->framebuffers[imageIndex],
    };

    vkBeginCommandBuffer(app->commandBuffer->commandBuffer, &beginInfo);
    vkCmdBeginRenderPass(app->commandBuffer->commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(app->commandBuffer->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->renderPass->graphicsPipeline);

    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)app->swapchain->extent->width,
        .height = (float)app->swapchain->extent->height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    VkRect2D scissor = {
        .offset = { 0, 0 },
        .extent = *app->swapchain->extent
    };

    vkCmdSetViewport(app->commandBuffer->commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(app->commandBuffer->commandBuffer, 0, 1, &scissor);

    vkCmdDraw(app->commandBuffer->commandBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(app->commandBuffer->commandBuffer);
    vkEndCommandBuffer(app->commandBuffer->commandBuffer);
}

static void t_draw(TApp* app)
{
    uint32_t imageIndex;
    vkWaitForFences(app->commandBuffer->device, 1, &app->commandBuffer->inFlightFences, VK_TRUE, UINT64_MAX);
    vkResetFences(app->commandBuffer->device, 1, &app->commandBuffer->inFlightFences);
    vkAcquireNextImageKHR(app->commandBuffer->device, app->swapchain->swapchain, UINT64_MAX, app->commandBuffer->imageAvailableSemaphore[app->currentFrame], VK_NULL_HANDLE, &imageIndex);

    vkResetCommandBuffer(app->commandBuffer->commandBuffer, 0);
    t_command_buffer_record(app, imageIndex);

    VkSemaphore waitSemaphores[] = { app->commandBuffer->imageAvailableSemaphore[app->currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    VkSemaphore signalSemaphores[] = { app->commandBuffer->renderFinishedSemaphore[app->currentFrame] };

    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = waitSemaphores,
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &app->commandBuffer->commandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = signalSemaphores
    };

    VkResult ret;
    fu_vk_return_if_fail(ret = vkQueueSubmit(app->physicalDriver->queue, 1, &submitInfo, app->commandBuffer->inFlightFences), ret);

    VkSwapchainKHR swapchains[] = { app->swapchain->swapchain };
    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signalSemaphores,
        .swapchainCount = 1,
        .pSwapchains = swapchains,
        .pImageIndices = &imageIndex,
    };

    vkQueuePresentKHR(app->physicalDriver->queue, &presentInfo);

    app->currentFrame = (app->currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

int main()
{
    TApp* app = t_app_new();

    while (!glfwWindowShouldClose(app->window)) {
        glfwPollEvents();
        t_draw(app);
    }
    vkDeviceWaitIdle(app->physicalDriver->driver);
    t_app_free(app);

    printf("Bye!\n");
    return 0;
}

#endif