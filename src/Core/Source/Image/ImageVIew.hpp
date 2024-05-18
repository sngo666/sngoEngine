#ifndef __SNGO_IMAGEVIEW_H
#define __SNGO_IMAGEVIEW_H

#include <vulkan/vulkan_core.h>

#include <cstddef>
#include <stdexcept>
#include <vector>

#include "fmt/core.h"
#include "src/Core/Data.h"
#include "src/Core/Device/LogicalDevice.hpp"

namespace SngoEngine::Core::Source::ImageView
{

//===========================================================================================================================
// EngineImageView
//===========================================================================================================================

struct EngineImageView
{
  EngineImageView() = default;
  EngineImageView(EngineImageView&&) noexcept = default;
  EngineImageView& operator=(EngineImageView&&) noexcept = default;
  template <typename... Args>
  explicit EngineImageView(const Device::LogicalDevice::EngineDevice* _device, Args... args)
  {
    creator(_device, args...);
  }
  template <typename... Args>
  void operator()(const Device::LogicalDevice::EngineDevice* _device, Args... args)
  {
    creator(_device, args...);
  }
  ~EngineImageView()
  {
    if (image_view != VK_NULL_HANDLE)
      vkDestroyImageView(device->logical_device, image_view, Alloc);
  }

  VkImageView image_view{};
  const Device::LogicalDevice::EngineDevice* device{};

 private:
  void creator(const Device::LogicalDevice::EngineDevice* _device,
               Data::ImageViewCreate_Info _info,
               const VkAllocationCallbacks* alloc = nullptr);
  const VkAllocationCallbacks* Alloc{};
};

//===========================================================================================================================
// EngineImageViews
//===========================================================================================================================

struct EngineImageViews
{
  EngineImageViews() = default;
  EngineImageViews(EngineImageViews&&) noexcept = default;
  EngineImageViews& operator=(EngineImageViews&&) noexcept = default;
  template <typename... Args>
  explicit EngineImageViews(const Device::LogicalDevice::EngineDevice* _device, Args... args)
  {
    creator(_device, args...);
  }

  template <typename... Args>
  void operator()(const Device::LogicalDevice::EngineDevice* _device, Args... args)
  {
    creator(_device, args...);
  }

  ~EngineImageViews()
  {
    destroyer();
  }

  VkImageView& operator[](size_t t);
  VkImageView operator[](size_t t) const;
  VkImageView* data();
  std::vector<VkImageView>::iterator begin();
  std::vector<VkImageView>::iterator end();
  [[nodiscard]] size_t size() const;
  size_t size();
  void destroyer();

  std::vector<VkImageView> image_views;
  const Device::LogicalDevice::EngineDevice* device{};

 private:
  void creator(const Device::LogicalDevice::EngineDevice* device,
               const std::vector<VkImage>& images,
               Data::ImageViewCreate_Info _info,
               const VkAllocationCallbacks* alloc = nullptr);
  const VkAllocationCallbacks* Alloc{};
};

}  // namespace SngoEngine::Core::Source::ImageView

#endif