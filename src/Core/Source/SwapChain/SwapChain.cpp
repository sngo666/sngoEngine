#include "SwapChain.hpp"

#include <vulkan/vulkan_core.h>

#include <iostream>
#include <stdexcept>

#include "src/Core/Data.h"
#include "src/Core/Device/LogicalDevice.hpp"
#include "src/Core/Device/PhysicalDevice.hpp"
#include "src/Core/Render/FrameBuffer.hpp"
#include "src/Core/Render/RenderPass.hpp"
#include "src/Core/Source/Image/ImageVIew.hpp"
#include "src/GLFWEXT/Surface.h"

// extern template SngoEngine::Core::Source::ImageView::EngineImageView::EngineImageView<>(
//     SngoEngine::Core::Device::LogicalDevice::EngineDevice const*,
//     std::vector<VkImage_T*, std::allocator<VkImage_T*> >,
//     SngoEngine::Core::Data::ImageViewCreate_Info,
//     VkAllocationCallbacks const*);

SngoEngine::Core::Source::SwapChain::SwapChainRequirement::SwapChainRequirement(
    const SngoEngine::Core::Device::LogicalDevice::EngineDevice* _device,
    GLFWwindow* window)
    : surface_format(), present_mode(), optimal_extent(), img_count()
{
  Data::SwapChainSupportDetails swap_chain_support = {
      Core::Device::PhysicalDevice::Query_SwapChain_Support(_device->pPD->physical_device,
                                                            _device->device_surface)};
  surface_format = Source::SwapChain::Choose_SurfaceFormat(swap_chain_support.formats);
  present_mode = Source::SwapChain::Choose_PresentMode(swap_chain_support.present_modes);
  optimal_extent = Source::SwapChain::Choose_Extent(swap_chain_support.capabilities, window);
  transform = swap_chain_support.capabilities.currentTransform;

  uint32_t _img_count{swap_chain_support.capabilities.minImageCount + 1};
  if (swap_chain_support.capabilities.maxImageCount > 0
      && _img_count > swap_chain_support.capabilities.maxImageCount)
    {
      _img_count = swap_chain_support.capabilities.maxImageCount;
    }
  img_count = _img_count;
}

VkSurfaceFormatKHR SngoEngine::Core::Source::SwapChain::Choose_SurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& available_formats)
{
  for (const auto& format : available_formats)
    {
      if (format.format == VK_FORMAT_B8G8R8_SRGB
          && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
          return format;
        }
    }
  // return first format if failed
  return available_formats[0];
}

VkPresentModeKHR SngoEngine::Core::Source::SwapChain::Choose_PresentMode(
    const std::vector<VkPresentModeKHR>& available_present_mode)
{
  for (const auto& alternative_mode : available_present_mode)
    {
      if (alternative_mode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
          return VK_PRESENT_MODE_MAILBOX_KHR;
        }
    }
  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D SngoEngine::Core::Source::SwapChain::Choose_Extent(
    const VkSurfaceCapabilitiesKHR& capabilities,
    VkExtent2D _extent)
{
  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
      return capabilities.currentExtent;
    }
  else
    {
      int32_t width{static_cast<int32_t>(_extent.width)},
          height{static_cast<int32_t>(_extent.height)};

      VkExtent2D actual_extent{static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

      actual_extent.width = std::clamp(actual_extent.width,
                                       capabilities.minImageExtent.width,
                                       capabilities.maxImageExtent.width);
      actual_extent.height = std::clamp(actual_extent.height,
                                        capabilities.minImageExtent.height,
                                        capabilities.maxImageExtent.height);
      return actual_extent;
    }
}

VkExtent2D SngoEngine::Core::Source::SwapChain::Choose_Extent(
    const VkSurfaceCapabilitiesKHR& capabilities,
    GLFWwindow* window)
{
  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
      return capabilities.currentExtent;
    }
  else
    {
      int32_t width{}, height{};
      glfwGetFramebufferSize(window, &width, &height);

      VkExtent2D actual_extent{static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

      actual_extent.width = std::clamp(actual_extent.width,
                                       capabilities.minImageExtent.width,
                                       capabilities.maxImageExtent.width);
      actual_extent.height = std::clamp(actual_extent.height,
                                        capabilities.minImageExtent.height,
                                        capabilities.maxImageExtent.height);
      return actual_extent;
    }
}

VkSwapchainKHR SngoEngine::Core::Source::SwapChain::Create_SwapChain(
    const SwapChainRequirement* req,
    const SngoEngine::Core::Device::LogicalDevice::EngineDevice* _device,
    const VkAllocationCallbacks* alloc)
{
  VkSwapchainKHR swap_chain;

  VkSwapchainCreateInfoKHR create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  create_info.surface = _device->device_surface;

  create_info.minImageCount = req->img_count;
  create_info.imageFormat = req->surface_format.format;
  create_info.imageColorSpace = req->surface_format.colorSpace;
  create_info.presentMode = req->present_mode;
  create_info.imageExtent = req->optimal_extent;
  create_info.imageArrayLayers = 1;
  create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  Data::QueueFamilyIndices indices{Device::PhysicalDevice::Find_Queue_Families(
      _device->pPD->physical_device, _device->device_surface, VK_QUEUE_GRAPHICS_BIT)};

  std::vector<uint32_t> queue_family_indices{};
  queue_family_indices.emplace_back(indices.graphicsFamily.value());
  queue_family_indices.emplace_back(indices.presentFamily.value());

  if (indices.graphicsFamily != indices.presentFamily)
    {
      create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
      create_info.queueFamilyIndexCount = 2;
      create_info.pQueueFamilyIndices = queue_family_indices.data();
    }
  else
    {
      create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
      create_info.queueFamilyIndexCount = 0;
      create_info.pQueueFamilyIndices = nullptr;
    }

  create_info.preTransform = req->transform;
  create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

  create_info.oldSwapchain = VK_NULL_HANDLE;

  if (vkCreateSwapchainKHR(_device->logical_device, &create_info, alloc, &swap_chain) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create swap chain!");
    }

  return swap_chain;
}

std::vector<VkImage> SngoEngine::Core::Source::SwapChain::Create_SwapChainImages(
    const SwapChainRequirement* req,
    const SngoEngine::Core::Device::LogicalDevice::EngineDevice* _device,
    VkSwapchainKHR swap_chain,
    const VkAllocationCallbacks* alloc)
{
  std::vector<VkImage> images;

  Data::SwapChainSupportDetails swap_chain_support{
      Core::Device::PhysicalDevice::Query_SwapChain_Support(_device->pPD->physical_device,
                                                            _device->device_surface)};
  uint32_t img_count{swap_chain_support.capabilities.minImageCount + 1};
  if (swap_chain_support.capabilities.maxImageCount > 0
      && img_count > swap_chain_support.capabilities.maxImageCount)
    {
      img_count = swap_chain_support.capabilities.maxImageCount;
    }

  vkGetSwapchainImagesKHR(_device->logical_device, swap_chain, &img_count, nullptr);
  images.resize(img_count);
  vkGetSwapchainImagesKHR(_device->logical_device, swap_chain, &img_count, images.data());

  return images;
}

//===========================================================================================================================
// EngineSwapChain
//===========================================================================================================================

void SngoEngine::Core::Source::SwapChain::EngineSwapChain::creator(
    const SngoEngine::Core::Device::LogicalDevice::EngineDevice* _device,
    const SwapChainRequirement& requirements,
    const VkAllocationCallbacks* alloc)
{
  destroyer();

  device = _device;
  Alloc = alloc;
  image_format = requirements.surface_format.format;
  extent = requirements.optimal_extent;

  swap_chain = Create_SwapChain(&requirements, device, Alloc);

  images = {Create_SwapChainImages(&requirements, device, swap_chain, Alloc)};

  Data::ImageViewCreate_Info imageViews_info{
      nullptr, image_format, Data::DEFAULT_COLOR_IMAGE_SUBRESOURCE_INFO};
  image_views.init(device, images, imageViews_info, Alloc);
}

void SngoEngine::Core::Source::SwapChain::EngineSwapChain::Recreate_Self(GLFWwindow* window)
{
  destroyer();

  SwapChainRequirement requirements{device, window};
  swap_chain = Create_SwapChain(&requirements, device, Alloc);

  images = {Create_SwapChainImages(&requirements, device, swap_chain, Alloc)};

  Data::ImageViewCreate_Info imageViews_info{
      nullptr, image_format, Data::DEFAULT_COLOR_IMAGE_SUBRESOURCE_INFO};
  image_views = ImageView::EngineImageViews(device, images, imageViews_info, Alloc);
}

void SngoEngine::Core::Source::SwapChain::EngineSwapChain::cleanup_self()
{
  destroyer();
}

void SngoEngine::Core::Source::SwapChain::EngineSwapChain::destroyer()
{
  if (device)
    {
      for (auto img : images)
        vkDestroyImage(device->logical_device, img, Alloc);
      image_views.destroyer();
      vkDestroySwapchainKHR(device->logical_device, swap_chain, Alloc);
    }
}
