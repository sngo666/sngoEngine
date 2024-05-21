#include "Image.hpp"

#include <vulkan/vulkan_core.h>
#include <winnt.h>

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>

#include "fmt/core.h"
#include "ktx.h"
#include "ktxvulkan.h"
#include "src/Core/Source/Image/Sampler.hpp"
#include "src/Core/Utils/Utils.hpp"

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

void SngoEngine::Core::Source::Image::Copy_Buffer2Image(
    const Device::LogicalDevice::EngineDevice* device,
    VkCommandPool _command_pool,
    VkBuffer buffer,
    VkImage img,
    const std::vector<VkBufferImageCopy>& regions)

{
  Core::Source::Buffer::EngineOnceCommandBuffer once_commandbuffer{
      device, _command_pool, device->graphics_queue};

  vkCmdCopyBufferToImage(once_commandbuffer.command_buffer,
                         buffer,
                         img,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         regions.size(),
                         regions.data());

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

//===========================================================================================================================
// EngineImage
//===========================================================================================================================

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
      image = VK_NULL_HANDLE;
    }
}

//===========================================================================================================================
// EngineTextureImage
//===========================================================================================================================

ktxResult SngoEngine::Core::Source::Image::loadKTXFile(const std::string& filename,
                                                       ktxTexture** target)
{
  ktxResult result = KTX_SUCCESS;
  if (!SngoEngine::Core::Utils::isFile_Exists(filename))
    {
      throw std::runtime_error(
          "Could not load texture from " + filename
          + "\n\nMake sure the assets submodule has been checked out and is up-to-date.");
    }
  result = ktxTexture_CreateFromNamedFile(
      filename.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, target);
  return result;
}

void SngoEngine::Core::Source::Image::EngineTextureImage::creator(
    const Device::LogicalDevice::EngineDevice* _device,
    VkCommandPool _pool,
    const std::string& texture_file,
    const VkAllocationCallbacks* alloc,
    bool using_stage,
    VkFormat _format,
    VkImageUsageFlags _usage,
    VkImageLayout _layout)
{
  destroyer();
  Alloc = alloc;
  device = _device;

  auto ext_name{Utils::GetFile_Extension(texture_file)};
  if (ext_name.empty())
    {
      throw std::runtime_error("wrong extension name in file " + texture_file);
    }
  if (ext_name == "jpg" || ext_name == "png")
    {
      int tex_width{}, tex_height{}, tex_channel{};
      stbi_uc* pixels{
          stbi_load(texture_file.c_str(), &tex_width, &tex_height, &tex_channel, STBI_rgb_alpha)};

      extent = {static_cast<uint32_t>(tex_width), static_cast<uint32_t>(tex_height)};
      mip_levels = 1;

      if (!pixels)
        {
          throw std::runtime_error("failed to load texture img!");
        }
      if (using_stage)
        {
          CreateWith_Staging(_device,
                             _pool,
                             EnginePixelData{
                                 pixels,
                                 static_cast<uint32_t>(tex_width),
                                 static_cast<uint32_t>(tex_height),
                                 static_cast<uint64_t>(tex_width * tex_height * 4),
                             },
                             _format,
                             _usage,
                             _layout,
                             {},
                             Alloc);
        }
      else
        {
          CreateWithout_Staging(_device,
                                _pool,
                                EnginePixelData{
                                    pixels,
                                    static_cast<uint32_t>(tex_width),
                                    static_cast<uint32_t>(tex_height),
                                    static_cast<uint64_t>(tex_width * tex_height * 4),
                                },
                                _format,
                                _usage,
                                _layout,
                                Alloc);
        }
      stbi_image_free(pixels);
    }
  else if (ext_name == "ktx")
    {
      ktxTexture* ktxTexture;
      ktxResult result = loadKTXFile(texture_file, &ktxTexture);
      assert(result == KTX_SUCCESS);

      extent = {ktxTexture->baseWidth, ktxTexture->baseHeight};
      mip_levels = ktxTexture->numLevels;

      ktx_uint8_t* ktxTexture_Data = ktxTexture_GetData(ktxTexture);
      ktx_size_t ktxTextureSize = ktxTexture_GetSize(ktxTexture);

      std::vector<VkBufferImageCopy> bufferCopyRegions;

      for (uint32_t i = 0; i < mip_levels; i++)
        {
          ktx_size_t offset;
          KTX_error_code result = ktxTexture_GetImageOffset(ktxTexture, i, 0, 0, &offset);
          assert(result == KTX_SUCCESS);

          VkBufferImageCopy bufferCopyRegion = {};
          bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
          bufferCopyRegion.imageSubresource.mipLevel = i;
          bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
          bufferCopyRegion.imageSubresource.layerCount = 1;
          bufferCopyRegion.imageExtent.width = std::max(1u, ktxTexture->baseWidth >> i);
          bufferCopyRegion.imageExtent.height = std::max(1u, ktxTexture->baseHeight >> i);
          bufferCopyRegion.imageExtent.depth = 1;
          bufferCopyRegion.bufferOffset = offset;

          bufferCopyRegions.push_back(bufferCopyRegion);
        }

      if (using_stage)
        {
          CreateWith_Staging(_device,
                             _pool,
                             EnginePixelData{
                                 ktxTexture_Data,
                                 extent.width,
                                 extent.height,
                                 ktxTextureSize,
                             },
                             _format,
                             _usage,
                             _layout,
                             bufferCopyRegions,
                             Alloc);
        }
      else
        {
          CreateWithout_Staging(_device,
                                _pool,
                                EnginePixelData{
                                    ktxTexture_Data,
                                    extent.width,
                                    extent.height,
                                    ktxTextureSize,
                                },
                                _format,
                                _usage,
                                _layout,
                                Alloc);
        }

      ktxTexture_Destroy(ktxTexture);
    }

  descriptor.sampler = sampler.sampler;
  descriptor.imageView = view.image_view;
  descriptor.imageLayout = _layout;
}

void SngoEngine::Core::Source::Image::EngineTextureImage::creator(
    const Device::LogicalDevice::EngineDevice* _device,
    VkCommandPool _pool,
    EnginePixelData pixel_data,
    const VkAllocationCallbacks* alloc,
    bool using_stage,
    VkFormat _format,
    VkImageUsageFlags _usage,
    VkImageLayout dst_layout)
{
  destroyer();
  Alloc = alloc;
  device = _device;
  extent = {pixel_data.width, pixel_data.height};
  mip_levels = 1;

  if (using_stage)
    {
      CreateWith_Staging(_device, _pool, pixel_data, _format, _usage, dst_layout, {}, Alloc);
    }
  else
    {
      CreateWithout_Staging(_device, _pool, pixel_data, _format, _usage, dst_layout, Alloc);
    }

  descriptor.sampler = sampler.sampler;
  descriptor.imageView = view.image_view;
  descriptor.imageLayout = dst_layout;
}

// creator for jpg/png
void SngoEngine::Core::Source::Image::EngineTextureImage::CreateWith_Staging(
    const Device::LogicalDevice::EngineDevice* _device,
    VkCommandPool _pool,
    EnginePixelData pixel_data,
    VkFormat _format,
    VkImageUsageFlags _usage,
    VkImageLayout dst_layout,
    const std::vector<VkBufferImageCopy>& regions,
    const VkAllocationCallbacks* alloc)
{
  assert(pixel_data.is_available());

  auto subresourceRange{Data::DEFAULT_COLOR_IMAGE_SUBRESOURCE_INFO};
  auto used_format{_format};

  Buffer::EngineBuffer staging_buffer{
      device,
      Data::BufferCreate_Info(pixel_data.size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT),
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      Alloc};

  void* data;
  vkMapMemory(device->logical_device, staging_buffer.buffer_memory, 0, pixel_data.size, 0, &data);
  memcpy(data, pixel_data.data, pixel_data.size);
  vkUnmapMemory(device->logical_device, staging_buffer.buffer_memory);

  EngineImage img{
      device,
      Data::ImageCreate_Info{
          used_format,
          {static_cast<uint32_t>(pixel_data.width), static_cast<uint32_t>(pixel_data.height), 1},
          VK_IMAGE_TILING_OPTIMAL,
          _usage,
          mip_levels},
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};

  Transition_ImageLayout(device,
                         _pool,
                         img.image,
                         subresourceRange,
                         VK_IMAGE_LAYOUT_UNDEFINED,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  if (regions.empty())
    {
      Copy_Buffer2Image(device, _pool, staging_buffer.buffer, img.image, extent);
    }
  else
    {
      Copy_Buffer2Image(device, _pool, staging_buffer.buffer, img.image, regions);
    }

  Transition_ImageLayout(
      device, _pool, img.image, subresourceRange, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dst_layout);

  image = img.image;
  image_memory = img.image_memory;

  // create image view

  Data::ImageViewCreate_Info _info{image, used_format, subresourceRange};
  _info.subresourceRange.levelCount = mip_levels;
  view(device, _info, Alloc);

  // create image sampler

  auto sampler_info{Get_Default_Sampler(device, static_cast<float>(mip_levels))};
  sampler(device, sampler_info, Alloc);

  img.image = VK_NULL_HANDLE;
  img.image_memory = VK_NULL_HANDLE;
  staging_buffer.destroyer();
}

void SngoEngine::Core::Source::Image::EngineTextureImage::CreateWithout_Staging(
    const Device::LogicalDevice::EngineDevice* _device,
    VkCommandPool _pool,
    EnginePixelData pixel_data,
    VkFormat _format,
    VkImageUsageFlags _usage,
    VkImageLayout dst_layout,
    const VkAllocationCallbacks* alloc)
{
  assert(pixel_data.is_available());
  destroyer();
  Alloc = alloc;
  device = _device;
  extent = {pixel_data.width, pixel_data.height};

  auto used_format{_format};
  mip_levels = 1;
  VkMemoryRequirements req{};
  vkGetImageMemoryRequirements(device->logical_device, image, &req);
  auto subresourceRange{Data::DEFAULT_COLOR_IMAGE_SUBRESOURCE_INFO};

  EngineImage img{
      device,
      Data::ImageCreate_Info{
          used_format,
          {static_cast<uint32_t>(pixel_data.width), static_cast<uint32_t>(pixel_data.height), 1},
          VK_IMAGE_TILING_LINEAR,
          _usage,
          mip_levels},
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};

  VkImageSubresource sub_resource{};
  sub_resource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  sub_resource.mipLevel = 0;

  VkSubresourceLayout subres_layout;
  void* data;

  vkGetImageSubresourceLayout(device->logical_device, img.image, &sub_resource, &subres_layout);
  Utils::Vk_Exception(vkMapMemory(device->logical_device, img.image_memory, 0, req.size, 0, &data));

  Transition_ImageLayout(
      device, _pool, img.image, subresourceRange, VK_IMAGE_LAYOUT_UNDEFINED, dst_layout);

  image = img.image;
  image_memory = img.image_memory;

  {
    Data::ImageViewCreate_Info _info{image, used_format, subresourceRange};
    _info.subresourceRange.levelCount = mip_levels;
    view(device, _info, Alloc);
  }

  // create image sampler
  {
    auto sampler_info{Get_Default_Sampler(device, static_cast<float>(mip_levels))};
    sampler(device, sampler_info, Alloc);
  }

  img.image = VK_NULL_HANDLE;
  img.image_memory = VK_NULL_HANDLE;
}

void SngoEngine::Core::Source::Image::EngineTextureImage::destroyer()
{
  if (image != VK_NULL_HANDLE)
    {
      vkDestroyImage(device->logical_device, image, Alloc);
      vkFreeMemory(device->logical_device, image_memory, Alloc);
    }
}

void SngoEngine::Core::Source::Image::Get_EmptyTextureImg(
    const Device::LogicalDevice::EngineDevice* _device,
    EngineTextureImage& img,
    VkCommandPool _pool,
    const VkAllocationCallbacks* alloc)
{
  uint32_t width{1};
  uint32_t height{1};

  size_t bufferSize = static_cast<size_t>(width * height) * 4;
  unsigned char* buffer = new unsigned char[bufferSize];
  memset(buffer, 0, bufferSize);

  img(_device, _pool, EnginePixelData{buffer, width, height, bufferSize});

  delete[] buffer;
}

//===========================================================================================================================
// EngineCubeTexture
//===========================================================================================================================

void SngoEngine::Core::Source::Image::EngineCubeTexture::creator(
    const Device::LogicalDevice::EngineDevice* _device,
    VkCommandPool _pool,
    const std::string& texture_file,
    const VkAllocationCallbacks* alloc,
    VkFormat _format,
    VkImageUsageFlags _usage,
    VkImageLayout _layout)
{
  destroyer();
  Alloc = alloc;
  device = _device;

  auto ext_name{Utils::GetFile_Extension(texture_file)};
  if (ext_name.empty())
    {
      throw std::runtime_error("wrong extension name in file " + texture_file);
    }
  if (ext_name == "jpg" || ext_name == "png")
    {
      int tex_width{}, tex_height{}, tex_channel{};
      stbi_uc* pixels{
          stbi_load(texture_file.c_str(), &tex_width, &tex_height, &tex_channel, STBI_rgb_alpha)};

      extent = {static_cast<uint32_t>(tex_width), static_cast<uint32_t>(tex_height)};
      mip_levels = 1;

      if (!pixels)
        {
          throw std::runtime_error("failed to load texture img!");
        }

      // TODO: add jpg-like support for cube texture
      throw std::runtime_error("Unfinished support for jpg-like textures!");

      stbi_image_free(pixels);
    }
  else if (ext_name == "ktx")
    {
      ktxTexture* ktxTexture;
      ktxResult result = loadKTXFile(texture_file, &ktxTexture);
      assert(result == KTX_SUCCESS);

      extent = {ktxTexture->baseWidth, ktxTexture->baseHeight};
      mip_levels = ktxTexture->numLevels;

      ktx_uint8_t* ktxTexture_Data = ktxTexture_GetData(ktxTexture);
      ktx_size_t ktxTextureSize = ktxTexture_GetSize(ktxTexture);

      std::vector<VkBufferImageCopy> bufferCopyRegions;
      for (uint32_t k = 0; k < 6; k++)
        {
          for (uint32_t i = 0; i < mip_levels; i++)
            {
              ktx_size_t offset;
              KTX_error_code result = ktxTexture_GetImageOffset(ktxTexture, i, 0, k, &offset);
              assert(result == KTX_SUCCESS);

              VkBufferImageCopy bufferCopyRegion = {};
              bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
              bufferCopyRegion.imageSubresource.mipLevel = i;
              bufferCopyRegion.imageSubresource.baseArrayLayer = k;
              bufferCopyRegion.imageSubresource.layerCount = 1;
              bufferCopyRegion.imageExtent.width = std::max(1u, ktxTexture->baseWidth >> i);
              bufferCopyRegion.imageExtent.height = std::max(1u, ktxTexture->baseHeight >> i);
              bufferCopyRegion.imageExtent.depth = 1;
              bufferCopyRegion.bufferOffset = offset;

              bufferCopyRegions.push_back(bufferCopyRegion);
            }
        }

      CreateCubeWith_Staging(_device,
                             _pool,
                             EnginePixelData{
                                 ktxTexture_Data,
                                 extent.width,
                                 extent.height,
                                 ktxTextureSize,
                             },
                             _format,
                             _usage,
                             _layout,
                             bufferCopyRegions,
                             Alloc);

      ktxTexture_Destroy(ktxTexture);
    }

  descriptor.sampler = sampler.sampler;
  descriptor.imageView = view.image_view;
  descriptor.imageLayout = _layout;
}

// creator for jpg/png
void SngoEngine::Core::Source::Image::EngineCubeTexture::CreateCubeWith_Staging(
    const Device::LogicalDevice::EngineDevice* _device,
    VkCommandPool _pool,
    EnginePixelData pixel_data,
    VkFormat _format,
    VkImageUsageFlags _usage,
    VkImageLayout dst_layout,
    const std::vector<VkBufferImageCopy>& regions,
    const VkAllocationCallbacks* alloc)
{
  assert(pixel_data.is_available());

  auto subresourceRange{Data::DEFAULT_COLOR_IMAGE_SUBRESOURCE_INFO};
  subresourceRange.levelCount = mip_levels;
  subresourceRange.layerCount = 6;
  auto used_format{_format};

  Buffer::EngineBuffer staging_buffer{
      device,
      Data::BufferCreate_Info(pixel_data.size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT),
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      Alloc};

  void* data;
  vkMapMemory(device->logical_device, staging_buffer.buffer_memory, 0, pixel_data.size, 0, &data);
  memcpy(data, pixel_data.data, pixel_data.size);
  vkUnmapMemory(device->logical_device, staging_buffer.buffer_memory);

  EngineImage img{
      device,
      Data::ImageCreate_Info{
          used_format,
          {static_cast<uint32_t>(pixel_data.width), static_cast<uint32_t>(pixel_data.height), 1},
          VK_IMAGE_TILING_OPTIMAL,
          _usage,
          mip_levels,
          VK_SAMPLE_COUNT_1_BIT,
          6,
          VK_IMAGE_TYPE_2D,
          VK_IMAGE_LAYOUT_UNDEFINED,
          VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT},
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};

  Transition_ImageLayout(device,
                         _pool,
                         img.image,
                         subresourceRange,
                         VK_IMAGE_LAYOUT_UNDEFINED,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  if (regions.empty())
    {
      // Copy_Buffer2Image(device, _pool, staging_buffer.buffer, img.image, extent);
      // TODO: add jpg-like support for cube texture
    }
  else
    {
      Copy_Buffer2Image(device, _pool, staging_buffer.buffer, img.image, regions);
    }

  Transition_ImageLayout(
      device, _pool, img.image, subresourceRange, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dst_layout);

  image = img.image;
  image_memory = img.image_memory;

  // create image view

  Data::ImageViewCreate_Info _info{image, used_format, subresourceRange};
  _info.subresourceRange.levelCount = mip_levels;
  _info.subresourceRange.layerCount = 6;
  _info.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
  view(device, _info, Alloc);

  // create image sampler

  auto sampler_info{Get_CubeTex_Sampler(device, static_cast<float>(mip_levels))};
  sampler(device, sampler_info, Alloc);

  img.image = VK_NULL_HANDLE;
  img.image_memory = VK_NULL_HANDLE;
  staging_buffer.destroyer();
}

void SngoEngine::Core::Source::Image::EngineCubeTexture::destroyer()
{
  if (image != VK_NULL_HANDLE)
    {
      vkDestroyImage(device->logical_device, image, Alloc);
      vkFreeMemory(device->logical_device, image_memory, Alloc);
    }
}