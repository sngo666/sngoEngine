#include "LogicalDevice.hpp"

#include <vulkan/vulkan_core.h>

#include <stdexcept>
#include <vector>

#include "src/Core/Data.h"

bool SngoEngine::Core::Device::LogicalDevice::EngineDevice::ext_supported(const std::string& _ext) const
{
  return (pPD->extensions.find(_ext) != pPD->extensions.end());
}

void SngoEngine::Core::Device::LogicalDevice::EngineDevice::creator(
    PhysicalDevice::EnginePhysicalDevice* P_physical_device,
    VkSurfaceKHR _device_surface,
    const std::vector<const char*>& required_EXTs,
    const std::vector<const char*>& required_LAYERs,
    float queuePriority,
    const VkAllocationCallbacks* alloc)
{
  if (logical_device != VK_NULL_HANDLE)
    vkDestroyDevice(logical_device, Alloc);
  pPD = P_physical_device;
  device_surface = _device_surface;
  Alloc = alloc;

  Data::QueueFamilyIndices indices{SngoEngine::Core::Device::PhysicalDevice::Find_Queue_Families(
      pPD->physical_device, device_surface, VK_QUEUE_GRAPHICS_BIT)};

  queue_family = indices;

  VkPhysicalDeviceFeatures device_features{};
  // device_features.samplerAnisotropy = VK_TRUE;

  std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
  std::set<uint32_t> unique_queue_families = {indices.graphicsFamily.value(),
                                              indices.presentFamily.value()};

  for (uint32_t queueFamily : unique_queue_families)
    {
      VkDeviceQueueCreateInfo queue_create_info{};
      queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queue_create_info.queueFamilyIndex = queueFamily;
      queue_create_info.queueCount = 1;
      queue_create_info.pQueuePriorities = &queuePriority;
      queue_create_infos.push_back(queue_create_info);
    }

  VkDeviceCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  create_info.pQueueCreateInfos = queue_create_infos.data();
  create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
  create_info.pEnabledFeatures = &device_features;

  create_info.enabledExtensionCount = static_cast<uint32_t>(required_EXTs.size());
  create_info.ppEnabledExtensionNames = required_EXTs.data();
  create_info.enabledLayerCount = static_cast<uint32_t>(required_LAYERs.size());
  create_info.ppEnabledLayerNames = required_LAYERs.data();

  if (vkCreateDevice(pPD->physical_device, &create_info, Alloc, &logical_device) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create logical device!");
    }

  vkGetDeviceQueue(logical_device, indices.graphicsFamily.value(), 0, &graphics_queue);
  vkGetDeviceQueue(logical_device, indices.presentFamily.value(), 0, &present_queue);
}