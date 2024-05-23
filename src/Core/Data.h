#ifndef __SNGO_DATA_H
#define __SNGO_DATA_H

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <optional>
#include <vector>

namespace SngoEngine::Core::Data
{

struct ImageCreate_Info : public VkImageCreateInfo
{
  ImageCreate_Info() = delete;
  ImageCreate_Info(VkImageCreateInfo _info)
      : VkImageCreateInfo(_info.sType,
                          _info.pNext,
                          _info.flags,
                          _info.imageType,
                          _info.format,
                          _info.extent,
                          _info.mipLevels,
                          _info.arrayLayers,
                          _info.samples,
                          _info.tiling,
                          _info.usage,
                          _info.sharingMode,
                          _info.queueFamilyIndexCount,
                          _info.pQueueFamilyIndices,
                          _info.initialLayout)
  {
  }
  ImageCreate_Info(VkFormat _format,
                   VkExtent3D _extent,
                   VkImageTiling _tiling,
                   VkImageUsageFlags _usage,
                   uint32_t _mipLevel = 1,
                   uint32_t _arrayLayers = 1,
                   VkSampleCountFlagBits _samples = VK_SAMPLE_COUNT_1_BIT,
                   VkImageLayout _initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                   VkImageCreateFlags _flag = VkImageCreateFlagBits(),
                   VkImageType _imageType = VK_IMAGE_TYPE_2D,
                   VkSharingMode _sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                   const void* _pNext = nullptr,
                   uint32_t _queueFamilyIndexCount = 0,
                   const uint32_t* _pQueueFamilyIndices = nullptr,
                   VkStructureType _sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO)
      : VkImageCreateInfo(_sType,
                          _pNext,
                          _flag,
                          _imageType,
                          _format,
                          _extent,
                          _mipLevel,
                          _arrayLayers,
                          _samples,
                          _tiling,
                          _usage,
                          _sharingMode,
                          _queueFamilyIndexCount,
                          _pQueueFamilyIndices,
                          _initialLayout)
  {
  }
};

struct ImageSubresourceRange_Info : public VkImageSubresourceRange
{
  ImageSubresourceRange_Info() = delete;

  explicit ImageSubresourceRange_Info(VkImageSubresourceRange sub)
      : VkImageSubresourceRange(sub.aspectMask,
                                sub.baseMipLevel,
                                sub.levelCount,
                                sub.baseArrayLayer,
                                sub.layerCount)
  {
  }
  explicit ImageSubresourceRange_Info(VkImageAspectFlags _aspectMask,
                                      uint32_t _baseMipLevel = 0,
                                      uint32_t _levelCount = 1,
                                      uint32_t _baseArrayLayer = 0,
                                      uint32_t _layerCount = 1)
      : VkImageSubresourceRange(_aspectMask,
                                _baseMipLevel,
                                _levelCount,
                                _baseArrayLayer,
                                _layerCount)
  {
  }
};

const ImageSubresourceRange_Info DEFAULT_COLOR_IMAGE_SUBRESOURCE_INFO{VK_IMAGE_ASPECT_COLOR_BIT};

struct ImageViewCreate_Info : public VkImageViewCreateInfo
{
  ImageViewCreate_Info() = delete;
  ImageViewCreate_Info(VkImageViewCreateInfo _info)
      : VkImageViewCreateInfo(_info.sType,
                              _info.pNext,
                              _info.flags,
                              _info.image,
                              _info.viewType,
                              _info.format,
                              _info.components,
                              _info.subresourceRange)
  {
  }
  ImageViewCreate_Info(VkImage _image,
                       VkFormat _format,
                       ImageSubresourceRange_Info _subresourceRange,
                       VkImageViewType _viewType = VK_IMAGE_VIEW_TYPE_2D,
                       VkComponentMapping _components = {VK_COMPONENT_SWIZZLE_R,
                                                         VK_COMPONENT_SWIZZLE_G,
                                                         VK_COMPONENT_SWIZZLE_B,
                                                         VK_COMPONENT_SWIZZLE_A},
                       VkImageViewCreateFlags _flags = VkImageViewCreateFlags(),
                       const void* _pNext = nullptr,
                       VkStructureType _sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO)
      : VkImageViewCreateInfo(_sType,
                              _pNext,
                              _flags,
                              _image,
                              _viewType,
                              _format,
                              _components,
                              _subresourceRange)
  {
  }
};

struct FrameBufferCreate_Info : public VkFramebufferCreateInfo
{
  FrameBufferCreate_Info() = delete;
  FrameBufferCreate_Info(FrameBufferCreate_Info const& _info)
      : VkFramebufferCreateInfo(_info.sType,
                                _info.pNext,
                                _info.flags,
                                _info.renderPass,
                                _info.attachmentCount,
                                _info.pAttachments,
                                _info.width,
                                _info.height,
                                _info.layers)
  {
  }
FrameBufferCreate_Info(VkRenderPass _renderPass,
                         std::vector<VkImageView>& _attachments,
                         const VkExtent3D _extent,
                         uint32_t _layers = 1,
                         VkFramebufferCreateFlags _flags = 0,
                         const void* _pNext = nullptr,
                         VkStructureType _sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO)
      : VkFramebufferCreateInfo(_sType,
                                _pNext,
                                _flags,
                                _renderPass,
                                _attachments.size(),
                                _attachments.data(),
                                _extent.width,
                                _extent.height,
                                _layers)
  {
  }
};

struct MemoryAllocate_Info : public VkMemoryAllocateInfo
{
  MemoryAllocate_Info() = delete;
  MemoryAllocate_Info(VkMemoryAllocateInfo _info)
      : VkMemoryAllocateInfo(_info.sType, _info.pNext, _info.allocationSize, _info.memoryTypeIndex)
  {
  }

  MemoryAllocate_Info(VkDeviceSize _allocationSize,
                      uint32_t _memoryTypeIndex,
                      const void* _pNext = nullptr,
                      VkStructureType _sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO)
      : VkMemoryAllocateInfo(_sType, _pNext, _allocationSize, _memoryTypeIndex)
  {
  }
};

struct BufferCreate_Info : public VkBufferCreateInfo
{
  BufferCreate_Info() = delete;
  BufferCreate_Info(VkBufferCreateInfo _info)
      : VkBufferCreateInfo(_info.sType,
                           _info.pNext,
                           _info.flags,
                           _info.size,
                           _info.usage,
                           _info.sharingMode,
                           _info.queueFamilyIndexCount,
                           _info.pQueueFamilyIndices)
  {
  }

  BufferCreate_Info(VkDeviceSize _size,
                    VkBufferUsageFlags _usage,
                    uint32_t _queueFamilyIndexCount = 0,
                    const uint32_t* _pQueueFamilyIndices = nullptr,
                    VkSharingMode _sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                    VkBufferCreateFlags _flags = VkBufferCreateFlags(),
                    const void* _pNext = nullptr,
                    VkStructureType _sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO)
      : VkBufferCreateInfo(_sType,
                           _pNext,
                           _flags,
                           _size,
                           _usage,
                           _sharingMode,
                           _queueFamilyIndexCount,
                           _pQueueFamilyIndices)
  {
  }
};

struct SwapChainSupportDetails
{
  VkSurfaceCapabilitiesKHR capabilities{};
  std::vector<VkSurfaceFormatKHR> formats{};
  std::vector<VkPresentModeKHR> present_modes{};
};

struct CommandPoolCreate_Info : public VkCommandPoolCreateInfo
{
  CommandPoolCreate_Info() = delete;

  explicit CommandPoolCreate_Info(VkCommandPoolCreateInfo _info)
      : VkCommandPoolCreateInfo(_info.sType, _info.pNext, _info.flags, _info.queueFamilyIndex)
  {
  }
  explicit CommandPoolCreate_Info(
      uint32_t _queueFamilyIndex,
      uint32_t _flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      const void* _pNext = nullptr,
      VkStructureType _sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO)
      : VkCommandPoolCreateInfo(_sType, _pNext, _flags, _queueFamilyIndex)
  {
  }
};

struct CommandBufferAlloc_Info : public VkCommandBufferAllocateInfo
{
  CommandBufferAlloc_Info() = delete;
  CommandBufferAlloc_Info(VkCommandBufferAllocateInfo _info)
      : VkCommandBufferAllocateInfo(_info.sType,
                                    _info.pNext,
                                    _info.commandPool,
                                    _info.level,
                                    _info.commandBufferCount)
  {
  }
  CommandBufferAlloc_Info(VkCommandPool _commandPool,
                          VkCommandBufferLevel _level,
                          uint32_t _commandBufferCount = 1,
                          const void* _pNext = nullptr,
                          VkStructureType _sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO)
      : VkCommandBufferAllocateInfo(_sType, _pNext, _commandPool, _level, _commandBufferCount)
  {
  }
};

struct AttachmentDscription_Info : public VkAttachmentDescription
{
  AttachmentDscription_Info() = delete;
  AttachmentDscription_Info(VkAttachmentDescription des)
      : VkAttachmentDescription(des.flags,
                                des.format,
                                des.samples,
                                des.loadOp,
                                des.storeOp,
                                des.stencilLoadOp,
                                des.stencilStoreOp,
                                des.initialLayout,
                                des.finalLayout)
  {
  }

  AttachmentDscription_Info(VkFormat _format,
                            VkSampleCountFlagBits _samples,
                            VkAttachmentLoadOp _loadOp,
                            VkAttachmentStoreOp _storeOp,
                            VkAttachmentLoadOp _stencilLoadOp,
                            VkAttachmentStoreOp _stencilStoreOp,
                            VkImageLayout _initialLayout,
                            VkImageLayout _finalLayout,
                            VkAttachmentDescriptionFlags _flags = VkAttachmentDescriptionFlags())
      : VkAttachmentDescription(_flags,
                                _format,
                                _samples,
                                _loadOp,
                                _storeOp,
                                _stencilLoadOp,
                                _stencilStoreOp,
                                _initialLayout,
                                _finalLayout)
  {
  }
};

struct AttachmentRef_Info : public VkAttachmentReference
{
  AttachmentRef_Info() : VkAttachmentReference(){};
  AttachmentRef_Info(VkAttachmentReference ref) : VkAttachmentReference(ref.attachment, ref.layout)
  {
  }

  AttachmentRef_Info(uint32_t _attachment, VkImageLayout _layout)
      : VkAttachmentReference(_attachment, _layout)
  {
  }
};

struct AttachmentData_Info
{
  std::vector<AttachmentDscription_Info> descriptions;
  std::vector<uint32_t> preserve_attachment;
  AttachmentRef_Info color_ref;
  AttachmentRef_Info depth_ref;
  AttachmentRef_Info input_ref;
  AttachmentRef_Info resolve_ref;

  int color_count;
  int depth_count;
  int input_count;
  int total_count;
  int preserve_count;
};

struct SubpassDependency_Info : public VkSubpassDependency
{
  SubpassDependency_Info() = delete;
  SubpassDependency_Info(VkSubpassDependency dep)
      : VkSubpassDependency(dep.srcSubpass,
                            dep.dstSubpass,
                            dep.srcStageMask,
                            dep.dstStageMask,
                            dep.srcAccessMask,
                            dep.dstAccessMask,
                            dep.dependencyFlags){

      };
  SubpassDependency_Info(uint32_t _srcSubpass,
                         uint32_t _dstSubpass,
                         VkPipelineStageFlags _srcStageMask,
                         VkPipelineStageFlags _dstStageMask,
                         VkAccessFlags _srcAccessMask,
                         VkAccessFlags _dstAccessMask,
                         VkDependencyFlags _dependencyFlags)
      : VkSubpassDependency(_srcSubpass,
                            _dstSubpass,
                            _srcStageMask,
                            _dstStageMask,
                            _srcAccessMask,
                            _dstAccessMask,
                            _dependencyFlags)
  {
  }
};

VkPipelineVertexInputStateCreateInfo GetVertexInput_Info(
    const std::vector<VkVertexInputBindingDescription>& bindings,
    const std::vector<VkVertexInputAttributeDescription>& attributes);

VkPipelineViewportStateCreateInfo GetViewportState_Info(VkExtent2D _extent);
VkPipelineDynamicStateCreateInfo GetDynamicState_Info(
    const std::vector<VkDynamicState>& dynamic_states = {VK_DYNAMIC_STATE_VIEWPORT,
                                                         VK_DYNAMIC_STATE_SCISSOR});
VkPipelineInputAssemblyStateCreateInfo GetInputAssembly_Info();

VkPipelineRasterizationStateCreateInfo DEFAULT_RASTERIZER_INFO();
VkPipelineMultisampleStateCreateInfo DEFAULT_MULTISAMPLING_INFO();
VkPipelineDepthStencilStateCreateInfo DEFAULT_DEPTHSTENCIL_INFO();
VkPipelineDepthStencilStateCreateInfo DEFAULT_DEPTHSTENCIL_DISABLED_INFO();

VkPipelineColorBlendAttachmentState GetDefaultColorBlend_Attachment();
VkPipelineColorBlendStateCreateInfo DEFAULT_COLORBLEND_INFO(
    VkPipelineColorBlendAttachmentState color_blend_attachment_info);

struct PipelinePreparation_Info
{
  VkPipelineVertexInputStateCreateInfo vertex_input;

  VkPipelineInputAssemblyStateCreateInfo input_assembly;

  VkPipelineViewportStateCreateInfo view_state;

  VkPipelineDynamicStateCreateInfo dynamic_state;

  VkPipelineRasterizationStateCreateInfo rasterizer;

  VkPipelineMultisampleStateCreateInfo multisampling;

  VkPipelineDepthStencilStateCreateInfo depth_stencil;

  VkPipelineColorBlendStateCreateInfo color_blend;
};

struct QueueFamilyIndices
{
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;

  [[nodiscard]] bool is_complete() const
  {
    return graphicsFamily.has_value() && presentFamily.has_value();
  }
};

}  // namespace SngoEngine::Core::Data

#endif