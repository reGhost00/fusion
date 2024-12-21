#ifdef FU_RENDERER_TYPE_VK
#ifndef _FU_RENDERER_VULKAN_MISC_H_
#define _FU_RENDERER_VULKAN_MISC_H_
#ifdef FU_USE_GLFW
#include <GLFW/glfw3.h>
typedef GLFWwindow RawWindow;
#elif FU_USE_SDL
#include <SDL2/SDL.h>
typedef SDL_Window RawWindow;
#endif
#include <cglm/cglm.h>
#include <mimalloc.h>
#include <vulkan/vulkan.h>

#include "core/object.h"

#include "libs/vk_mem_alloc.h"

#include "renderer/misc.h"
#include "renderer/shader.h"
#define FU_USE_HOST_COPY
//
// Custom allocator

static void* _vk_malloc(void* usd, size_t size, size_t alignment, VkSystemAllocationScope scope) { return mi_aligned_alloc(alignment, size); }
static void* _vk_realloc(void* usd, void* p, size_t size, size_t alignment, VkSystemAllocationScope scope) { return mi_aligned_recalloc(p, 1, size, alignment); }
static void _vk_free(void* usd, void* p) { mi_free(p); }

static VkAllocationCallbacks defCustomAllocator = {
    .pfnAllocation = _vk_malloc,
    .pfnReallocation = _vk_realloc,
    .pfnFree = _vk_free
};

void tvk_fns_init_instance(VkInstance instance);
bool tvk_check_format_support_transfer(VkPhysicalDevice dev, VkFormat format);
bool tvk_image_view_init(VkDevice device, VkFormat format, VkImage image, VkImageView* view);
//
//  FUShader
typedef struct _TShaderAttrNode TShaderAttrNode;
typedef struct _TShaderAttrNode {
    TShaderAttrNode* next;
    VkFormat format;
    uint32_t size, offset, idx;
} TShaderAttrNode;

typedef struct _TShaderBindNode TShaderBindNode;
typedef struct _TShaderBindNode {
    TShaderBindNode* next;
    VkVertexInputRate rate;
    uint32_t size, idx;
} TShaderBindNode;

typedef struct _TShaderDescNode TShaderDescNode;
typedef struct _TShaderDescNode {
    TShaderDescNode* next;
    VkDescriptorSetLayoutBinding descriptor;
    VkDescriptorBufferInfo bufferInfo;
    VkDescriptorImageInfo imageInfo;
    void* data;
    FUSize imgSize;
    FUNotify destroy;
    uint32_t cnt, dataSize, idx;
} TShaderDescNode;
void t_shader_desc_free(TShaderDescNode* desc);

typedef enum _EShaderArgsFlag {
    E_SHADER_ARGS_FLAG_USER,
    E_SHADER_ARGS_FLAG_DEFAULT,
    E_SHADER_ARGS_FLAG_DEFAULT_WITH_UBO = 1U << 1,
    E_SHADER_ARGS_FLAG_DEFAULT_WITH_TEXTURE = 1U << 2
} EShaderArgsFlag;

typedef struct _TShaderArgs TShaderArgs;
typedef struct _TShaderArgs {
    TShaderArgs* next;
    TShaderAttrNode* attr;
    TShaderBindNode* bind;
    TShaderDescNode* desc;
    VkShaderModuleCreateInfo info;
    VkShaderStageFlagBits stage;
    // VkShaderModuleCreateInfo.pCode 是常量指针
    uint32_t* code;
    VkFilter filter;
    VkSamplerMipmapMode mipmapMode;
    VkSamplerAddressMode addressMode;
    EShaderArgsFlag flags;
    uint32_t size, idx;
} TShaderArgs;
typedef TShaderArgs* (*FUShaderGetArgs)();
typedef struct _FUShader {
    FUObject parent;
    FUShader* next;
    TShaderArgs* args;
    VkDevice device;
    VkShaderStageFlagBits stage;
    char* name;
    uint32_t size, idx;
} FUShader;

void t_shader_args_free(TShaderArgs* args);
void t_shader_args_free_all(TShaderArgs* args);

void fu_shader_unref_all(FUShader* shader);
FUShader* fu_shader_link_and_steal(FUShader** curr, FUShader** next);
TShaderArgs* t_shader_args_link_and_steal(FUShader* shader, TShaderArgs* prev);
TShaderArgs* t_shader_args_init_default_ubo_vert(TShaderArgs* sda);
TShaderArgs* t_shader_args_init_default_ubo_frag(TShaderArgs* sda);
TShaderArgs* t_shader_args_init_default_texture_vert(TShaderArgs* sda);
TShaderArgs* t_shader_args_init_default_texture_frag(TShaderArgs* sda);

TShaderDescNode* t_shader_descriptor_link_and_steal(FUShader* shader, TShaderDescNode* prev);
VkVertexInputAttributeDescription* t_shader_submit_attributes(FUShader* shader, uint32_t* cnt);
VkDescriptorSetLayoutBinding* t_shader_descriptors_to_binding(TShaderDescNode* desc);
VkDescriptorPoolSize* t_shader_descriptors_fill_sizes(TShaderDescNode* desc, VkDescriptorPoolSize* sizes);

//
//  通用
//  同步对象
typedef struct _TVKFence {
    VkDevice device;
    VkFence* fences;
    uint32_t count;
} TVKFence;
TVKFence* tvk_fence_new(VkDevice device, uint32_t count, bool signaled);
void tvk_fence_free(TVKFence* fence);
//
//  缓冲
typedef struct _TSurface TSurface;
typedef struct _TUniform {
    VmaAllocation alloc;
    VmaAllocationInfo allocInfo;
    VkBuffer buffer;
    VkDeviceAddress address;
} TUniform;

typedef struct _TImage {
    VmaAllocation alloc;
    VmaAllocationInfo allocInfo;
    VkImage image;
    VkImageView view;
    VkImageLayout layout;
    VkSampler sampler;
    VkExtent3D extent;
} TImage;

bool tvk_buffer_allocator_init(TSurface* sf);
void tvk_buffer_allocator_destroy();

void t_uniform_destroy(TUniform* buffer);
bool t_uniform_init(VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags, TUniform* buffer);
bool t_uniform_init_with_data(VkDeviceSize size, const void* data, VkBufferUsageFlags usage, TUniform* buffer);

void t_uniform_free(TUniform* buffer);
TUniform* t_uniform_new_staging(VkDeviceSize size, const void* data);

void t_image_destroy(TImage* buffer);
bool t_image_init(VkFormat format, FUSize* size, VkImageUsageFlags usage, TImage* buffer);
bool t_image_init_with_data(VkCommandPool pool, VkQueue queue, VkFormat format, FUSize* size, const void* data, VkImageUsageFlags usage, TImage* buffer);
bool t_image_init_sampler(TImage* image, VkFilter filter, VkSamplerMipmapMode minimap, VkSamplerAddressMode addr);
//
//  命令
typedef struct _TCommandBuffer {
    VkDevice device;
    VkCommandPool pool;
    VkCommandBuffer buffer;
} TCommandBuffer;
TCommandBuffer* tvk_command_buffer_new_once(VkDevice device, VkCommandPool pool);
TCommandBuffer* tvk_command_buffer_submit_free(TCommandBuffer* cmdf, VkQueue queue, VkFence fence);
void tvk_command_buffer_free(TCommandBuffer* cmdf);
//
//  TSurface 实例成员
typedef struct _TQueueFamilyProperties {
    VkQueueFamilyProperties properties;
    uint32_t index;
    bool presentSupported;
} TQueueFamilyProperties;

typedef struct _TPhysicalDevice {
    VkPhysicalDevice device;
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceDescriptorBufferPropertiesEXT descriptorBufferProperties;
    VkExtensionProperties* extensions;
    TQueueFamilyProperties* queueFamilyProperties;
    TQueueFamilyProperties* queueFamilyGraphics;
    TQueueFamilyProperties* queueFamilyCompute;
    TQueueFamilyProperties* queueFamilyTransfer;
    uint32_t deviceCount, deviceIndex, queueFamilyCount, extensionCount, score;
} TPhysicalDevice;

typedef struct _TSwapchainDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR* supportedFormats;
    VkPresentModeKHR* supportedPresentModes;
    uint32_t formatCount, presentModeCount;
} TSwapchainDetails;

/** 逻辑设备 */
typedef struct _TDevice {
    TPhysicalDevice physicalDevice;
    VkDevice logicalDevice;
    VkQueue generalQueue;
    uint32_t queueIndex;
} TDevice;
#define T_PHYSICAL_DEVICE(sf) ((sf)->device.physicalDevice.device)
#define T_DEVICE(sf) ((sf)->device.logicalDevice)
#define T_QUEUE(sf) ((sf)->device.generalQueue)

TPhysicalDevice* t_physical_device_enumerate(VkInstance inst, uint32_t* cnt);
void t_physical_device_init_with(TPhysicalDevice* dst, const TPhysicalDevice* src);
void t_physical_device_free(TPhysicalDevice* dev);
void t_physical_device_destroy(TPhysicalDevice* dev);
TPhysicalDevice* t_physical_device_rate(TPhysicalDevice* devs);

bool t_physical_device_enumerate_queue_family(TPhysicalDevice* dev, VkSurfaceKHR sf);
int t_physical_device_check_support_extension2(TPhysicalDevice* dev, const char* const* const names, const uint32_t cnt);
/** 交换链 */
typedef struct _TSwapchain {
    VkSwapchainKHR swapchain;
    VkSurfaceFormatKHR format;
    VkPresentModeKHR presentMode;
    VkExtent2D size;
    VkImage* images;
    VkImageView* imageViews;
    uint32_t imageCount;
} TSwapchain;
#define T_IMAGE(sf) ((sf)->swapchain.images)
#define T_IMAGE_VIEW(sf) ((sf)->swapchain.imageViews)
#define T_SWAPCHAIN(sf) ((sf)->swapchain.swapchain)
#define T_FRAME_SIZE(sf) ((sf)->swapchain.size)
// #define T_FRAME_BUFFER(sf) ((sf)->swapchain.framebuffers)

void t_swapchain_details_free(TSwapchainDetails* details);
TSwapchainDetails* t_swapchain_details_query(TPhysicalDevice* dev, VkSurfaceKHR surface);

//
//  TSurface

typedef struct _TPipelineLibrary {
    VkDevice device;
    VkPipelineCache cache;
    VkPipeline vertexInputInterface;
    VkPipeline preRasterizationShaders;
    VkPipeline fragmentOutputInterface;
    VkPipeline fragmentShaders;
} TPipelineLibrary;

TPipelineLibrary* t_pipeline_library_new(VkDevice device);
void t_pipeline_library_free(TPipelineLibrary* lib);

typedef struct _TPipeline {
    TPipelineLibrary* pipelineLibrary;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    VkRenderPass renderPass;
    VkFramebuffer* framebuffers;
    TUniform vertices, indices;

    uint32_t vertexCount, indexCount, frameCount;

} TPipeline;
#define T_PIPELINE_LAYOUT(ctx) ((ctx)->pipeline.pipelineLayout)
#define T_PIPELINE(ctx) ((ctx)->pipeline.pipeline)
#define T_RENDER_PASS(ctx) ((ctx)->pipeline.renderPass)
#define T_VERTEX_BUFF(ctx) ((ctx)->pipeline.vertices.buffer)
#define T_INDEX_BUFF(ctx) ((ctx)->pipeline.indices.buffer)
#define T_FRAME_BUFF(ctx) ((ctx)->pipeline.framebuffers)

typedef struct _TPool {
    VkCommandPool commandPool;
    VkCommandBuffer* commandBuffers;
    TUniform* uniformBuffers;
    TImage* imageBuffers;
    uint32_t uniformBufferCount, imageBufferCount;
    VkDescriptorPool descriptorPool;
    VkDescriptorSet* descriptorSets;
} TPool;
#define T_DESC_POOL(ctx) ((ctx)->pool.descriptorPool)
#define T_DESC_SET(ctx) ((ctx)->pool.descriptorSets)
#endif
#define T_UNIFORM_BUFF(ctx) ((ctx)->pool.uniformBuffers)
#define T_IMAGE_BUFF(ctx) ((ctx)->pool.imageBuffers)
#define T_CMD_POOL(ctx) ((ctx)->pool.commandPool)
#define T_CMD_BUFFER(ctx) ((ctx)->pool.commandBuffers)

typedef struct _TSynchronizationObjects {
    VkSemaphore *imageAvailableSemaphores, *renderFinishedSemaphores;
    VkFence* inFlightFences;
} TSynchronizationObjects;
#define T_IMAGE_SEMAPHORES(ctx) ((ctx)->sync.imageAvailableSemaphores)
#define T_RENDER_SEMAPHORES(ctx) ((ctx)->sync.renderFinishedSemaphores)
#define T_FENCES(ctx) ((ctx)->sync.inFlightFences)
//
// 初始化阶段临时数据
typedef struct _TCtxInitArgs {
    TShaderArgs *sda, *vertex, *fragment;
    TShaderDescNode* desc;
    VkVertexInputAttributeDescription* attr;
    VkDescriptorSetLayoutBinding* descBinding;
    VkDescriptorSetLayout* descSetLayout;
    VkDescriptorPoolSize* descPoolSize;
    VkWriteDescriptorSet* descWriteSets;
    VkFilter filter;
    VkSamplerMipmapMode mipmapMode;
    VkSamplerAddressMode addressMode;
    uint32_t attrCount, descCount, descImageInfoCount;
} TCtxInitArgs;
//
// 上下文
// 保存渲染管线，顶点数据等
typedef struct _FUContext {
    FUObject parent;
    TCtxInitArgs* args;
    FUShader* shader;
    VkDevice device;
    VkDescriptorSetLayout descriptorSetLayout;
    TPipeline pipeline;
    TPool pool;
    TSynchronizationObjects sync;
} FUContext;

typedef struct _TSurface {
    FUObject parent;
#ifdef FU_ENABLE_MESSAGE_CALLABLE
    VkDebugUtilsMessengerEXT debugger;
#endif
    TPipelineLibrary* pipelineLibrary;
    RawWindow* window;
    VkInstance instance;
    VkSurfaceKHR surface;
    TDevice device;
    TSwapchain swapchain;
    FUSize size;
    bool outOfDate;
} TSurface;

#endif