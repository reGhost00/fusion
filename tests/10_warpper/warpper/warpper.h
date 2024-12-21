#ifdef FU_USE_GLFW
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
typedef GLFWwindow RawWindow;
#elif defined(FU_USE_SDL)
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>
typedef SDL_Window RawWindow;
#endif
#include "libs/vk_mem_alloc.h"
#include <mimalloc.h>
#ifdef __cplusplus
extern "C" {
#endif

inline void* _vk_malloc(void* usd, size_t size, size_t alignment, VkSystemAllocationScope scope) { return mi_aligned_alloc(alignment, size); }
inline void* _vk_realloc(void* usd, void* p, size_t size, size_t alignment, VkSystemAllocationScope scope) { return mi_aligned_recalloc(p, 1, size, alignment); }
inline void _vk_free(void* usd, void* p) { mi_free(p); }

static VkAllocationCallbacks defCustomAllocator = {
    .pfnAllocation = _vk_malloc,
    .pfnReallocation = _vk_realloc,
    .pfnFree = _vk_free
};

void vk_allocator_destroy();
bool vk_allocator_init(VkInstance instance, VkDevice device, VkPhysicalDevice physicalDevice);

typedef struct _TFence {
    VkFence* fences;
    VkDevice device;
    uint32_t count;
} TFence;
TFence* vk_fence_new(VkDevice device, uint32_t count, bool signaled);
void vk_fence_free(TFence* fence);

typedef struct _TCommandBuffer {
    VkCommandBuffer buffer;
    VkCommandPool pool;
    VkDevice device;
} TCommandBuffer;

typedef struct _TUniformBuffer {
    VmaAllocation alloc;
    VmaAllocationInfo allocInfo;
    VkBuffer buffer;
} TUniformBuffer;
void vk_uniform_buffer_destroy(TUniformBuffer* buff);
void vk_uniform_buffer_free(TUniformBuffer* buff);
bool vk_uniform_buffer_init(VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags, TUniformBuffer* tar);
TUniformBuffer* vk_uniform_buffer_new(VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags);
TCommandBuffer* vk_uniform_buffer_copy(VkCommandPool commandPool, VkQueue queue, TUniformBuffer* dst, TUniformBuffer* src, VkFence fence);

typedef struct _TImage {
    VkImage image;
    VkImageView view;
} TImage;

typedef struct _TImageBuffer {
    VmaAllocation alloc;
    VmaAllocationInfo allocInfo;
    TImage buff;
    VkImageLayout layout;
    VkFormat format;
    VkExtent2D extent;
    uint32_t levels;
} TImageBuffer;

void vk_image_buffer_destroy(TImageBuffer* buff);
bool vk_image_view_init(VkDevice device, VkFormat format, VkImage image, VkImageAspectFlags aspect, uint32_t levels, VkImageView* view);
bool vk_image_buffer_init(VkImageUsageFlags usage, VmaAllocationCreateFlags flags, VkFormat format, VkImageAspectFlags aspect, uint32_t levels, VkSampleCountFlagBits samples, TImageBuffer* tar);
bool vk_image_buffer_init_from_data(const uint8_t* data, const uint32_t width, const uint32_t height, uint32_t levels, VkSampleCountFlagBits samples, TImageBuffer* tar);

void vk_command_buffer_free(TCommandBuffer* cmdf);
TCommandBuffer* vk_image_buffer_transition_layout(TImageBuffer* image, VkCommandPool pool, VkQueue queue, VkImageLayout layout, VkFence fence);
TCommandBuffer* vk_uniform_buffer_copy_to_image(VkCommandPool commandPool, VkQueue queue, TImageBuffer* dst, TUniformBuffer* src, VkFence fence);
TCommandBuffer* vk_image_buffer_generate_mipmaps(TImageBuffer* image, VkCommandPool pool, VkQueue queue, VkFence fence);

bool vk_sampler_init(VkFilter filter, VkSamplerMipmapMode mipmap, VkSamplerAddressMode addr, uint32_t maxLod, VkSampler* sampler);
//
//  vulkan device
typedef struct _TQueueFamilyProperties {
    VkQueueFamilyProperties properties;
    uint32_t index;
    bool presentSupported;
} TQueueFamilyProperties;

typedef struct _TPhysicalDevice {
    VkPhysicalDevice physicalDevice;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceProperties properties;
    VkExtensionProperties* extensions;
    TQueueFamilyProperties* queueFamilies;
    TQueueFamilyProperties* graphicsQueueFamilies;
    TQueueFamilyProperties* computeQueueFamilies;
    TQueueFamilyProperties* transferQueueFamilies;
    uint32_t extensionCount, queueFamilyCount, graphicsQueueFamilyCount, computeQueueFamilyCount, transferQueueFamilyCount;
} TPhysicalDevice;

typedef struct _TDevice {
    TPhysicalDevice physicalDevice;
    VkDevice device;
    /** 通用队列 */
    VkQueue generalQueue;
    /** 多重采样数量 */
    VkSampleCountFlagBits sampleCount;
    /** 呈现交换链 */
    uint32_t queueFamilyIndex;
} TDevice;

//
//  vulkan surface
typedef struct _TSurface {
    VkSurfaceKHR surface;
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR format;
    VkPresentModeKHR presentMode;
    VkExtent2D extent;
    RawWindow* window;
} TSurface;
bool t_surface_init(uint32_t width, uint32_t height, const char* title, VkInstance instance, TSurface* surface);
void t_surface_destroy(TSurface* surface, VkInstance instance);
//
//  vulkan swapchain
typedef struct _TSwapchain {
    VkSwapchainKHR swapchain;
    // VkFramebuffer* framebuffers;
    TImage* images;
    uint32_t imageCount;
    bool outOfDate;
} TSwapchain;
bool t_swapchain_init(VkInstance instance, TDevice* device, TSurface* surface, TSwapchain* swapchain);
bool t_swapchain_update(TSwapchain* swapchain, TDevice* device, TSurface* surface);
void t_swapchain_destroy(TSwapchain* swapchain, TDevice* device);
//
//  vulkan description

typedef struct _TDescriptorArgs {
    VkDescriptorSetLayoutBinding* bindings;
    VkDescriptorPoolSize* sizes;
    VkWriteDescriptorSet* writes;
    uint32_t bindingCount, descriptorCount;
} TDescriptorArgs;

typedef union _TWriteDescriptorSetInfo {
    VkDescriptorBufferInfo* bufferInfo;
    VkDescriptorImageInfo* imageInfo;
} TWriteDescriptorSetInfo;

typedef struct _TDescriptor {
    VkDescriptorSet* sets;
    VkDescriptorSetLayout layout;
    VkDescriptorPool pool;
} TDescriptor;

typedef void (*TWriteDescriptorSetUpdate)(VkWriteDescriptorSet* info, uint32_t index, void* usd);
TDescriptorArgs* vk_descriptor_args_new(uint32_t framebufferCount, uint32_t descriptorCount, ...);
void vk_descriptor_args_free(TDescriptorArgs* args);

void vk_descriptor_args_update_stage_flags(TDescriptorArgs* args, ...);
void vk_descriptor_args_update_info(TDescriptorArgs* args, ...);

bool t_descriptor_init(VkDevice device, TDescriptorArgs* args, TDescriptor* descriptor, TWriteDescriptorSetUpdate fnUpdate, void* usd);
void t_descriptor_destroy(VkDevice device, TDescriptor* descriptor);

//
//  vulkan pipeline

typedef struct _TPipelineIndices {
    int vertexInputInterfaceIndex, preRasterizationShaderIndex, fragmentOutputInterfaceIndex, fragmentShaderIndex;
} TPipelineIndices;
#define use_warp
#ifdef use_warp
typedef struct _TPipelineLibrary {
    VkPipeline* vertexInputInterfaces;
    VkPipeline* preRasterizationShaders;
    VkPipeline* fragmentOutputInterfaces;
    VkPipeline* fragmentShaders;
    uint32_t vertexInputInterfaceCount, preRasterizationShaderCount, fragmentOutputInterfaceCount, fragmentShaderCount;
} TPipelineLibrary;
#else
typedef struct _TPipelineLibrary {
    VkPipelineCache cache;
    VkPipeline vertexInputInterface;
    VkPipeline preRasterizationShaders;
    VkPipeline fragmentOutputInterface;
    VkPipeline fragmentShaders;
} TPipelineLibrary;
#endif
typedef struct _TPipeline {
    TPipelineLibrary* library;
    VkRenderPass renderPass;
    VkPipelineLayout layout;
    VkPipeline pipeline;
} TPipeline;

bool t_pipeline_init_render_pass(TPipeline* pipeline, VkDevice device, uint32_t attachmentCount, const VkAttachmentDescription* attachments, uint32_t subpassCount, const VkSubpassDescription* subpasses, uint32_t dependencyCount, const VkSubpassDependency* dependencies);
int t_pipeline_new_vertex_input_interface(TPipeline* pipeline, VkDevice device, uint32_t bindingCount, const VkVertexInputBindingDescription* bindings, uint32_t attributeCount, const VkVertexInputAttributeDescription* attributes);
int t_pipeline_new_pre_rasterization_shaders(TPipeline* pipeline, VkDevice device, uint32_t stageCount, const VkPipelineShaderStageCreateInfo* stages);
int t_pipeline_init_layout_and_pre_rasterization_shaders(TPipeline* pipeline, VkDevice device, uint32_t setLayoutCount, const VkDescriptorSetLayout* setLayouts, uint32_t stageCount, const VkPipelineShaderStageCreateInfo* stages);
int t_pipeline_new_fragment_shader(TPipeline* pipeline, VkDevice device, uint32_t stageCount, const VkPipelineShaderStageCreateInfo* stages, bool hasDepth);
int t_pipeline_new_fragment_output_interface(TPipeline* pipeline, VkDevice device, VkSampleCountFlagBits samples, uint32_t attachmentCount, const VkPipelineColorBlendAttachmentState* attachments);

bool t_pipeline_init(TPipeline* pipeline, VkDevice device, const TPipelineIndices* indices);
void t_pipeline_destroy(TPipeline* pipeline, VkDevice device);
void t_pipeline_destroy_all(TPipeline* pipeline, VkDevice device);

/**
 * enumerate physical device
 * select best one
 * and create logical device
 */
bool t_device_init(VkInstance instance, VkSurfaceKHR surface, TDevice* device);
void t_device_destroy(TDevice* device);

//
//  vulkan instance
typedef struct _TInstance {
    VkInstance instance;
#ifdef FU_ENABLE_MESSAGE_CALLABLE
    VkDebugUtilsMessengerEXT debugger;
#endif
} TInstance;
/** init vulkan instance */
bool t_instance_init(const char* name, const uint32_t version, TInstance* instance);
void t_instance_destroy(TInstance* instance);

#ifdef __cplusplus
}
#endif