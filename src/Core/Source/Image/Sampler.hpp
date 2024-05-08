#ifndef __SNGO_SAMPLER_H
#define __SNGO_SAMPLER_H

#include <vulkan/vulkan_core.h>

#include "src/Core/Device/LogicalDevice.hpp"

namespace SngoEngine::Core::Source::Image
{

VkSamplerCreateInfo Get_Default_Sampler(const Device::LogicalDevice::EngineDevice* device,
                                        float mip_level = 0.0f);

struct EngineSampler
{
  EngineSampler() = default;
  EngineSampler(EngineSampler&&) noexcept = default;
  EngineSampler& operator=(EngineSampler&&) noexcept = default;

  template <class... Args>
  explicit EngineSampler(const Device::LogicalDevice::EngineDevice* _device, Args... args)
  {
    creator(_device, args...);
  }

  template <class... Args>
  void operator()(const Device::LogicalDevice::EngineDevice* _device, Args... args)
  {
    creator(_device, args...);
  }
  ~EngineSampler() {}

  void destroyer();

  VkSampler sampler{};
  const Device::LogicalDevice::EngineDevice* device{};

 private:
  void creator(const Device::LogicalDevice::EngineDevice* _device,
               VkSamplerCreateInfo& _sampler_info,
               const VkAllocationCallbacks* alloc);
  const VkAllocationCallbacks* Alloc{};
};
}  // namespace SngoEngine::Core::Source::Image
#endif