#include "Image.hpp"

#include <vulkan/vulkan_core.h>
#include <winnt.h>

#include <cstdint>

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "src/Core/Data.h"
#include "src/Core/Source/Buffer/Buffer.hpp"
#include "src/Core/Source/Buffer/CommandBuffer.hpp"
#include "stb_image.h"

uint32_t SngoEngine::Core::Source::Image::Find_MemoryType(const VkPhysicalDevice& physical_device,
                                                          uint32_t type_filter,
                                                          VkMemoryPropertyFlags properties)
{
  VkPhysicalDeviceMemoryProperties mem_properties{};
  vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);

  for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++)
    {
      if (type_filter & (1 << i)
          && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
        {
          return i;
        }
    }

  throw std::runtime_error("failed to find suitable memory type!");
}

VkFormat SngoEngine::Core::Source::Image::Find_Format(VkPhysicalDevice physical_device,
                                                      const std::vector<VkFormat>& candidates,
                                                      VkImageTiling tiling,
                                                      VkFormatFeatureFlags features)
{
  for (auto format : candidates)
    {
      VkFormatProperties props;
      vkGetPhysicalDeviceFormatProperties(physical_device, format, &props);

      if ((tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
          || (tiling == VK_IMAGE_TILING_OPTIMAL
              && (props.optimalTilingFeatures & features) == features))
        {
          return format;
        }
    }

  throw std::runtime_error("failed to find supported format!");
}

VkFormat SngoEngine::Core::Source::Image::Find_DepthFormat(VkPhysicalDevice physical_device)
{
  return Find_Format(
      physical_device,
      {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
      VK_IMAGE_TILING_OPTIMAL,
      VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

SngoEngine::Core::Data::ImageCreate_Info
SngoEngine::Core::Source::Image::GenerateImgCreateInfo_Texture2D(VkExtent3D _extent)
{
  return {VK_FORMAT_R8G8B8A8_SRGB,
          _extent,
          VK_IMAGE_TILING_OPTIMAL,
          VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT};
}

void SngoEngine::Core::Source::Image::CreateImage(
    const Device::LogicalDevice::EngineDevice* engine_device,
    const Data::ImageCreate_Info& _info,
    VkMemoryPropertyFlags properties,
    VkImage& image,
    VkDeviceMemory& image_memory)
{
  VkImageCreateInfo img_info{static_cast<VkImageCreateInfo>(_info)};
  auto& device{engine_device->logical_device};
  if (vkCreateImage(device, &img_info, nullptr, &image) != VK_SUCCESS)
    {
      throw std::runtime_error("fail to create image!");
    }

  VkMemoryRequirements req{};
  vkGetImageMemoryRequirements(device, image, &req);
  VkMemoryAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = req.size;
  alloc_info.memoryTypeIndex =
      Find_MemoryType(engine_device->pPD->physical_device, req.memoryTypeBits, properties);

  if (vkAllocateMemory(device, &alloc_info, nullptr, &image_memory) != VK_SUCCESS)
    {
      throw std::runtime_error("fail to allocate memory for image!");
    }
  vkBindImageMemory(device, image, image_memory, 0);
}

void SngoEngine::Core::Source::Image::Copy_Buffer2Image(
    const Device::LogicalDevice::EngineDevice* device,
    VkCommandPool _command_pool,
    VkBuffer buffer,
    VkImage img,
    VkExtent2D _extent)
{
  Core::Source::Buffer::EngineOnceCommandBuffer once_commandbuffer{
      device, _command_pool, device->graphics_queue};

  VkBufferImageCopy region{};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;

  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.layerCount = 1;

  region.imageOffset = {0, 0, 0};
  region.imageExtent = {_extent.width, _extent.height, 1};

  vkCmdCopyBufferToImage(once_commandbuffer.command_buffer,
                         buffer,
                         img,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         1,
                         &region);

  once_commandbuffer.end_buffer();
}

void SngoEngine::Core::Source::Image::Transition_ImageLayout(
    const Device::LogicalDevice::EngineDevice* device,
    VkCommandPool _command_pool,
    VkImage image,
    VkImageSubresourceRange subresourceRange,
    VkImageLayout old_layout,
    VkImageLayout new_layout,
    VkPipelineStageFlags sourceStage,
    VkPipelineStageFlags destinationStage)
{
  Core::Source::Buffer::EngineOnceCommandBuffer once_commandbuffer{
      device, _command_pool, device->graphics_queue};

  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = old_layout;
  barrier.newLayout = new_layout;

  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

  barrier.image = image;
  barrier.subresourceRange = subresourceRange;

  barrier.srcAccessMask = 0;
  barrier.dstAccessMask = 0;

  switch (old_layout)
    {
      case VK_IMAGE_LAYOUT_UNDEFINED:
        // Image layout is undefined (or does not matter)
        // Only valid as initial layout
        // No flags required, listed only for completeness
        barrier.srcAccessMask = 0;
        break;

      case VK_IMAGE_LAYOUT_PREINITIALIZED:
        // Image is preinitialized
        // Only valid as initial layout for linear images, preserves memory contents
        // Make sure host writes have been finished
        barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        break;

      case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        // Image is a color attachment
        // Make sure any writes to the color buffer have been finished
        barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;

      case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        // Image is a depth/stencil attachment
        // Make sure any writes to the depth/stencil buffer have been finished
        barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;

      case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        // Image is a transfer source
        // Make sure any reads from the image have been finished
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;

      case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        // Image is a transfer destination
        // Make sure any writes to the image have been finished
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;

      case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        // Image is read by a shader
        // Make sure any shader reads from the image have been finished
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;
      default:
        // Other source layouts aren't handled (yet)
        throw std::invalid_argument("unsupported layout transition!");

        break;
    }

  // Target layouts (new)
  // Destination access mask controls the dependency for the new image layout
  switch (new_layout)
    {
      case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        // Image will be used as a transfer destination
        // Make sure any writes to the image have been finished
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;

      case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        // Image will be used as a transfer source
        // Make sure any reads from the image have been finished
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;

      case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        // Image will be used as a color attachment
        // Make sure any writes to the color buffer have been finished
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;

      case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        // Image layout will be used as a depth/stencil attachment
        // Make sure any writes to depth/stencil buffer have been finished
        barrier.dstAccessMask =
            barrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;

      case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        // Image will be read in a shader (sampler, input attachment)
        // Make sure any writes to the image have been finished
        if (barrier.srcAccessMask == 0)
          {
            barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
          }
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;
      default:
        // Other source layouts aren't handled (yet)
        throw std::invalid_argument("unsupported layout transition!");

        break;
    }

  vkCmdPipelineBarrier(once_commandbuffer.command_buffer,
                       sourceStage,
                       destinationStage,
                       0,
                       0,
                       nullptr,
                       0,
                       nullptr,
                       1,
                       &barrier);

  once_commandbuffer.end_buffer();
}

SngoEngine::Core::Data::BufferCreate_Info
SngoEngine::Core::Source::Image::Generate_BufferMemoryAllocate_Info(
    const SngoEngine::Core::Device::LogicalDevice::EngineDevice* device,
    VkBuffer buffer,
    VkMemoryPropertyFlags properties)
{
  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(device->logical_device, buffer, &memRequirements);

  VkPhysicalDeviceMemoryProperties mem_properties;
  vkGetPhysicalDeviceMemoryProperties(device->pPD->physical_device, &mem_properties);

  return {memRequirements.size,
          Image::Find_MemoryType(
              device->pPD->physical_device, memRequirements.memoryTypeBits, properties)};
}

// SngoEngine::Core::Source::Image::EngineImage::EngineImage(
//     const Device::LogicalDevice::EngineDevice* _device,
//     Data::ImageCreate_Info _image_info,
//     VkMemoryPropertyFlags _properties,
//     const VkAllocationCallbacks* alloc)
//     : SngoEngine::Core::Source::Image::EngineImage()
// {
//   creator(_device, _image_info, _properties, alloc);
// }

// void SngoEngine::Core::Source::Image::EngineImage::operator()(
//     const Device::LogicalDevice::EngineDevice* _device,
//     Data::ImageCreate_Info _image_info,
//     VkMemoryPropertyFlags _properties,
//     const VkAllocationCallbacks* alloc)
// {
//   creator(_device, _image_info, _properties, alloc);
// }

void SngoEngine::Core::Source::Image::EngineImage::creator(
    const Device::LogicalDevice::EngineDevice* _device,
    Data::ImageCreate_Info _image_info,
    VkMemoryPropertyFlags _properties,
    const VkAllocationCallbacks* alloc)
{
  destroyer();
  Alloc = alloc;
  device = _device;
  extent = _image_info.extent;

  VkImageCreateInfo img_info{_image_info};
  if (vkCreateImage(device->logical_device, &img_info, Alloc, &image) != VK_SUCCESS)
    {
      throw std::runtime_error("fail to create image!");
    }

  VkMemoryRequirements req{};
  vkGetImageMemoryRequirements(device->logical_device, image, &req);

  VkMemoryAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = req.size;
  alloc_info.memoryTypeIndex =
      Find_MemoryType(_device->pPD->physical_device, req.memoryTypeBits, _properties);

  if (vkAllocateMemory(device->logical_device, &alloc_info, Alloc, &image_memory) != VK_SUCCESS)
    {
      throw std::runtime_error("fail to allocate memory for image!");
    }

  vkBindImageMemory(device->logical_device, image, image_memory, 0);
}

void SngoEngine::Core::Source::Image::EngineImage::destroyer()
{
  if (image != VK_NULL_HANDLE)
    {
      vkDestroyImage(device->logical_device, image, Alloc);
      vkFreeMemory(device->logical_device, image_memory, Alloc);
    }
}

void SngoEngine::Core::Source::Image::EngineTextureImage::creator(
    const Device::LogicalDevice::EngineDevice* _device,
    const Buffer::EngineCommandPool* _pool,
    const std::string& texture_file,
    const VkAllocationCallbacks* alloc)
{
  creator(_device, _pool->command_pool, texture_file, alloc);
}

void SngoEngine::Core::Source::Image::EngineTextureImage::creator(
    const Device::LogicalDevice::EngineDevice* _device,
    VkCommandPool _pool,
    const std::string& texture_file,
    const VkAllocationCallbacks* alloc)
{
  int tex_width{}, tex_height{}, tex_channel{};
  stbi_uc* pixels{
      stbi_load(texture_file.c_str(), &tex_width, &tex_height, &tex_channel, STBI_rgb_alpha)};

  extent = {static_cast<uint32_t>(tex_width), static_cast<uint32_t>(tex_height)};

  if (!pixels)
    {
      throw std::runtime_error("failed to load texture img!");
    }

  creator(_device,
          _pool,
          EnginePixelData{
              pixels,
              static_cast<uint32_t>(tex_width),
              static_cast<uint32_t>(tex_height),
              static_cast<uint32_t>(tex_channel),
          },
          alloc);

  stbi_image_free(pixels);
}

void SngoEngine::Core::Source::Image::EngineTextureImage::creator(
    const Device::LogicalDevice::EngineDevice* _device,
    VkCommandPool _pool,
    EnginePixelData pixel_data,
    const VkAllocationCallbacks* alloc)
{
  assert(pixel_data.is_available());
  destroyer();
  Alloc = alloc;
  device = _device;
  extent = {pixel_data.width, pixel_data.height};

  auto subresourceRange{Data::DEFAULT_COLOR_IMAGE_SUBRESOURCE_INFO};

  VkDeviceSize img_size{static_cast<VkDeviceSize>(pixel_data.width * pixel_data.height * 4)};
  Buffer::EngineBuffer staging_buffer{
      device,
      Data::BufferCreate_Info(img_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT),
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      Alloc};

  void* data;
  vkMapMemory(device->logical_device, staging_buffer.buffer_memory, 0, img_size, 0, &data);
  memcpy(data, pixel_data.data, static_cast<size_t>(img_size));
  vkUnmapMemory(device->logical_device, staging_buffer.buffer_memory);

  EngineImage img{
      device,
      Data::ImageCreate_Info{
          VK_FORMAT_R8G8B8A8_SRGB,
          {static_cast<uint32_t>(pixel_data.width), static_cast<uint32_t>(pixel_data.height), 1},
          VK_IMAGE_TILING_OPTIMAL,
          VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT

      },
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};

  Transition_ImageLayout(device,
                         _pool,
                         img.image,
                         subresourceRange,
                         VK_IMAGE_LAYOUT_UNDEFINED,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  Copy_Buffer2Image(device, _pool, staging_buffer.buffer, img.image, {extent});

  Transition_ImageLayout(device,
                         _pool,
                         img.image,
                         subresourceRange,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  image = img.image;
  image_memory = img.image_memory;

  Data::ImageViewCreate_Info _info{image, VK_FORMAT_R8G8B8A8_SRGB, subresourceRange};

  if (vkCreateImageView(device->logical_device, &_info, Alloc, &view) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create texture image view!");
    }

  img.image = VK_NULL_HANDLE;
  img.image_memory = VK_NULL_HANDLE;
  staging_buffer.destroyer();
}

void SngoEngine::Core::Source::Image::EngineTextureImage::destroyer()
{
  if (image != VK_NULL_HANDLE)
    {
      vkDestroyImage(device->logical_device, image, Alloc);
      vkFreeMemory(device->logical_device, image_memory, Alloc);
    }
}
