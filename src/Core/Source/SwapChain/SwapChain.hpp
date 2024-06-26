#ifndef __SNGO_SWAPCHAIN_H
#define __SNGO_SWAPCHAIN_H
#include <vulkan/vulkan_core.h>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

#include "GLFW/glfw3.h"
#include "src/Core/Data.h"
#include "src/Core/Device/LogicalDevice.hpp"
#include "src/Core/Render/FrameBuffer.hpp"
#include "src/Core/Render/RenderPass.hpp"
#include "src/Core/Source/Image/ImageVIew.hpp"
#include "src/GLFWEXT/Surface.h"

namespace SngoEngine::Core::Source::SwapChain
{

struct SwapChainRequirement
{
  SwapChainRequirement() = delete;
  SwapChainRequirement(const Device::LogicalDevice::EngineDevice* _device, GLFWwindow* window);

  // Data::SwapChainSupportDetails swap_chain_support;
  VkSurfaceFormatKHR surface_format;
  VkPresentModeKHR present_mode;
  VkExtent2D optimal_extent;

  VkSurfaceTransformFlagBitsKHR transform;
  uint32_t img_count;
};

VkSurfaceFormatKHR Choose_SurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats);
VkPresentModeKHR Choose_PresentMode(const std::vector<VkPresentModeKHR>& available_present_mode);
VkExtent2D Choose_Extent(const VkSurfaceCapabilitiesKHR& capabilities, VkExtent2D _extent);
VkExtent2D Choose_Extent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);

VkSwapchainKHR Create_SwapChain(
    const SwapChainRequirement* req,
    const SngoEngine::Core::Device::LogicalDevice::EngineDevice* _device,
    const VkAllocationCallbacks* alloc);

std::vector<VkImage> Create_SwapChainImages(
    const SwapChainRequirement* req,
    const SngoEngine::Core::Device::LogicalDevice::EngineDevice* _device,
    VkSwapchainKHR swap_chain,
    const VkAllocationCallbacks* alloc);

//===========================================================================================================================
// EngineSwapChain
//===========================================================================================================================

struct EngineSwapChain
{
  EngineSwapChain() = default;
  EngineSwapChain(EngineSwapChain&&) noexcept = default;
  EngineSwapChain& operator=(EngineSwapChain&&) noexcept = default;
  template <typename... Args>
  explicit EngineSwapChain(const Device::LogicalDevice::EngineDevice* _device, Args... args)
  {
    creator(_device, args...);
  }
  template <typename... Args>
  void operator()(const Device::LogicalDevice::EngineDevice* _device, Args... args)
  {
    creator(_device, args...);
  }
  ~EngineSwapChain()
  {
    destroyer();
  }
  void cleanup_self();
  void destroyer();
  void Recreate_Self(GLFWwindow* window);

  VkSwapchainKHR swap_chain{};
  std::vector<VkImage> images;
  ImageView::EngineImageViews image_views;

  VkFormat image_format{};
  VkExtent2D extent{};
  const SngoEngine::Core::Device::LogicalDevice::EngineDevice* device{};

 private:
  void creator(const SngoEngine::Core::Device::LogicalDevice::EngineDevice* _device,
               const SwapChainRequirement& requirements,
               const VkAllocationCallbacks* alloc = nullptr);
  const VkAllocationCallbacks* Alloc{};
};

}  // namespace SngoEngine::Core::Source::SwapChain

#endif