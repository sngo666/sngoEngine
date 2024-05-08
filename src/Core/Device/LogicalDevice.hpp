#ifndef __SNGO_LOGICAL_DEVICE_H
#define __SNGO_LOGICAL_DEVICE_H

#include <vulkan/vulkan_core.h>

#include <cstddef>
#include <optional>
#include <set>

#include "src/Core/Device/PhysicalDevice.hpp"
#include "src/Core/Macro.h"

namespace SngoEngine::Core::Device::LogicalDevice
{

struct EngineDevice
{
  EngineDevice() = default;
  EngineDevice(EngineDevice&&) noexcept = default;
  EngineDevice& operator=(EngineDevice&&) noexcept = default;

  template <class... Args>
  explicit EngineDevice(Args... args)
  {
    creator(args...);
  }

  template <class... Args>
  void operator()(Args... args)
  {
    creator(args...);
  }
  template <typename U>
  U& operator=(U&) = delete;
  ~EngineDevice()
  {
    if (logical_device != VK_NULL_HANDLE)
      vkDestroyDevice(logical_device, Alloc);
  };

  Data::QueueFamilyIndices queue_family;
  VkSurfaceKHR device_surface{};
  PhysicalDevice::EnginePhysicalDevice* pPD{};
  VkDevice logical_device{};
  VkQueue graphics_queue{};
  VkQueue present_queue{};

 private:
  void creator(PhysicalDevice::EnginePhysicalDevice* _physical_device,
               VkSurfaceKHR _device_surface,
               const std::vector<const char*>& required_EXTs,
               const std::vector<const char*>& required_LAYERs,
               float queuePriority = 0.0f,
               const VkAllocationCallbacks* alloc = nullptr);
  const VkAllocationCallbacks* Alloc{};
};
}  // namespace SngoEngine::Core::Device::LogicalDevice

#endif