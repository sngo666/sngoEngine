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
    const std::vector<std::string>& required_extensions);
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

//===========================================================================================================================
// EngineInstance
//===========================================================================================================================

struct EngineInstance
{
  EngineInstance() = default;
  EngineInstance(EngineInstance&&) noexcept = default;
  EngineInstance& operator=(EngineInstance&&) noexcept = default;
  template <class... Args>
  explicit EngineInstance(const std::string& _app_name,
                          const std::vector<std::string>& _exts,
                          Args... args)
  {
    creator(_app_name, _exts, args...);
  }
  template <class... Args>
  void init(const std::string& _app_name, const std::vector<std::string>& _exts, Args... args)
  {
    creator(_app_name, _exts, args...);
  }
  ~EngineInstance()
  {
    destroyer();
  }
  void destroyer();
  std::string appName();

  VkInstance instance{};

 private:
  void creator(const std::string& _app_name,
               const std::vector<std::string>& _exts,
               const VkAllocationCallbacks* alloc = nullptr);
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