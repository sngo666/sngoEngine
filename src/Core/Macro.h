#ifndef __SNGO_MACRO_H
#define __SNGO_MACRO_H

#include <vulkan/vulkan_core.h>

#include <string>
#include <vector>

namespace SngoEngine::Core::Macro
{
const int32_t MAX_FRAMES_IN_FLIGHT = 2;

const bool ENABLE_VALIDATION_LAYERS = true;

const std::vector<const char*> ENGINE_LAYERS{
    "VK_LAYER_KHRONOS_validation",
};

const std::vector<std::string> DEVICE_REQUIRED_EXTS = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

const std::vector<const char*> EMPTY_EXTS = {};

}  // namespace SngoEngine::Core::Macro

#endif