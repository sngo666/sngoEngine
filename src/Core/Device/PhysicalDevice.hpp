#ifndef __SNGO_PHYSICAL_DEVICE_H
#define __SNGO_PHYSICAL_DEVICE_H

#include <vulkan/vulkan_core.h>

#include <cstddef>

#include "src/Core/Data.h"
#include "src/Core/Instance/Instance.h"
#include "src/GLFWEXT/Surface.h"

namespace SngoEngine::Core::Device::PhysicalDevice
{

Data::SwapChainSupportDetails Query_SwapChain_Support(VkPhysicalDevice physical_device,
                                                      VkSurfaceKHR surface);

bool Check_PhysicalDevice_ExtensionSupport(const VkPhysicalDevice& physical_device);
bool Check_PhysicalDevice_ExtensionSupport(const VkPhysicalDevice& physical_device,
                                           const char* extension);

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
  void operator()(Args... args)
  {
    creator(args...);
  }

  explicit operator VkPhysicalDevice() const
  {
    return physical_device;
  }

  template <typename U>
  U& operator=(U&) = delete;

  VkPhysicalDevice physical_device{VK_NULL_HANDLE};
  const Instance::EngineInstance* instance{};
  VkSurfaceKHR surface{};

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