#ifndef __SNGO_IMAGE_H
#define __SNGO_IMAGE_H

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <stdexcept>
#include <string>

#include "src/Core/Data.h"
#include "src/Core/Device/LogicalDevice.hpp"
#include "src/Core/Source/Buffer/CommandBuffer.hpp"

namespace SngoEngine::Core::Source::Image
{

uint32_t Find_MemoryType(const VkPhysicalDevice& physical_device,
                         uint32_t type_filter,
                         VkMemoryPropertyFlags properties);

VkFormat Find_Format(VkPhysicalDevice physical_device,
                     const std::vector<VkFormat>& candidates,
                     VkImageTiling tiling,
                     VkFormatFeatureFlags features);

VkFormat Find_DepthFormat(VkPhysicalDevice physical_device);

Data::ImageCreate_Info GenerateImgCreateInfo_Texture2D(VkExtent3D _extent);

void CreateImage(const Device::LogicalDevice::EngineDevice* engine_device,
                 const Data::ImageCreate_Info& _info,
                 VkMemoryPropertyFlags properties,
                 VkImage& image,
                 VkDeviceMemory& image_memory);

void Copy_Buffer2Image(const Device::LogicalDevice::EngineDevice* device,
                       VkCommandPool _command_pool,
                       VkBuffer buffer,
                       VkImage img,
                       VkExtent2D _extent);

void Transition_ImageLayout(
    const Device::LogicalDevice::EngineDevice* device,
    VkCommandPool _command_pool,
    VkImage image,
    VkImageSubresourceRange subresourceRange,
    VkImageLayout old_layout,
    VkImageLayout new_layout,
    VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

Data::BufferCreate_Info Generate_BufferMemoryAllocate_Info(
    const Device::LogicalDevice::EngineDevice* device,
    VkBuffer buffer,
    VkMemoryPropertyFlags properties);
Data::ImageCreate_Info Generate_ImageCreate_Info();

//===========================================================================================================================
// EnginePixelData
//===========================================================================================================================

struct EnginePixelData
{
  unsigned char* data;
  unsigned int width;
  unsigned int height;
  unsigned int channel;

  [[nodiscard]] bool is_available() const
  {
    return (data);
  }
};

//===========================================================================================================================
// EngineImage
//===========================================================================================================================

struct EngineImage
{
  EngineImage() = default;
  EngineImage(EngineImage&&) noexcept = default;
  EngineImage& operator=(EngineImage&&) noexcept = default;
  template <class... Args>
  explicit EngineImage(const Device::LogicalDevice::EngineDevice* _device, Args... args)
  {
    creator(_device, args...);
  }
  template <class... Args>
  void operator()(const Device::LogicalDevice::EngineDevice* _device, Args... args)
  {
    creator(_device, args...);
  }
  template <typename U>
  U& operator=(U&) = delete;
  ~EngineImage()
  {
    destroyer();
  }
  void destroyer();

  VkExtent3D extent{};
  VkImage image{};
  VkDeviceMemory image_memory{};
  const Device::LogicalDevice::EngineDevice* device{};

 private:
  void creator(const Device::LogicalDevice::EngineDevice* _device,
               Data::ImageCreate_Info _image_info,
               VkMemoryPropertyFlags _properties,
               const VkAllocationCallbacks* alloc = nullptr);
  const VkAllocationCallbacks* Alloc{};
};

//===========================================================================================================================
// EngineTextureImage
//===========================================================================================================================

struct EngineTextureImage
{
  EngineTextureImage() = default;
  EngineTextureImage(EngineTextureImage&&) noexcept = default;
  EngineTextureImage& operator=(EngineTextureImage&&) noexcept = default;
  template <class... Args>
  explicit EngineTextureImage(const Device::LogicalDevice::EngineDevice* _device, Args... args)
  {
    creator(_device, args...);
  }
  template <class... Args>
  void operator()(const Device::LogicalDevice::EngineDevice* _device, Args... args)
  {
    creator(_device, args...);
  }
  template <typename U>
  U& operator=(U&) = delete;
  ~EngineTextureImage()
  {
    destroyer();
  }
  void destroyer();

  VkExtent2D extent{};
  VkImage image{};
  VkImageView view{};
  VkDeviceMemory image_memory{};
  const Device::LogicalDevice::EngineDevice* device{};

 private:
  void creator(const Device::LogicalDevice::EngineDevice* _device,
               const Buffer::EngineCommandPool* _pool,
               const std::string& texture_file,
               const VkAllocationCallbacks* alloc = nullptr);
  void creator(const Device::LogicalDevice::EngineDevice* _device,
               VkCommandPool _pool,
               const std::string& texture_file,
               const VkAllocationCallbacks* alloc = nullptr);
  void creator(const Device::LogicalDevice::EngineDevice* _device,
               VkCommandPool _pool,
               EnginePixelData pixel_data,
               const VkAllocationCallbacks* alloc = nullptr);
  const VkAllocationCallbacks* Alloc{};
};
};  // namespace SngoEngine::Core::Source::Image

#endif