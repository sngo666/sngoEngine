#include "Surface.h"

#include <minwindef.h>
#include <vulkan/vulkan_win32.h>

#include <string>

VkSurfaceKHR SngoEngine::GlfwExt::Create_Surface(GLFWwindow* _window, VkInstance _instance)
{
  VkSurfaceKHR surface;
  VkWin32SurfaceCreateInfoKHR create_info{};

  create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
  create_info.hwnd = glfwGetWin32Window(_window);
  create_info.hinstance = GetModuleHandle(nullptr);

  if (vkCreateWin32SurfaceKHR(_instance, &create_info, nullptr, &surface) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create window surface!");
    }
  return surface;
}

void SngoEngine::GlfwExt::EngineGlfwSurface::creator(
    const Core::Instance::EngineInstance* _instance,
    VkExtent2D _extent,
    const std::string& _window_name,
    const VkAllocationCallbacks* alloc)
{
  destroyer();
  instance = _instance;
  window_title = _window_name;
  Alloc = alloc;

  window = glfwCreateWindow(static_cast<int>(_extent.width),
                            static_cast<int>(_extent.height),
                            window_title.c_str(),
                            nullptr,
                            nullptr);

  glfwSetWindowUserPointer(window, this);
  glfwSetFramebufferSizeCallback(window, frame_buffer_resize_callback<EngineGlfwSurface>);

  VkWin32SurfaceCreateInfoKHR create_info{};

  create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
  create_info.hwnd = glfwGetWin32Window(window);
  create_info.hinstance = GetModuleHandle(nullptr);

  if (vkCreateWin32SurfaceKHR(instance->instance, &create_info, Alloc, &surface) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create window surface!");
    }
}

std::string SngoEngine::GlfwExt::EngineGlfwSurface::title()
{
  return window_title;
}

VkExtent2D SngoEngine::GlfwExt::EngineGlfwSurface::extent() const
{
  int width = 0, height = 0;
  glfwGetFramebufferSize(window, &width, &height);
  return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
}

void SngoEngine::GlfwExt::EngineGlfwSurface::resize(VkExtent2D new_extent)
{
  glfwSetWindowSize(
      window, static_cast<int>(new_extent.width), static_cast<int>(new_extent.height));
}

void SngoEngine::GlfwExt::EngineGlfwSurface::destroyer()
{
  if (surface != VK_NULL_HANDLE)
    vkDestroySurfaceKHR(instance->instance, surface, nullptr);
  if (window != nullptr)
    {
      glfwDestroyWindow(window);
    }
}
