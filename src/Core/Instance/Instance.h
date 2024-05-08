#ifndef __SNGO_INSTANCE_H
#define __SNGO_INSTANCE_H

#include <vulkan/vulkan_core.h>

#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "src/Core/Macro.h"

#define GLFW_USED
#include "GLFW/glfw3.h"

namespace SngoEngine::Core::Instance
{

bool IsExtension_Available(const std::vector<VkExtensionProperties>& properties,
                           const char* extension);
std::vector<const char*> Get_Required_Extensions(
    const std::vector<const char*>& required_extensions);
bool Check_Validation_Available();

VkBool32 VKAPI_CALL debug_call_back(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                                    VkDebugUtilsMessageTypeFlagsEXT message_type,
                                    const VkDebugUtilsMessengerCallbackDataEXT* p_call_back_data,
                                    void* p_user_data);

VkDebugUtilsMessengerCreateInfoEXT Populate_Debug_CreateInfo(
    decltype(debug_call_back) lambda = debug_call_back,
    VkDebugUtilsMessageSeverityFlagsEXT severity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                                                   | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                                                   | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
    VkDebugUtilsMessageTypeFlagsEXT type = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                                           | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                                           | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT);

struct EngineInstance
{
  EngineInstance() : instance(VK_NULL_HANDLE){};
  EngineInstance(EngineInstance&&) noexcept = default;
  EngineInstance& operator=(EngineInstance&&) noexcept = default;

  explicit EngineInstance(const std::string& _app_name,
                          const std::vector<const char*>& Instance_Exts = Macro::EMPTY_EXTS,
                          const VkAllocationCallbacks* _alloc = nullptr);
  void operator()(const std::string& _app_name,
                  const std::vector<const char*>& Instance_Exts = Macro::EMPTY_EXTS,
                  const VkAllocationCallbacks* _alloc = nullptr);

  template <typename U>
  U& operator=(U&) = delete;
  ~EngineInstance()
  {
    if (instance != VK_NULL_HANDLE)
      vkDestroyInstance(instance, Alloc);
  }

  std::string appName();
  const VkAllocationCallbacks* AllocationCallbacks();
  VkInstance instance;

 private:
  void creator(const std::string& _app_name,
               const std::vector<const char*>& Instance_Exts,
               const VkAllocationCallbacks* alloc);
  std::string app_name;
  const VkAllocationCallbacks* Alloc{};
};

struct EngineApplication
{
 private:
  std::shared_ptr<EngineInstance> pinstance;
  std::vector<VkPhysicalDevice> physical_devices;
};

}  // namespace SngoEngine::Core::Instance

#endif