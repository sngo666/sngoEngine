#include "PhysicalDevice.hpp"

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <set>
#include <vector>

#include "src/Core/Data.h"
#include "src/Core/Source/SwapChain/SwapChain.hpp"
#include "src/Core/Utils/Utils.hpp"

uint32_t SngoEngine::Core::Device::PhysicalDevice::IsDevice_Suitable(
    VkPhysicalDevice physical_device,
    VkSurfaceKHR surface)
{
  VkPhysicalDeviceProperties device_properties;
  VkPhysicalDeviceFeatures device_features;

  vkGetPhysicalDeviceProperties(physical_device, &device_properties);
  vkGetPhysicalDeviceFeatures(physical_device, &device_features);

  bool extension_availablity{Check_PhysicalDevice_ExtensionSupport(physical_device)};
  uint32_t score{0};

  bool swap_chain_adequate{false};
  if (extension_availablity)
    {
      Data::SwapChainSupportDetails details{Query_SwapChain_Support(physical_device, surface)};

      swap_chain_adequate = (!details.formats.empty() && !details.present_modes.empty());
      extension_availablity = swap_chain_adequate;
    }

  if (!device_features.geometryShader
      || !(Find_Queue_Families(physical_device, surface, VK_QUEUE_GRAPHICS_BIT).is_complete())
      || !extension_availablity)
    {
      return 0;
    }
  else
    {
      if (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
          score += 1000;
        }
      score += device_properties.limits.maxImageDimension2D;
    }

  return score;
}

bool SngoEngine::Core::Device::PhysicalDevice::Check_PhysicalDevice_ExtensionSupport(
    VkPhysicalDevice physical_device,
    const std::vector<std::string>& _extension)
{
  uint32_t extension_count;

  vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, nullptr);

  std::vector<VkExtensionProperties> available_extensions{extension_count};
  vkEnumerateDeviceExtensionProperties(
      physical_device, nullptr, &extension_count, available_extensions.data());

  std::set<std::string> required_extensions{_extension.begin(), _extension.end()};

  for (const auto& extension_name : available_extensions)
    {
      required_extensions.erase(extension_name.extensionName);
    }

  return required_extensions.empty();
}

SngoEngine::Core::Data::SwapChainSupportDetails
SngoEngine::Core::Device::PhysicalDevice::Query_SwapChain_Support(VkPhysicalDevice physical_device,
                                                                  VkSurfaceKHR surface)
{
  Data::SwapChainSupportDetails swap_chain_details;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
      physical_device, surface, &swap_chain_details.capabilities);

  uint32_t format_count;
  vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, nullptr);

  if (format_count != 0)
    {
      swap_chain_details.formats.resize(format_count);
      vkGetPhysicalDeviceSurfaceFormatsKHR(
          physical_device, surface, &format_count, swap_chain_details.formats.data());
    }

  uint32_t present_mode_count;
  vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, nullptr);
  if (present_mode_count != 0)
    {
      swap_chain_details.present_modes.resize(present_mode_count);
      vkGetPhysicalDeviceSurfacePresentModesKHR(
          physical_device, surface, &present_mode_count, swap_chain_details.present_modes.data());
    }

  return swap_chain_details;
}

SngoEngine::Core::Data::QueueFamilyIndices
SngoEngine::Core::Device::PhysicalDevice::Find_Queue_Families(VkPhysicalDevice physical_device,
                                                              VkSurfaceKHR surface,
                                                              VkQueueFlagBits requirement)
{
  Data::QueueFamilyIndices indices;

  uint32_t queue_families_count{0};
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_families_count, nullptr);
  std::vector<VkQueueFamilyProperties> queue_families{queue_families_count};

  vkGetPhysicalDeviceQueueFamilyProperties(
      physical_device, &queue_families_count, queue_families.data());

  uint32_t i{0};
  for (const auto& queue_family : queue_families)
    {
      if (queue_family.queueFlags & requirement)
        {
          indices.graphicsFamily = i;
        }
      VkBool32 present_support = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &present_support);

      if (present_support)
        {
          indices.presentFamily = i;
        }

      if (indices.is_complete())
        {
          break;
        }

      i++;
    }

  return indices;
}

//===========================================================================================================================
// EnginePhysicalDevice
//===========================================================================================================================

void SngoEngine::Core::Device::PhysicalDevice::EnginePhysicalDevice::creator(
    const Instance::EngineInstance* _instance,
    VkSurfaceKHR _surface,
    const VkAllocationCallbacks* alloc)
{
  instance = _instance;
  surface = _surface;
  Alloc = alloc;

  uint32_t device_count{0};
  Utils::Vk_Exception(vkEnumeratePhysicalDevices(instance->instance, &device_count, nullptr));

  if (device_count == 0)
    {
      throw std::runtime_error("fail to find available GPUs with Vulkan support!");
    }

  std::vector<VkPhysicalDevice> devices{device_count};
  vkEnumeratePhysicalDevices(instance->instance, &device_count, devices.data());

  for (const auto& device : devices)
    {
      //  just get more strict strategy to get best GPU device
      auto score = IsDevice_Suitable(device, surface);

      if (score > 0)
        {
          physical_device = device;
          break;
        }
    }

  if (physical_device == VK_NULL_HANDLE)
    {
      throw std::runtime_error("failed to find a suitable GPU!");
    }

  vkGetPhysicalDeviceProperties(physical_device, &this->properties);
  vkGetPhysicalDeviceFeatures(physical_device, &this->enabled_features);

  uint32_t ext_count{0};
  vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &ext_count, nullptr);
  if (ext_count > 0)
    {
      std::vector<VkExtensionProperties> exts(ext_count);
      if (vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &ext_count, &exts.front())
          == VK_SUCCESS)
        {
          for (auto& ext : exts)
            {
              extensions.insert(ext.extensionName);
            }
        }
    }
}
