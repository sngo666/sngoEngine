#ifndef __SNGO_INDEX_BUFFER_H
#define __SNGO_INDEX_BUFFER_H

#include <vulkan/vulkan_core.h>

#include <array>
#include <concepts>
#include <type_traits>
#include <vector>

#include "src/Core/Device/LogicalDevice.hpp"
#include "src/Core/Source/Buffer/Buffer.hpp"
#include "src/Core/Source/Buffer/CommandBuffer.hpp"

namespace SngoEngine::Core::Source::Buffer
{

//===========================================================================================================================
// EngineIndexBuffer
//===========================================================================================================================

template <typename T>
struct EngineIndexBuffer
{
  EngineIndexBuffer() = default;
  EngineIndexBuffer(EngineIndexBuffer&&) noexcept = default;
  EngineIndexBuffer& operator=(EngineIndexBuffer&&) noexcept = default;
  template <class... Args>
  explicit EngineIndexBuffer(const Device::LogicalDevice::EngineDevice* _device, Args... args)
  {
    creator(_device, args...);
  }
  template <class... Args>
  void init(const Device::LogicalDevice::EngineDevice* _device, Args... args)
  {
    creator(_device, args...);
  }
  ~EngineIndexBuffer()
  {
    destroyer();
  }
  void destroyer()
  {
    if (device)
      {
        vkDestroyBuffer(device->logical_device, buffer, Alloc);
        vkFreeMemory(device->logical_device, buffer_memory, Alloc);
      }
  }

  VkBuffer buffer{};
  VkDeviceMemory buffer_memory{};
  const Device::LogicalDevice::EngineDevice* device{};

 private:
  void creator(const Device::LogicalDevice::EngineDevice* _device,
               VkCommandPool vk_pool,
               VkQueue _graphic_queue,
               std::vector<T> _index_data,
               const VkAllocationCallbacks* alloc = nullptr)
  {
    destroyer();
    device = _device;
    Alloc = alloc;

    VkDeviceSize bufferSize = sizeof(T) * _index_data.size();
    EngineBuffer staging_buffer(
        device,
        Data::BufferCreate_Info{
            bufferSize,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        },
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        Alloc);

    void* data;
    vkMapMemory(device->logical_device, staging_buffer.buffer_memory, 0, bufferSize, 0, &data);
    memcpy(data, _index_data.data(), (size_t)bufferSize);
    vkUnmapMemory(device->logical_device, staging_buffer.buffer_memory);

    EngineBuffer index_buffer(
        device,
        Data::BufferCreate_Info{
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        },
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        Alloc);

    copy_buffer(
        device, vk_pool, _graphic_queue, staging_buffer.buffer, index_buffer.buffer, bufferSize);

    buffer = index_buffer.buffer;
    buffer_memory = index_buffer.buffer_memory;

    index_buffer.buffer = VK_NULL_HANDLE;
    index_buffer.buffer_memory = VK_NULL_HANDLE;
    staging_buffer.destroyer();
  }

  void creator(const Device::LogicalDevice::EngineDevice* _device,
               const EngineCommandPool* _pool,
               VkQueue _graphic_queue,
               std::vector<T> _index_data,
               const VkAllocationCallbacks* alloc = nullptr)
  {
    creator(_device, _pool->command_pool, _graphic_queue, _index_data, alloc);
  }

  const VkAllocationCallbacks* Alloc{};
};
}  // namespace SngoEngine::Core::Source::Buffer

#endif