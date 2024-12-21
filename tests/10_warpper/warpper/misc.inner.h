// #include <mimalloc.h>
// #include <vulkan/vulkan.h>

// static void* _vk_malloc(void* usd, size_t size, size_t alignment, VkSystemAllocationScope scope) { return mi_aligned_alloc(alignment, size); }
// static void* _vk_realloc(void* usd, void* p, size_t size, size_t alignment, VkSystemAllocationScope scope) { return mi_aligned_recalloc(p, 1, size, alignment); }
// static void _vk_free(void* usd, void* p) { mi_free(p); }

// static VkAllocationCallbacks defCustomAllocator = {
//     .pfnAllocation = _vk_malloc,
//     .pfnReallocation = _vk_realloc,
//     .pfnFree = _vk_free
// };

// #ifdef __cplusplus
// extern "C" {
// #endif
// void vk_uniform_buffer_allocator_destroy();
// bool vk_uniform_buffer_allocator_init(VkInstance instance, VkDevice device, VkPhysicalDevice physicalDevice);

// bool vk_image_view_init(VkDevice device, VkFormat format, VkImage image, VkImageAspectFlags aspect, uint32_t levels, VkImageView* view);
// #ifdef __cplusplus
// }
// #endif
