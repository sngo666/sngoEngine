#include "DebugMessenger.h"

#include <vulkan/vulkan_core.h>

#include <iostream>

#include "fmt/core.h"

VkDebugUtilsMessengerCreateInfoEXT
SngoEngine::Core::Instance::DebugMessenger::Populate_Debug_CreateInfo(
    Debug_CallBack_Lambda lambda,
    VkDebugUtilsMessageSeverityFlagsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type)
{
  VkDebugUtilsMessengerCreateInfoEXT create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  create_info.messageSeverity = severity;
  create_info.messageType = type;
  create_info.pfnUserCallback = lambda;
  create_info.pUserData = nullptr;
  create_info.pNext = nullptr;
  create_info.flags = 0;

  return create_info;
}

SngoEngine::Core::Instance::DebugMessenger::EngineDebugMessenger::EngineDebugMessenger(
    const EngineInstance* _instance,
    Debug_CallBack_Lambda lambda,
    const VkAllocationCallbacks* alloc)
    : EngineDebugMessenger()
{
  creator(_instance, lambda, alloc);
}

void SngoEngine::Core::Instance::DebugMessenger::EngineDebugMessenger::operator()(
    const EngineInstance* _instance,
    Debug_CallBack_Lambda lambda,
    const VkAllocationCallbacks* alloc)
{
  creator(_instance, lambda, alloc);
}

void SngoEngine::Core::Instance::DebugMessenger::EngineDebugMessenger::creator(
    const EngineInstance* _instance,
    Debug_CallBack_Lambda lambda,
    const VkAllocationCallbacks* alloc)
{
  if (debug_messenger != VK_NULL_HANDLE)
    DestroyDebugUtilsMessengerEXT(instance->instance, &debug_messenger, Alloc);

  instance = _instance;
  Alloc = alloc;
  auto create_info{Populate_Debug_CreateInfo(lambda)};

  if (CreateDebugUtilsMessengerEXT(instance->instance, &create_info, alloc, &debug_messenger)
      != VK_SUCCESS)
    {
      throw std::runtime_error("[err] failed to create debug messenger");
    }
}

const VkAllocationCallbacks*
SngoEngine::Core::Instance::DebugMessenger::EngineDebugMessenger::AllocationCallbacks()
{
  return Alloc;
}

VKAPI_ATTR VkBool32 VKAPI_CALL debug_report(VkDebugReportFlagsEXT flags,
                                            VkDebugReportObjectTypeEXT objectType,
                                            uint64_t object,
                                            size_t location,
                                            int32_t messageCode,
                                            const char* pLayerPrefix,
                                            const char* pMessage,
                                            void* pUserData)
{
  (void)flags;
  (void)object;
  (void)location;
  (void)messageCode;
  (void)pUserData;
  (void)pLayerPrefix;  // Unused arguments
  fprintf(
      stderr, "[vulkan] Debug report from ObjectType: %i\nMessage: %s\n\n", objectType, pMessage);
  return VK_FALSE;
}
