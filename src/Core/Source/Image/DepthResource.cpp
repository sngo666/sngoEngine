#include "DepthResource.hpp"

#include "src/Core/Source/Image/Image.hpp"
#include "src/Core/Source/Image/ImageVIew.hpp"

VkFormat SngoEngine::Core::Source::DepthResource::FInd_DepthFormat(VkPhysicalDevice physical_device)
{
  return Image::Find_Format(
      physical_device,
      {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
      VK_IMAGE_TILING_OPTIMAL,
      VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

void SngoEngine::Core::Source::DepthResource::EngineDepthResource::creator(
    const Device::LogicalDevice::EngineDevice* _device,
    VkExtent3D _extent,
    VkImageTiling _tiling,
    VkImageUsageFlags _usage,
    VkMemoryPropertyFlags _properties,
    const VkAllocationCallbacks* alloc)
{
  device = _device;
  Alloc = alloc;
  extent = _extent;

  VkFormat _format = Image::Find_DepthFormat(device->pPD->physical_device);
  engine_Image(
      device, Data::ImageCreate_Info(_format, _extent, _tiling, _usage), _properties, Alloc);
  engine_ImageView(
      device,
      Data::ImageViewCreate_Info{
          engine_Image.image, _format, Data::ImageSubresourceRange_Info{VK_IMAGE_ASPECT_DEPTH_BIT}},
      Alloc);
}