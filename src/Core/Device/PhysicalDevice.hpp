#ifndef __SNGO_PHYSICAL_DEVICE_H
#define __SNGO_PHYSICAL_DEVICE_H

#include <vulkan/vulkan_core.h>

#include <cstddef>
#include <unordered_set>

#include "src/Core/Data.h"
#include "src/Core/Instance/Instance.hpp"
#include "src/GLFWEXT/Surface.h"

namespace SngoEngine::Core::Device::PhysicalDevice
{

Data::SwapChainSupportDetails Query_SwapChain_Support(VkPhysicalDevice physical_device,
                                                      VkSurfaceKHR surface);

bool Check_PhysicalDevice_ExtensionSupport(
    VkPhysicalDevice physical_device,
    const std::vector<std::string>& extension = Macro::DEVICE_REQUIRED_EXTS);

Data::QueueFamilyIndices Find_Queue_Families(VkPhysicalDevice physical_device,
                                             VkSurfaceKHR surface,
                                             VkQueueFlagBits requirement = VK_QUEUE_GRAPHICS_BIT);

uint32_t IsDevice_Suitable(VkPhysicalDevice physical_device, VkSurfaceKHR surface);

//===========================================================================================================================
// EnginePhysicalDevice
//===========================================================================================================================

struct EnginePhysicalDevice
{
  EnginePhysicalDevice() = default;
  EnginePhysicalDevice(EnginePhysicalDevice&&) noexcept = default;
  EnginePhysicalDevice& operator=(EnginePhysicalDevice&&) noexcept = default;

  template <class... Args>
  explicit EnginePhysicalDevice(Args... args)
  {
    creator(args...);
  }
  template <class... Args>
  void init(Args... args)
  {
    creator(args...);
  }
  VkPhysicalDevice operator()() const
  {
    return physical_device;
  }
  explicit operator VkPhysicalDevice() const
  {
    return physical_device;
  }
  ~EnginePhysicalDevice() = default;

  VkPhysicalDevice physical_device{};
  const Instance::EngineInstance* instance{};
  VkSurfaceKHR surface{};

  std::unordered_set<std::string> extensions;
  VkPhysicalDeviceProperties properties{};
  VkPhysicalDeviceFeatures enabled_features{};

 private:
  void creator(const Instance::EngineInstance* _instance,
               VkSurfaceKHR _surface,
               const VkAllocationCallbacks* alloc = nullptr);
  const VkAllocationCallbacks* Alloc{};
};

}  // namespace SngoEngine::Core::Device::PhysicalDevice

#endif