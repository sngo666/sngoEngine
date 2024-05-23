#ifndef __SNGO_DEBUG_MESSENGER_H
#define __SNGO_DEBUG_MESSENGER_H

#include <vulkan/vulkan_core.h>

#include <iostream>
#include <stdexcept>

#include "fmt/core.h"
#include "src/Core/Instance/Instance.hpp"
#include "src/Core/Macro.h"

namespace SngoEngine::Core::Instance::DebugMessenger
{

using Debug_CallBack_Lambda = VkBool32 VKAPI_CALL (*)(VkDebugUtilsMessageSeverityFlagBitsEXT,
                                                      VkDebugUtilsMessageTypeFlagsEXT,
                                                      const VkDebugUtilsMessengerCallbackDataEXT*,
                                                      void*);
static VKAPI_ATTR VkBool32 VKAPI_CALL
debug_call_back(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                VkDebugUtilsMessageTypeFlagsEXT message_type,
                const VkDebugUtilsMessengerCallbackDataEXT* p_call_back_data,
                void* p_user_data)
{
  if (message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
      std::cerr << "validation layers: " << p_call_back_data->pMessage << '\n';
    }
  return VK_FALSE;
}

const auto DEFAULT_MESSAGER_SEVERITY{VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                                     | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                                     | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT};

const auto DEFAULT_MESSAGE_TYPE{VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                                | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                                | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT};
//=====================================================================================================

static VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* p_create_info,
    const VkAllocationCallbacks* p_allocator,
    VkDebugUtilsMessengerEXT* p_debug_messenger)
{
  auto func{(PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkCreateDebugUtilsMessengerEXT")};

  if (func != nullptr)
    {
      return func(instance, p_create_info, p_allocator, p_debug_messenger);
    }
  else
    {
      return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

static void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                          VkDebugUtilsMessengerEXT* p_debug_messenger,
                                          const VkAllocationCallbacks* p_allocator)
{
  auto func{(PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkDestroyDebugUtilsMessengerEXT")};

  if (func != nullptr)
    {
      func(instance, *p_debug_messenger, p_allocator);
    }
}
VkDebugUtilsMessengerCreateInfoEXT Populate_Debug_CreateInfo(
    Debug_CallBack_Lambda lambda,
    VkDebugUtilsMessageSeverityFlagsEXT severity = DEFAULT_MESSAGER_SEVERITY,
    VkDebugUtilsMessageTypeFlagsEXT type = DEFAULT_MESSAGE_TYPE);

//===========================================================================================================================
// EngineDebugMessenger
//===========================================================================================================================

struct EngineDebugMessenger
{
  EngineDebugMessenger() = default;
  EngineDebugMessenger(EngineDebugMessenger&&) noexcept = default;
  EngineDebugMessenger& operator=(EngineDebugMessenger&&) noexcept = default;
  template <class... Args>
  explicit EngineDebugMessenger(const EngineInstance* _instance, Args... args)
  {
    creator(_instance, args...);
  }
  template <class... Args>
  void init(const EngineInstance* _instance, Args... args)
  {
    creator(_instance, args...);
  }
  ~EngineDebugMessenger()
  {
    destroyer();
  }
  void destroyer();

  VkDebugUtilsMessengerEXT debug_messenger{};
  const EngineInstance* instance{};

 private:
  void creator(const EngineInstance* _instance,
               Debug_CallBack_Lambda lambda,
               const VkAllocationCallbacks* alloc = nullptr);
  const VkAllocationCallbacks* Alloc{};
};

struct EngineDebugReport
{
  // TODO:
  VkDebugReportCallbackEXT debugReporter_callBack;

 private:
  VkInstance instance;
};

}  // namespace SngoEngine::Core::Instance::DebugMessenger
#endif