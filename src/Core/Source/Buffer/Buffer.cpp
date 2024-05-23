#include "Buffer.hpp"

#include <vulkan/vulkan_core.h>

#include "src/Core/Data.h"
#include "src/Core/Device/LogicalDevice.hpp"
#include "src/Core/Source/Buffer/CommandBuffer.hpp"
#include "src/Core/Source/Image/Image.hpp"

void SngoEngine::Core::Source::Buffer::copy_buffer(
    const Device::LogicalDevice::EngineDevice* device,
    VkCommandPool pool,
    VkQueue graphics_queue,
    VkBuffer src_buffer,
    VkBuffer dst_buffer,
    VkDeviceSize size)
{
  EngineOnceCommandBuffer command_buffer(device, pool, graphics_queue);

  VkBufferCopy copy_region{};
  copy_region.srcOffset = 0;
  copy_region.dstOffset = 0;
  copy_region.size = size;
  vkCmdCopyBuffer(command_buffer.command_buffer, src_buffer, dst_buffer, 1, &copy_region);

  command_buffer.end_buffer();
}

SngoEngine::Core::Data::BufferCreate_Info
SngoEngine::Core::Source::Buffer::Generate_BufferCreate_Info(VkDeviceSize size,
                                                             VkBufferUsageFlags usage)
{
  return {size, usage};
}

SngoEngine::Core::Data::BufferCreate_Info
SngoEngine::Core::Source::Buffer::Generate_BufferMemoryAllocate_Info(
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
// EngineBuffer
//===========================================================================================================================

void SngoEngine::Core::Source::Buffer::EngineBuffer::creator(
    const Device::LogicalDevice::EngineDevice* _device,
    Data::BufferCreate_Info _buffer_info,
    VkMemoryPropertyFlags properties,
    const VkAllocationCallbacks* alloc)
{
  destroyer();
  Alloc = alloc;
  device = _device;

  VkBufferCreateInfo buffer_info{_buffer_info};

  if (vkCreateBuffer(device->logical_device, &buffer_info, Alloc, &buffer))
    {
      throw std::runtime_error("failed to create buffer!");
    }

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(device->logical_device, buffer, &memRequirements);

  VkPhysicalDeviceMemoryProperties mem_properties;
  vkGetPhysicalDeviceMemoryProperties(device->pPD->physical_device, &mem_properties);

  VkMemoryAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = memRequirements.size;
  alloc_info.memoryTypeIndex = Image::Find_MemoryType(
      device->pPD->physical_device, memRequirements.memoryTypeBits, properties);

  if (vkAllocateMemory(device->logical_device, &alloc_info, Alloc, &buffer_memory) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to allocate vertex buffer memory");
    }
  vkBindBufferMemory(device->logical_device, buffer, buffer_memory, 0);
}

void SngoEngine::Core::Source::Buffer::EngineBuffer::destroyer()
{
  if (buffer != VK_NULL_HANDLE)
    {
      vkDestroyBuffer(device->logical_device, buffer, Alloc);
      vkFreeMemory(device->logical_device, buffer_memory, Alloc);
      buffer = VK_NULL_HANDLE;
      buffer_memory = VK_NULL_HANDLE;
    }
}
