#ifndef __SNGO_DEPTHRESOURCE_H
#define __SNGO_DEPTHRESOURCE_H

#include <vulkan/vulkan_core.h>

#include "src/Core/Device/LogicalDevice.hpp"
#include "src/Core/Source/Image/Image.hpp"
#include "src/Core/Source/Image/ImageVIew.hpp"

namespace SngoEngine::Core::Source::DepthResource
{

VkFormat FInd_DepthFormat(VkPhysicalDevice physical_device);

//===========================================================================================================================
// EngineDepthResource
//===========================================================================================================================

struct EngineDepthResource
{
  EngineDepthResource() = default;
  EngineDepthResource(EngineDepthResource&&) noexcept = default;
  EngineDepthResource& operator=(EngineDepthResource&&) noexcept = default;
  template <class... Args>
  explicit EngineDepthResource(const Device::LogicalDevice::EngineDevice* _device, Args... args)
  {
    creator(_device, args...);
  }
  template <class... Args>
  void init(const Device::LogicalDevice::EngineDevice* _device, Args... args)
  {
    creator(_device, args...);
  }
  ~EngineDepthResource() = default;

  Image::EngineImage engine_Image{};
  ImageView::EngineImageView engine_ImageView;
  VkExtent3D extent{};
  const Device::LogicalDevice::EngineDevice* device{};

 private:
  void creator(const Device::LogicalDevice::EngineDevice* _device,
               VkExtent3D _extent,
               VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL,
               VkImageUsageFlags usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
               VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
               const VkAllocationCallbacks* alloc = nullptr);
  const VkAllocationCallbacks* Alloc{};
};
}  // namespace SngoEngine::Core::Source::DepthResource

#endif