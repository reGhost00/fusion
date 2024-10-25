#include <stdarg.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <mimalloc.h>
#include <minwindef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEF_WIN_WIDTH 800
#define DEF_WIN_HEIGHT 600
#define DEF_WIN_TITLE "Vulkan Glfw Test"

typedef struct _TVertex {
    float position[3];
    float color[3];
} TVertex;

typedef struct _TVertexBuffer {
    TVertex* vertices;
    uint32_t* indices;
    uint32_t vertexCount;
    uint32_t indexCount;
    VkDevice device;
    // 保持这个顺序以便复制
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexMemory;
    // 保持这个顺序以便复制
    VkBuffer indexBuffer;
    VkDeviceMemory indexMemory;
} TVertexBuffer;

typedef struct _TApp {
    GLFWwindow* window;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;

    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice;

    int selectedQueueIndex;
    VkDevice logicalDevice;
    VkQueue queue;

    VkFormat swapchainImageFormat;
    VkExtent2D swapchainExtent;
    VkSwapchainKHR swapchain;
    VkImage* swapchainImages;
    VkImageView* swapchainImageViews;
    uint32_t swapchainImageCount;

    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    VkFramebuffer* framebuffers;
    TVertexBuffer* vertexBuffer;
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;

    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFences;
} TApp;

static PFN_vkVoidFunction vk_get_func(VkInstance instance, const char* name)
{
    PFN_vkVoidFunction rev = vkGetInstanceProcAddr(instance, name);
    if (!rev) {
        fprintf(stderr, "vkGetInstanceProcAddr failed for %s\n", name);
        abort();
    }
    return rev;
}

static uint32_t load_shader(const char* path, char** dest)
{
    FILE* file = fopen(path, "rb");
    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    fseek(file, 0, SEEK_SET);
    *dest = (char*)mi_malloc(size);
    fread(*dest, 1, size, file);

    fclose(file);
    return size;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL t_app_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    fprintf(stderr, "validation layer: %s\n", pCallbackData->pMessage);
    return VK_FALSE;
}

static uint32_t find_memory_type(VkPhysicalDevice device, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(device, &memoryProperties);
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }

    fprintf(stderr, "failed to find suitable memory type!\n");
    abort();
}

typedef struct _TBufferArgs {
    TApp* app;
    VkDeviceSize size;
    VkBufferUsageFlags usage;
    VkMemoryPropertyFlags properties;
} TBufferArgs;

typedef struct _TBufferResult {
    VkBuffer buffer;
    VkDeviceMemory memory;
} TBufferResult;

static bool t_buffer_init(TBufferArgs* args, TBufferResult* result)
{
    VkBufferCreateInfo bufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = args->size,
        .usage = args->usage
    };

    if (vkCreateBuffer(args->app->logicalDevice, &bufferInfo, NULL, &result->buffer) != VK_SUCCESS) {
        fprintf(stderr, "failed to create buffer!\n");
        return false;
    }
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(args->app->logicalDevice, result->buffer, &memRequirements);
    VkMemoryAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = find_memory_type(args->app->physicalDevice, memRequirements.memoryTypeBits, args->properties)
    };

    if (vkAllocateMemory(args->app->logicalDevice, &allocInfo, NULL, &result->memory) != VK_SUCCESS) {
        fprintf(stderr, "failed to allocate buffer memory!\n");
        vkDestroyBuffer(args->app->logicalDevice, result->buffer, NULL);
        return false;
    }
    vkBindBufferMemory(args->app->logicalDevice, result->buffer, result->memory, 0);
    args->size = memRequirements.size;
    return true;
}

static void t_buffer_copy(VkBuffer src, VkBuffer dst, VkDeviceSize size, TApp* app)
{
    VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = app->commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(app->logicalDevice, &allocInfo, &commandBuffer);
    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    VkBufferCopy copyRegion = {
        .size = size
    };
    vkCmdCopyBuffer(commandBuffer, src, dst, 1, &copyRegion);
    vkEndCommandBuffer(commandBuffer);
    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer
    };

    vkQueueSubmit(app->queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(app->queue);
    vkFreeCommandBuffers(app->logicalDevice, app->commandPool, 1, &commandBuffer);
}

static void t_buffer_destroy(TBufferResult* result, TApp* app)
{
    vkDestroyBuffer(app->logicalDevice, result->buffer, NULL);
    vkFreeMemory(app->logicalDevice, result->memory, NULL);
}

static TVertexBuffer* t_vertex_buffer_new(TApp* app)
{
    TVertexBuffer* vertexBuffer = (TVertexBuffer*)mi_malloc(sizeof(TVertexBuffer));
    vertexBuffer->vertices = (TVertex*)mi_malloc(sizeof(TVertex) * 4);
    vertexBuffer->indices = (uint32_t*)mi_malloc(sizeof(uint32_t) * 6);
    vertexBuffer->vertexCount = 4;
    vertexBuffer->indexCount = 6;
    vertexBuffer->device = app->logicalDevice;

    vertexBuffer->vertices[0] = (TVertex) { { -0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f } };
    vertexBuffer->vertices[1] = (TVertex) { { 0.5f, -0.5f, 0.0f }, { 1.0f, 1.0f, 0.0f } };
    vertexBuffer->vertices[2] = (TVertex) { { 0.5f, 0.5f, 0.0f }, { 0.0f, 1.0f, 1.0f } };
    vertexBuffer->vertices[3] = (TVertex) { { -0.5f, 0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f } };

    vertexBuffer->indices[0] = 0;
    vertexBuffer->indices[1] = 3;
    vertexBuffer->indices[2] = 2;
    vertexBuffer->indices[3] = 0;
    vertexBuffer->indices[4] = 2;
    vertexBuffer->indices[5] = 1;

    TBufferArgs bufferArgs = {
        .app = app,
        .size = sizeof(TVertex) * vertexBuffer->vertexCount,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    };
    TBufferResult stagingBuffer;
    if (!t_buffer_init(&bufferArgs, &stagingBuffer)) {
        fprintf(stderr, "failed to create vertex buffer!\n");
        goto out;
    }

    void* data;
    vkMapMemory(app->logicalDevice, stagingBuffer.memory, 0, bufferArgs.size, 0, &data);
    memcpy(data, vertexBuffer->vertices, (size_t)bufferArgs.size);
    vkUnmapMemory(app->logicalDevice, stagingBuffer.memory);

    bufferArgs.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferArgs.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    if (!t_buffer_init(&bufferArgs, (TBufferResult*)&vertexBuffer->vertexBuffer)) {
        fprintf(stderr, "failed to create vertex buffer!\n");
        goto out;
    }

    t_buffer_copy(stagingBuffer.buffer, vertexBuffer->vertexBuffer, sizeof(TVertex) * vertexBuffer->vertexCount, app);
    t_buffer_destroy(&stagingBuffer, app);

    bufferArgs.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferArgs.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    bufferArgs.size = sizeof(uint32_t) * vertexBuffer->indexCount;
    if (!t_buffer_init(&bufferArgs, &stagingBuffer)) {
        fprintf(stderr, "failed to create vertex buffer!\n");
        goto out;
    }

    vkMapMemory(app->logicalDevice, stagingBuffer.memory, 0, bufferArgs.size, 0, &data);
    memcpy(data, vertexBuffer->indices, (size_t)bufferArgs.size);
    vkUnmapMemory(app->logicalDevice, stagingBuffer.memory);

    bufferArgs.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    bufferArgs.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    if (!t_buffer_init(&bufferArgs, (TBufferResult*)&vertexBuffer->indexBuffer)) {
        fprintf(stderr, "failed to create vertex buffer!\n");
        goto out;
    }

    t_buffer_copy(stagingBuffer.buffer, vertexBuffer->indexBuffer, sizeof(uint32_t) * vertexBuffer->indexCount, app);
    t_buffer_destroy(&stagingBuffer, app);

    return vertexBuffer;
out:
    mi_free(vertexBuffer->vertices);
    mi_free(vertexBuffer);
    return NULL;
}

static void t_vertex_buffer_free(TVertexBuffer* buffer)
{
    vkDestroyBuffer(buffer->device, buffer->vertexBuffer, NULL);
    vkDestroyBuffer(buffer->device, buffer->indexBuffer, NULL);
    vkFreeMemory(buffer->device, buffer->vertexMemory, NULL);
    vkFreeMemory(buffer->device, buffer->indexMemory, NULL);
    mi_free(buffer->vertices);
    mi_free(buffer);
}

static void init_vertex_description(TVertexBuffer* buffer, VkPipelineVertexInputStateCreateInfo* description)
{
    const static VkVertexInputBindingDescription bindingDesc = {
        .stride = sizeof(TVertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };
    const static VkVertexInputAttributeDescription position = {
        .format = VK_FORMAT_R32G32B32_SFLOAT
    };
    const static VkVertexInputAttributeDescription color = {
        .location = 1,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(TVertex, color)
    };
    const static VkVertexInputAttributeDescription attributes[] = {
        position,
        color
    };
    description->vertexBindingDescriptionCount = 1;
    description->pVertexBindingDescriptions = &bindingDesc;
    description->vertexAttributeDescriptionCount = 2;
    description->pVertexAttributeDescriptions = attributes;
}

static void t_app_init_device(TApp* app)
{
    static const char* defFeatures[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    if (glfwCreateWindowSurface(app->instance, app->window, NULL, &app->surface)) {
        fprintf(stderr, "failed to create window surface!\n");
        abort();
    }
    //
    // pick physical device

    uint32_t deviceCount;
    VkPhysicalDeviceProperties properties;

    vkEnumeratePhysicalDevices(app->instance, &deviceCount, NULL);
    VkPhysicalDevice* devices = (VkPhysicalDevice*)mi_malloc(sizeof(VkPhysicalDevice) * deviceCount);
    vkEnumeratePhysicalDevices(app->instance, &deviceCount, devices);

    for (uint32_t i = 0; i < deviceCount; i++) {
        vkGetPhysicalDeviceProperties(devices[i], &properties);
        if (properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            continue;
        // todo: pick suitable device
        uint32_t queueCount;
        VkQueueFamilyProperties* queues;
        VkBool32 ifSupportPresent = VK_FALSE;

        vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &queueCount, NULL);
        queues = (VkQueueFamilyProperties*)mi_malloc(sizeof(VkQueueFamilyProperties) * queueCount);
        vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &queueCount, queues);

        app->physicalDevice = devices[i];
        app->selectedQueueIndex = -1;
        for (i = 0; i < queueCount; i++) {
            if (!(queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
                continue;
            if (!(queues[i].queueFlags & VK_QUEUE_COMPUTE_BIT))
                continue;

            vkGetPhysicalDeviceSurfaceSupportKHR(devices[i], i, app->surface, &ifSupportPresent);
            if (!ifSupportPresent)
                continue;
            app->selectedQueueIndex = i;
        }
        mi_free(queues);
        break;
    }
    if (!app->physicalDevice || 0 > app->selectedQueueIndex) {
        fprintf(stderr, "failed to find a suitable GPU!\n");
        abort();
    }

    printf("Device: %s\n", properties.deviceName);
    // return;
    const float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = (uint32_t)app->selectedQueueIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority
    };
    VkDeviceCreateInfo deviceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueCreateInfo,
        .enabledExtensionCount = sizeof(defFeatures) / sizeof(char*),
        .ppEnabledExtensionNames = defFeatures
    };
    if (vkCreateDevice(app->physicalDevice, &deviceCreateInfo, NULL, &app->logicalDevice)) {
        fprintf(stderr, "failed to create logical device!\n");
        abort();
    }
    vkGetDeviceQueue(app->logicalDevice, app->selectedQueueIndex, 0, &app->queue);

    // app->vertexBuffer = t_vertex_buffer_new(app);
    mi_free(devices);
}

static void t_app_init_swapchain(TApp* app)
{
    printf("%s\n", __func__);
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(app->physicalDevice, app->surface, &capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(app->physicalDevice, app->surface, &formatCount, NULL);
    VkSurfaceFormatKHR* formats = mi_malloc(sizeof(VkSurfaceFormatKHR) * formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(app->physicalDevice, app->surface, &formatCount, formats);

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(app->physicalDevice, app->surface, &presentModeCount, NULL);
    VkPresentModeKHR* presentModes = mi_malloc(sizeof(VkPresentModeKHR) * presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(app->physicalDevice, app->surface, &presentModeCount, presentModes);

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

    do {
        if (capabilities.currentExtent.width != ~0UL)
            app->swapchainExtent = capabilities.currentExtent;
        else {
            glfwGetFramebufferSize(app->window, (int*)&app->swapchainExtent.width, (int*)&app->swapchainExtent.height);

            app->swapchainExtent.width = max(min(app->swapchainExtent.width, capabilities.maxImageExtent.width), capabilities.minImageExtent.width);
            app->swapchainExtent.height = max(min(app->swapchainExtent.height, capabilities.maxImageExtent.height), capabilities.minImageExtent.height);
        }

        if (0 < app->swapchainExtent.width && 0 < app->swapchainExtent.height)
            break;
        glfwWaitEvents();
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(app->physicalDevice, app->surface, &capabilities);
    } while (true);

    app->swapchainImageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && app->swapchainImageCount > capabilities.maxImageCount) {
        app->swapchainImageCount = capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapchainCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = app->surface,
        .minImageCount = app->swapchainImageCount,
        .imageFormat = format.format,
        .imageColorSpace = format.colorSpace,
        .imageExtent = app->swapchainExtent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = presentMode,
        .clipped = VK_TRUE
    };
    if (vkCreateSwapchainKHR(app->logicalDevice, &swapchainCreateInfo, NULL, &app->swapchain)) {
        fprintf(stderr, "failed to create swapchain!\n");
        abort();
    }

    vkGetSwapchainImagesKHR(app->logicalDevice, app->swapchain, &app->swapchainImageCount, NULL);
    app->swapchainImages = mi_malloc(sizeof(VkImage) * app->swapchainImageCount);
    vkGetSwapchainImagesKHR(app->logicalDevice, app->swapchain, &app->swapchainImageCount, app->swapchainImages);
    app->swapchainImageViews = mi_malloc(sizeof(VkImageView) * app->swapchainImageCount);
    VkImageViewCreateInfo viewCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format.format,
        .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.baseMipLevel = 0,
        .subresourceRange.levelCount = 1,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount = 1,
    };

    for (uint32_t i = 0; i < app->swapchainImageCount; i++) {
        viewCreateInfo.image = app->swapchainImages[i];
        if (vkCreateImageView(app->logicalDevice, &viewCreateInfo, NULL, &app->swapchainImageViews[i]) != VK_SUCCESS) {
            fprintf(stderr, "failed to create image views!\n");
            abort();
        }
    }

    app->swapchainImageFormat = format.format;
    mi_free(formats);
    mi_free(presentModes);
}

static void t_app_init_render_pass(TApp* app)
{
    printf("%s\n", __func__);
    VkAttachmentDescription colorAttachment = {
        .format = app->swapchainImageFormat,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    VkAttachmentReference colorAttachmentRef = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef,
    };

    VkSubpassDependency dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };

    VkRenderPassCreateInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &colorAttachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    if (vkCreateRenderPass(app->logicalDevice, &renderPassInfo, NULL, &app->renderPass) != VK_SUCCESS) {
        fprintf(stderr, "failed to create render pass!\n");
        abort();
    }
}

static void t_app_init_pipeline(TApp* app)
{
    printf("%s\n", __func__);
    char *vertexShaderSource, *fragmentShaderSource;
    const uint32_t vertexShaderSize = load_shader("D:/dev/C/fusion/test/gl1/vert.spv", &vertexShaderSource);
    const uint32_t fragmentShaderSize = load_shader("D:/dev/C/fusion/test/gl1/frag.spv", &fragmentShaderSource);

    VkShaderModule vertexShaderModule, fragmentShaderModule;
    VkShaderModuleCreateInfo shaderCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = vertexShaderSize,
        .pCode = (uint32_t*)vertexShaderSource
    };

    if (vkCreateShaderModule(app->logicalDevice, &shaderCreateInfo, NULL, &vertexShaderModule)) {
        fprintf(stderr, "failed to create vertex shader module!\n");
        abort();
    }

    shaderCreateInfo.codeSize = fragmentShaderSize;
    shaderCreateInfo.pCode = (uint32_t*)fragmentShaderSource;
    if (vkCreateShaderModule(app->logicalDevice, &shaderCreateInfo, NULL, &fragmentShaderModule)) {
        fprintf(stderr, "failed to create fragment shader module!\n");
        abort();
    }

    VkPipelineShaderStageCreateInfo vertexShaderStageInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vertexShaderModule,
        .pName = "main"
    };

    VkPipelineShaderStageCreateInfo fragmentShaderStageInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fragmentShaderModule,
        .pName = "main"
    };

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderStageInfo, fragmentShaderStageInfo };

    VkPipelineVertexInputStateCreateInfo vertexInputState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    };
    init_vertex_description(app->vertexBuffer, &vertexInputState);

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
    };

    VkPipelineViewportStateCreateInfo viewportState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1
    };

    VkPipelineRasterizationStateCreateInfo rasterizationState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .lineWidth = 1.0f
    };

    VkPipelineMultisampleStateCreateInfo multisampleState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
    };

    VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };

    VkPipelineColorBlendStateCreateInfo colorBlendState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachmentState,
        .logicOp = VK_LOGIC_OP_COPY
    };

    VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = 2,
        .pDynamicStates = dynamicStates
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO
    };

    if (vkCreatePipelineLayout(app->logicalDevice, &pipelineLayoutInfo, NULL, &app->pipelineLayout) != VK_SUCCESS) {
        fprintf(stderr, "failed to create pipeline layout!\n");
        abort();
    }

    VkGraphicsPipelineCreateInfo pipelineInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputState,
        .pInputAssemblyState = &inputAssemblyState,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizationState,
        .pMultisampleState = &multisampleState,
        .pColorBlendState = &colorBlendState,
        .pDynamicState = &dynamicState,
        .layout = app->pipelineLayout,
        .renderPass = app->renderPass
    };

    if (vkCreateGraphicsPipelines(app->logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &app->pipeline) != VK_SUCCESS) {
        fprintf(stderr, "failed to create graphics pipeline!\n");
        abort();
    }

    vkDestroyShaderModule(app->logicalDevice, vertexShaderModule, NULL);
    vkDestroyShaderModule(app->logicalDevice, fragmentShaderModule, NULL);
    mi_free(vertexShaderSource);
    mi_free(fragmentShaderSource);
}

// frameBuffer & commandPool & commandBuffer
static void t_app_init_buffer(TApp* app)
{
    app->framebuffers = mi_calloc(app->swapchainImageCount, sizeof(VkFramebuffer));
    VkFramebufferCreateInfo framebufferInfo = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = app->renderPass,
        .attachmentCount = 1,
        .width = app->swapchainExtent.width,
        .height = app->swapchainExtent.height,
        .layers = 1
    };

    for (uint32_t i = 0; i < app->swapchainImageCount; i++) {
        framebufferInfo.pAttachments = &app->swapchainImageViews[i];
        if (vkCreateFramebuffer(app->logicalDevice, &framebufferInfo, NULL, &app->framebuffers[i]) != VK_SUCCESS) {
            fprintf(stderr, "failed to create framebuffer!\n");
            abort();
        }
    }

    VkCommandPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = (uint32_t)app->selectedQueueIndex
    };
    if (vkCreateCommandPool(app->logicalDevice, &poolInfo, NULL, &app->commandPool) != VK_SUCCESS) {
        fprintf(stderr, "failed to create command pool!\n");
        abort();
    }

    VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = app->commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1 // app->swapchainImageCount
    };
    if (vkAllocateCommandBuffers(app->logicalDevice, &allocInfo, &app->commandBuffer) != VK_SUCCESS) {
        fprintf(stderr, "failed to allocate command buffers!\n");
        abort();
    }
    app->vertexBuffer = t_vertex_buffer_new(app);
}

static void t_app_init_sync(TApp* app)
{
    VkSemaphoreCreateInfo semaphoreInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };

    VkFenceCreateInfo fenceInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    VkResult ret = vkCreateSemaphore(app->logicalDevice, &semaphoreInfo, NULL, &app->imageAvailableSemaphore);
    ret = (VkResult)(!ret && vkCreateSemaphore(app->logicalDevice, &semaphoreInfo, NULL, &app->renderFinishedSemaphore));
    ret = (VkResult)(!ret && vkCreateFence(app->logicalDevice, &fenceInfo, NULL, &app->inFlightFences));

    if (ret) {
        fprintf(stderr, "failed to create semaphores!\n");
        abort();
    }
}

static void t_app_destroy_device(TApp* app)
{
    // t_vertex_buffer_free(app->vertexBuffer);
    vkDestroyDevice(app->logicalDevice, NULL);
    vkDestroySurfaceKHR(app->instance, app->surface, NULL);
}

static void t_app_destroy_swapchain(TApp* app)
{
    for (uint32_t i = 0; i < app->swapchainImageCount; i++) {
        vkDestroyImageView(app->logicalDevice, app->swapchainImageViews[i], NULL);
    }
    vkDestroySwapchainKHR(app->logicalDevice, app->swapchain, NULL);
    mi_free(app->swapchainImages);
    mi_free(app->swapchainImageViews);
}

static void t_app_destroy_render_pass(TApp* app)
{
    vkDestroyRenderPass(app->logicalDevice, app->renderPass, NULL);
}

static void t_app_destroy_pipeline(TApp* app)
{
    vkDestroyPipeline(app->logicalDevice, app->pipeline, NULL);
    vkDestroyPipelineLayout(app->logicalDevice, app->pipelineLayout, NULL);
}

static void t_app_destroy_buffer(TApp* app)
{
    t_vertex_buffer_free(app->vertexBuffer);
    for (uint32_t i = 0; i < app->swapchainImageCount; i++) {
        vkDestroyFramebuffer(app->logicalDevice, app->framebuffers[i], NULL);
    }

    vkDestroyCommandPool(app->logicalDevice, app->commandPool, NULL);
    mi_free(app->framebuffers);
}

static void t_app_destroy_sync(TApp* app)
{
    vkDestroySemaphore(app->logicalDevice, app->imageAvailableSemaphore, NULL);
    vkDestroySemaphore(app->logicalDevice, app->renderFinishedSemaphore, NULL);
    vkDestroyFence(app->logicalDevice, app->inFlightFences, NULL);
}

static TApp* t_app_new()
{
    static const char* defExtensions[] = { "VK_KHR_surface", "VK_KHR_win32_surface", "VK_EXT_debug_utils" };
    static const char* defLayers[] = { "VK_LAYER_KHRONOS_validation" };
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    TApp* app = mi_malloc(sizeof(TApp));
    app->window = glfwCreateWindow(DEF_WIN_WIDTH, DEF_WIN_HEIGHT, DEF_WIN_TITLE, NULL, NULL);
    //
    // init vulkan instance
    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = DEF_WIN_TITLE,
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "Fusion VK",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_3
    };

    VkInstanceCreateInfo instanceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        // .ppEnabledExtensionNames = glfwGetRequiredInstanceExtensions(&createInfo.enabledExtensionCount),
        .ppEnabledExtensionNames = defExtensions,
        .enabledExtensionCount = sizeof(defExtensions) / sizeof(char*),
        .ppEnabledLayerNames = defLayers,
        .enabledLayerCount = sizeof(defLayers) / sizeof(char*)
    };

    if (vkCreateInstance(&instanceCreateInfo, NULL, &app->instance)) {
        printf("Vulkan not supported\n");
        return NULL;
    }

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = t_app_debug_callback
    };

    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vk_get_func(app->instance, "vkCreateDebugUtilsMessengerEXT");
    if (vkCreateDebugUtilsMessengerEXT(app->instance, &debugCreateInfo, NULL, &app->debugMessenger)) {
        printf("Debug messenger not supported\n");
        return NULL;
    }

    return app;
}

static void t_app_free(TApp* app)
{
    t_app_destroy_sync(app);
    t_app_destroy_buffer(app);
    t_app_destroy_pipeline(app);
    t_app_destroy_render_pass(app);
    t_app_destroy_swapchain(app);
    t_app_destroy_device(app);
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vk_get_func(app->instance, "vkDestroyDebugUtilsMessengerEXT");
    vkDestroyDebugUtilsMessengerEXT(app->instance, app->debugMessenger, NULL);
    vkDestroyInstance(app->instance, NULL);
    glfwDestroyWindow(app->window);
    glfwTerminate();
    mi_free(app);
}

static void t_app_rebuild_swapchain(TApp* app)
{
    vkDeviceWaitIdle(app->logicalDevice);

    t_app_destroy_sync(app);
    t_app_destroy_buffer(app);
    t_app_destroy_pipeline(app);
    t_app_destroy_render_pass(app);
    t_app_destroy_swapchain(app);

    t_app_init_swapchain(app);
    t_app_init_render_pass(app);
    t_app_init_pipeline(app);
    t_app_init_buffer(app);
    t_app_init_sync(app);
}

static void t_app_record_command(TApp* app, uint32_t imageIndex)
{
    vkResetCommandBuffer(app->commandBuffer, 0);
    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };

    if (vkBeginCommandBuffer(app->commandBuffer, &beginInfo) != VK_SUCCESS) {
        printf("Failed to begin recording command buffer\n");
        return;
    }

    VkClearValue clearColor = { { { 0.0f, 0.0f, 0.0f, 1.0f } } };
    VkRenderPassBeginInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .clearValueCount = 1,
        .pClearValues = &clearColor,
        .renderPass = app->renderPass,
        .framebuffer = app->framebuffers[imageIndex],
        .renderArea.offset = { 0, 0 },
        .renderArea.extent = app->swapchainExtent
    };

    vkCmdBeginRenderPass(app->commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(app->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->pipeline);

    VkViewport viewport = {
        .width = (float)app->swapchainExtent.width,
        .height = (float)app->swapchainExtent.height,
        .maxDepth = 1.0f,
    };

    VkRect2D scissor = {
        .extent = app->swapchainExtent
    };

    VkBuffer vertexBuffers[] = { app->vertexBuffer->vertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdSetViewport(app->commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(app->commandBuffer, 0, 1, &scissor);
    vkCmdBindVertexBuffers(app->commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(app->commandBuffer, app->vertexBuffer->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(app->commandBuffer, app->vertexBuffer->indexCount, 1, 0, 0, 0);
    vkCmdEndRenderPass(app->commandBuffer);
    if (vkEndCommandBuffer(app->commandBuffer) != VK_SUCCESS) {
        printf("Failed to record command buffer\n");
        return;
    }
}

static void t_app_draw(TApp* app)
{
    vkWaitForFences(app->logicalDevice, 1, &app->inFlightFences, VK_TRUE, UINT64_MAX);
    vkResetFences(app->logicalDevice, 1, &app->inFlightFences);
    uint32_t imageIndex;
    VkResult ret = vkAcquireNextImageKHR(app->logicalDevice, app->swapchain, UINT64_MAX, app->imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
    if (VK_ERROR_OUT_OF_DATE_KHR == ret) {
        t_app_rebuild_swapchain(app);
        return;
    }
    if (ret && VK_SUBOPTIMAL_KHR != ret) {
        printf("Failed to acquire next image\n");
        return;
    }

    t_app_record_command(app, imageIndex);

    VkSemaphore waitSemaphores[] = { app->imageAvailableSemaphore };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSemaphore signalSemaphores[] = { app->renderFinishedSemaphore };
    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = waitSemaphores,
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &app->commandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = signalSemaphores
    };
    if (vkQueueSubmit(app->queue, 1, &submitInfo, app->inFlightFences)) {
        printf("Failed to submit draw command buffer\n");
        return;
    }

    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signalSemaphores,
        .swapchainCount = 1,
        .pSwapchains = &app->swapchain,
        .pImageIndices = &imageIndex
    };
    vkQueuePresentKHR(app->queue, &presentInfo);
}

int main(int argc, char* argv[])
{
    TApp* app = t_app_new();
    t_app_init_device(app);
    t_app_init_swapchain(app);
    t_app_init_render_pass(app);
    t_app_init_pipeline(app);
    t_app_init_buffer(app);
    t_app_init_sync(app);
    while (!glfwWindowShouldClose(app->window)) {
        glfwPollEvents();
        t_app_draw(app);
    }

    vkDeviceWaitIdle(app->logicalDevice);

    t_app_free(app);
    return 0;
}