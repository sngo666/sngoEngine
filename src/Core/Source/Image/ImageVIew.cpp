#include "ImageVIew.hpp"

#include <io.h>
#include <vulkan/vulkan_core.h>

#include <cstddef>

void SngoEngine::Core::Source::ImageView::EngineImageView::creator(
    const Device::LogicalDevice::EngineDevice* _device,
    Data::ImageViewCreate_Info _info,
    const VkAllocationCallbacks* alloc)
{
  if (image_view != VK_NULL_HANDLE)
    vkDestroyImageView(device->logical_device, image_view, Alloc);
  device = _device;
  Alloc = alloc;

  if (vkCreateImageView(device->logical_device, &_info, Alloc, &image_view) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create texture image view!");
    }
}

void SngoEngine::Core::Source::ImageView::EngineImageViews::creator(
    const Device::LogicalDevice::EngineDevice* _device,
    const std::vector<VkImage>& images,
    Data::ImageViewCreate_Info _info,
    const VkAllocationCallbacks* alloc)
{
  for (auto& view : image_views)
    vkDestroyImageView(device->logical_device, view, Alloc);
  Alloc = alloc;
  device = _device;
  image_views.resize(images.size());

  for (int i = 0; i < size(); i++)
    {
      _info.image = images[i];
      VkImageView temp;
      if (vkCreateImageView(device->logical_device, &_info, Alloc, &temp) != VK_SUCCESS)
        {
          throw std::runtime_error("failed to create texture image view!");
        }
      image_views[i] = temp;
    }
}

void SngoEngine::Core::Source::ImageView::EngineImageViews::destroyer()
{
  for (auto& view : image_views)
    vkDestroyImageView(device->logical_device, view, Alloc);
  image_views.clear();
}

VkImageView& SngoEngine::Core::Source::ImageView::EngineImageViews::operator[](size_t t)
{
  return image_views[t];
}

VkImageView SngoEngine::Core::Source::ImageView::EngineImageViews::operator[](size_t t) const
{
  return image_views[t];
}

VkImageView* SngoEngine::Core::Source::ImageView::EngineImageViews::data()
{
  return image_views.data();
}

size_t SngoEngine::Core::Source::ImageView::EngineImageViews::size() const
{
  return image_views.size();
}

size_t SngoEngine::Core::Source::ImageView::EngineImageViews::size()
{
  return image_views.size();
}

std::vector<VkImageView>::iterator SngoEngine::Core::Source::ImageView::EngineImageViews::begin()
{
  return image_views.begin();
}

std::vector<VkImageView>::iterator SngoEngine::Core::Source::ImageView::EngineImageViews::end()
{
  return image_views.end();
}
