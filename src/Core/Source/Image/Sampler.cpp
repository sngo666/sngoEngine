#include "Sampler.hpp"

VkSamplerCreateInfo SngoEngine::Core::Source::Image::Get_Default_Sampler(
    const Device::LogicalDevice::EngineDevice* device,
    float mip_level)
{
  VkSamplerCreateInfo sampler_info{};
  sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  sampler_info.magFilter = VK_FILTER_LINEAR;
  sampler_info.minFilter = VK_FILTER_LINEAR;
  sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_info.anisotropyEnable = VK_FALSE;
  sampler_info.maxAnisotropy = device->pPD->enabled_features.samplerAnisotropy
                                   ? device->pPD->properties.limits.maxSamplerAnisotropy
                                   : 1.0f;

  VkPhysicalDeviceProperties properties{};
  vkGetPhysicalDeviceProperties(device->pPD->physical_device, &properties);

  sampler_info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
  sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  sampler_info.unnormalizedCoordinates = VK_FALSE;
  sampler_info.compareEnable = VK_FALSE;
  sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;

  sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  sampler_info.mipLodBias = 0.0f;
  sampler_info.minLod = 0.0f;
  sampler_info.maxLod = 0.0f;

  return sampler_info;
}

void SngoEngine::Core::Source::Image::EngineSampler::destroyer()
{
  if (sampler != VK_NULL_HANDLE)
    vkDestroySampler(device->logical_device, sampler, Alloc);
}

void SngoEngine::Core::Source::Image::EngineSampler::creator(
    const Device::LogicalDevice::EngineDevice* _device,
    VkSamplerCreateInfo& _sampler_info,
    const VkAllocationCallbacks* alloc)
{
  destroyer();
  Alloc = alloc;
  device = _device;

  VkSamplerCreateInfo sampler_info{_sampler_info};

  if (vkCreateSampler(device->logical_device, &sampler_info, Alloc, &sampler) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create texture sampler!");
    }
}
