#ifndef __SNGO_SAMPLER_H
#define __SNGO_SAMPLER_H

#include <vulkan/vulkan_core.h>

#include "src/Core/Device/LogicalDevice.hpp"

namespace SngoEngine::Core::Source::Image
{

VkBool32 FormatIs_Filterable(VkPhysicalDevice physicalDevice,
                             VkFormat format,
                             VkImageTiling tiling);

//===========================================================================================================================
// EngineSampler
//===========================================================================================================================

VkSamplerCreateInfo Get_Default_Sampler(
    const Device::LogicalDevice::EngineDevice* device,
    float mip_level = 1.0f,
    VkSamplerAddressMode _address_mode = VK_SAMPLER_ADDRESS_MODE_REPEAT);

VkSamplerCreateInfo Get_CubeTex_Sampler(const Device::LogicalDevice::EngineDevice* device,
                                        float mip_level);

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
  void init(const Device::LogicalDevice::EngineDevice* _device, Args... args)
  {
    creator(_device, args...);
  }
  VkSampler operator()() const
  {
    return sampler;
  }

  ~EngineSampler()
  {
    destroyer();
  }
  void destroyer();

  VkSampler sampler{};
  const Device::LogicalDevice::EngineDevice* device{};

 private:
  void creator(const Device::LogicalDevice::EngineDevice* _device,
               VkSamplerCreateInfo& _sampler_info,
               const VkAllocationCallbacks* alloc = nullptr);
  const VkAllocationCallbacks* Alloc{};
};
}  // namespace SngoEngine::Core::Source::Image
#endif