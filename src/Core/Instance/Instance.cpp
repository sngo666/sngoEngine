
#include "Instance.hpp"

#include <vulkan/vulkan_core.h>

#include <iostream>
#include <ostream>
#include <utility>
#include <vector>

#include "src/Core/Utils/Utils.hpp"

using Debug_CallBack_Lambda = VkBool32 VKAPI_CALL (*)(VkDebugUtilsMessageSeverityFlagBitsEXT,
                                                      VkDebugUtilsMessageTypeFlagsEXT,
                                                      const VkDebugUtilsMessengerCallbackDataEXT*,
                                                      void*);

VkBool32 VKAPI_CALL SngoEngine::Core::Instance::debug_call_back(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* p_call_back_data,
    void* p_user_data)
{
  if (message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
      throw std::runtime_error("validation layers: " + std::string(p_call_back_data->pMessage)
                               + '\n');
    }
  return VK_FALSE;
};

bool SngoEngine::Core::Instance::Check_Validation_Available()
{
  bool layer_founded{false};
  uint32_t lay_count{0};
  Utils::Vk_Exception(vkEnumerateInstanceLayerProperties(&lay_count, nullptr));
  std::vector<VkLayerProperties> available_layers{lay_count};
  vkEnumerateInstanceLayerProperties(&lay_count, available_layers.data());
  for (const char* layer_name : Macro::ENGINE_LAYERS)
    {
      for (const auto& layer_properties : available_layers)
        {
          if (strcmp(layer_name, layer_properties.layerName) == 0)
            {
              layer_founded = true;
              break;
            }
        }
    }
  return layer_founded;
}

bool SngoEngine::Core::Instance::IsExtension_Available(
    const std::vector<VkExtensionProperties>& properties,
    const char* extension)
{
  for (const VkExtensionProperties& p : properties)
    if (strcmp(p.extensionName, extension) == 0)
      return true;
  return false;
}

std::vector<const char*> SngoEngine::Core::Instance::Get_Required_Extensions(
    const std::vector<std::string>& required_extensions)
{
  std::vector<const char*> extensions{};

  uint32_t properties_count;
  std::vector<VkExtensionProperties> properties{};
  Utils::Vk_Exception(vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, nullptr));
  properties.resize(properties_count);
  vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, properties.data());

  for (auto required_extension : required_extensions)
    {
      if (IsExtension_Available(properties, required_extension.c_str()))
        {
          extensions.push_back(required_extension.c_str());
        }
    }

  uint32_t glfw_extension_count{0};
  const char** glfw_extensions{glfwGetRequiredInstanceExtensions(&glfw_extension_count)};
  for (auto& elem : std::vector(glfw_extensions, glfw_extensions + glfw_extension_count))
    {
      extensions.push_back(elem);
    }

  if (Macro::ENABLE_VALIDATION_LAYERS)
    {
      extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

  return extensions;
}

VkDebugUtilsMessengerCreateInfoEXT SngoEngine::Core::Instance::Populate_Debug_CreateInfo(
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

  return create_info;
}

//===========================================================================================================================
// EngineInstance
//===========================================================================================================================

void SngoEngine::Core::Instance::EngineInstance::creator(const std::string& _app_name,
                                                         const std::vector<std::string>& _exts,
                                                         const VkAllocationCallbacks* alloc)
{
  Alloc = alloc;
  app_name = _app_name;

  if (Macro::ENABLE_VALIDATION_LAYERS && !Check_Validation_Available())
    {
      throw std::runtime_error("validation layers are requested, but bot available!");
    }

  VkApplicationInfo app_info{};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pNext = nullptr;
  app_info.pApplicationName = app_name.c_str();
  app_info.pEngineName = "SNGO-ENGINE";
  app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.apiVersion = VK_API_VERSION_1_3;

  VkInstanceCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  create_info.pApplicationInfo = &app_info;

  auto extensions{Get_Required_Extensions(_exts)};
#ifdef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
  uint32_t properties_count;
  std::vector<VkExtensionProperties> properties{};
  Utils::Vk_Exception(vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, nullptr));
  properties.resize(properties_count);
  vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, properties.data());

  if (IsExtension_Available(properties, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME))
    {
      extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
      create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    }
#endif

  create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  create_info.ppEnabledExtensionNames = extensions.data();

  VkDebugUtilsMessengerCreateInfoEXT debug_create_info{Populate_Debug_CreateInfo(debug_call_back)};
  if (Macro::ENABLE_VALIDATION_LAYERS)
    {
      create_info.enabledLayerCount = static_cast<uint32_t>(Macro::ENGINE_LAYERS.size());
      create_info.ppEnabledLayerNames = Macro::ENGINE_LAYERS.data();

      create_info.pNext = &debug_create_info;
    }
  else
    {
      create_info.pNext = nullptr;
      create_info.enabledLayerCount = 0;
    }

  if (vkCreateInstance(&create_info, Alloc, &instance) != VK_SUCCESS)
    {
      throw std::runtime_error("[err] failed to create vulkan instance");
    }
}

std::string SngoEngine::Core::Instance::EngineInstance::appName()
{
  if (instance != VK_NULL_HANDLE)
    {
      return app_name;
    }
  else
    {
      throw std::runtime_error("[err] attempt to get NULL instance");
    }
}

void SngoEngine::Core::Instance::EngineInstance::destroyer()
{
  vkDestroyInstance(instance, Alloc);
}