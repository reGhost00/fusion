#ifdef FU_RENDERER_TYPE_VK
#ifndef _FU_RENDERER_VULKAN_MISC_H_
#define _FU_RENDERER_VULKAN_MISC_H_
#ifdef FU_USE_GLFW
#include <GLFW/glfw3.h>
typedef GLFWwindow RawWindow;
#elif defined(FU_USE_SDL)
#include <SDL2/SDL.h>
typedef SDL_Window RawWindow;
#endif
#include <cglm/cglm.h>
#include <mimalloc.h>
#include <vulkan/vulkan.h>

#include "core/object.h"
#include "libs/vk_mem_alloc.h"
// #include "../misc.h"

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

//
//  FUShader
typedef struct _TShaderAttrNode TShaderAttrNode;
typedef struct _TShaderAttrNode {
    TShaderAttrNode* next;
    VkVertexInputAttributeDescription* attr;
    VkFormat format;
    uint32_t attrCount, size, offset, idx;
} TShaderAttrNode;

typedef struct _TShaderBindNode TShaderBindNode;
typedef struct _TShaderBindNode {
    TShaderBindNode* next;
    VkVertexInputBindingDescription* binding;
    VkVertexInputRate rate;
    uint32_t size, idx;
} TShaderBindNode;

typedef struct _TShaderDescNode TShaderDescNode;
typedef struct _TShaderDescNode {
    TShaderDescNode* next;
    VkDescriptorSetLayoutBinding descriptor;
    VkDescriptorBufferInfo bufferInfo;
    void* data;
    FUNotify destroy;
    uint32_t size, idx;
} TShaderDescNode;

typedef struct _TShaderArgs TShaderArgs;
typedef struct _TShaderArgs {
    TShaderArgs* next;
    TShaderAttrNode* attr;
    TShaderBindNode* bind;
    TShaderDescNode* desc;
    VkShaderModuleCreateInfo info;
    // VkDevice device;
    VkShaderStageFlagBits stage;
    // VkShaderModule module;
    uint32_t* code;
    uint32_t size, idx;
} TShaderArgs;

typedef struct _FUShader FUShader;
typedef struct _FUShader {
    FUObject parent;
    FUShader* next;
    TShaderArgs* args;
    VkDevice device;
    // VkShaderEXT shader;
    // VkShaderStageFlagBits stage;
    char* name;
    uint32_t size, idx;
} FUShader;

void t_shader_args_free(TShaderArgs* args);
void t_shader_args_free_all(TShaderArgs* args);
FUShader* fu_shader_link_and_steal(FUShader** curr, FUShader** next);
TShaderArgs* t_shader_args_link_and_steal(FUShader* shader, TShaderArgs* prev);
TShaderDescNode* t_shader_descriptor_link_and_steal(FUShader* shader, TShaderDescNode* prev);
VkVertexInputBindingDescription* t_shader_submit_bindings(FUShader* shader, uint32_t* cnt);
VkVertexInputAttributeDescription* t_shader_submit_attributes(FUShader* shader, uint32_t* cnt);
VkDescriptorPoolSize* t_shader_descriptors_fill_sizes(TShaderDescNode* desc, VkDescriptorPoolSize* sizes);
VkDescriptorSetLayoutBinding* t_shader_descriptors_to_binding(TShaderDescNode* desc, uint32_t* cnt);
//
//  TSurface 实例成员
typedef struct _TSurface TSurface;
typedef struct _TBuffer {
    VmaAllocation alloc;
    VmaAllocationInfo allocInfo;
    VkBuffer buffer;
} TBuffer;

TBuffer* t_buffer_create(TSurface* surface, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
bool t_buffer_init(TSurface* sf, TBuffer* buffer, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
bool t_buffer_init_with_data(TSurface* sf, TBuffer* buffer, VkDeviceSize size, void* data, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
void t_buffer_free(TBuffer* buffer);

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
bool t_physical_device_check_support_extension(TPhysicalDevice* dev, const char* name);
/** 交换链 */
typedef struct _TSwapchain {
    VkSwapchainKHR swapchain;
    VkSurfaceFormatKHR format;
    VkPresentModeKHR presentMode;
    VkExtent2D size;
    VkImage* images;
    VkImageView* imageViews;
    VkFramebuffer* framebuffers;
    uint32_t imageCount;
} TSwapchain;
#define T_IMAGE(sf) ((sf)->swapchain.images)
#define T_IMAGE_VIEW(sf) ((sf)->swapchain.imageViews)
#define T_SWAPCHAIN(sf) ((sf)->swapchain.swapchain)
#define T_FRAME_SIZE(sf) ((sf)->swapchain.size)
#define T_FRAME_BUFFER(sf) ((sf)->swapchain.framebuffers)

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

typedef struct _TPool {
    VkCommandPool commandPool;
    VkCommandBuffer* commandBuffers;
    VkDescriptorPool descriptorPool;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSet* descriptorSets;
    TBuffer* uniformBuffers;
} TPool;
#define T_CMD_POOL(sf) ((sf)->pool.commandPool)
#define T_CMD_BUFFER(sf) ((sf)->pool.commandBuffers)
#define T_DESC_POOL(sf) ((sf)->pool.descriptorPool)
#define T_DESC_SET_LAYOUT(sf) ((sf)->pool.descriptorSetLayout)
#define T_DESC_SET(sf) ((sf)->pool.descriptorSets)
#define T_UNIFORM_BUFFER(sf) ((sf)->pool.uniformBuffers)

#ifdef NDEBUG
#define _t_surface_to_instance(sf)
#define _t_surface_to_device(sf)
#define _t_surface_to_swapchain(sf)

typedef struct _TSurface {
    FUObject parent;
#else
#define _t_surface_to_instance(sf)                                      \
    do {                                                                \
        if (E_SURFACE_INIT_STATE_INSTANCE > sf->_state)                 \
            sf->_state = E_SURFACE_INIT_STATE_INSTANCE;                 \
        else {                                                          \
            fu_warning("%s: vuklan instance already init\n", __func__); \
        }                                                               \
    } while (0)

#define _t_surface_to_device(sf)                                      \
    do {                                                              \
        if (E_SURFACE_INIT_STATE_DEVICE > sf->_state)                 \
            sf->_state = E_SURFACE_INIT_STATE_DEVICE;                 \
        else {                                                        \
            fu_warning("%s: vulkan device already init\n", __func__); \
        }                                                             \
    } while (0)

#define _t_surface_to_swapchain(sf)                                      \
    do {                                                                 \
        if (E_SURFACE_INIT_STATE_SWAPCHAIN > sf->_state)                 \
            sf->_state = E_SURFACE_INIT_STATE_SWAPCHAIN;                 \
        else {                                                           \
            fu_warning("%s: vulkan swapchain already init\n", __func__); \
        }                                                                \
    } while (0)

#define _t_surface_to_cmd_pool(sf)                                          \
    do {                                                                    \
        if (E_SURFACE_INIT_STATE_CMD_POOL > sf->_state)                     \
            sf->_state = E_SURFACE_INIT_STATE_CMD_POOL;                     \
        else {                                                              \
            fu_warning("%s: vulkan command pool already init\n", __func__); \
        }                                                                   \
    } while (0)

#define _t_surface_to_cmd_buffer(sf)                                          \
    do {                                                                      \
        if (E_SURFACE_INIT_STATE_CMD_BUFF > sf->_state)                       \
            sf->_state = E_SURFACE_INIT_STATE_CMD_BUFF;                       \
        else {                                                                \
            fu_warning("%s: vulkan command buffer already init\n", __func__); \
        }                                                                     \
    } while (0)

#define _t_surface_to_desc_pool(sf)                                            \
    do {                                                                       \
        if (E_SURFACE_INIT_STATE_DESC_POOL > sf->_state)                       \
            sf->_state = E_SURFACE_INIT_STATE_DESC_POOL;                       \
        else {                                                                 \
            fu_warning("%s: vulkan descriptor pool already init\n", __func__); \
        }                                                                      \
    } while (0)

#define _t_surface_to_desc_set(sf)                                            \
    do {                                                                      \
        if (E_SURFACE_INIT_STATE_DESC_SET > sf->_state)                       \
            sf->_state = E_SURFACE_INIT_STATE_DESC_SET;                       \
        else {                                                                \
            fu_warning("%s: vulkan descriptor set already init\n", __func__); \
        }                                                                     \
    } while (0)

#define _t_surface_to_buff(sf)                                                        \
    do {                                                                              \
        if (E_SURFACE_INIT_STATE_BUFF > sf->_state)                                   \
            sf->_state = E_SURFACE_INIT_STATE_BUFF;                                   \
        else {                                                                        \
            fu_warning("%s: framebuffers & uniform buffers already set\n", __func__); \
        }                                                                             \
    } while (0)

typedef enum _ESurfaceInitState {
    E_SURFACE_INIT_STATE_NONE,
    E_SURFACE_INIT_STATE_INSTANCE,
    E_SURFACE_INIT_STATE_DEVICE,
    E_SURFACE_INIT_STATE_SWAPCHAIN,
    E_SURFACE_INIT_STATE_CMD_POOL,
    E_SURFACE_INIT_STATE_CMD_BUFF,
    E_SURFACE_INIT_STATE_DESC_POOL,
    E_SURFACE_INIT_STATE_DESC_SET,
    E_SURFACE_INIT_STATE_BUFF, // frame buffers & uniforms
    E_SURFACE_INIT_STATE_CNT
} ESurfaceInitState;
typedef struct _TSynchronizationObjects {
    VkSemaphore *imageAvailableSemaphores, *renderFinishedSemaphores;
    VkFence* inFlightFences;
} TSynchronizationObjects;
#define T_IMAGE_SEMAPHORES(vk) ((vk)->sync.imageAvailableSemaphores)
#define T_RENDER_SEMAPHORES(vk) ((vk)->sync.renderFinishedSemaphores)
#define T_FENCES(vk) ((vk)->sync.inFlightFences)
typedef struct _TSurface {
    FUObject parent;
    ESurfaceInitState _state;
#endif
#ifdef FU_ENABLE_MESSAGE_CALLABLE
    VkDebugUtilsMessengerEXT debugger;
#endif
    TPipelineLibrary* pipelineLibrary;
    RawWindow* window;
    VkInstance instance;
    VkSurfaceKHR surface;
    TDevice device;
    TSwapchain swapchain;
    TPool pool;
    TSynchronizationObjects sync;
    uint32_t size[2];
} TSurface;
#endif
#endif