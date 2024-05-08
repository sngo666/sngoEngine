#ifndef __SNGO_SURFACE_H
#define __SNGO_SURFACE_H

#include <vulkan/vulkan_core.h>

#include <stdexcept>
#include <string>
#include <utility>

#include "src/Core/Instance/Instance.h"
#include "src/Core/Utils/Utils.hpp"

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

namespace SngoEngine::GlfwExt
{

VkSurfaceKHR Create_Surface(GLFWwindow* _window, VkInstance _instance);

template <typename T>
void frame_buffer_resize_callback(GLFWwindow* window, int width, int height)
{
  auto app = reinterpret_cast<T*>(glfwGetWindowUserPointer(window));
  glfwSetWindowSize(window, width, height);
  app->FrameBuffer_Resized = true;
}

struct EngineGlfwSurface
{
  EngineGlfwSurface() = default;
  EngineGlfwSurface(EngineGlfwSurface&&) noexcept = default;
  EngineGlfwSurface& operator=(EngineGlfwSurface&&) noexcept = default;
  EngineGlfwSurface(const Core::Instance::EngineInstance* _instance,
                    VkExtent2D _extent,
                    const std::string& _window_name,
                    const VkAllocationCallbacks* alloc = nullptr);
  void operator()(const Core::Instance::EngineInstance* _instance,
                  VkExtent2D _extent,
                  const std::string& _window_name,
                  const VkAllocationCallbacks* alloc = nullptr);
  template <typename U>
  U& operator=(U&) = delete;
  ~EngineGlfwSurface()
  {
    if (surface != VK_NULL_HANDLE)
      vkDestroySurfaceKHR(instance->instance, surface, nullptr);
    if (window != nullptr)
      {
        glfwDestroyWindow(window);
      }
  }

  std::string title();
  VkExtent2D extent();
  void resize(VkExtent2D new_extent);
  const VkAllocationCallbacks* AllocationCallbacks();

  VkSurfaceKHR surface{};
  GLFWwindow* window{};
  const Core::Instance::EngineInstance* instance{};
  bool FrameBuffer_Resized{};

 private:
  void creator(const Core::Instance::EngineInstance* _instance,
               VkExtent2D _extent,
               const std::string& _window_name,
               const VkAllocationCallbacks* alloc);
  std::string window_title;
  const VkAllocationCallbacks* Alloc{};
};
}  // namespace SngoEngine::GlfwExt
#endif