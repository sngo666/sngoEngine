#ifndef __SNGO_BUFFER_H
#define __SNGO_BUFFER_H

#include <vulkan/vulkan_core.h>

#include "src/Core/Device/LogicalDevice.hpp"

namespace SngoEngine::Core::Source::Buffer
{

void copy_buffer(const SngoEngine::Core::Device::LogicalDevice::EngineDevice* device,
                 VkCommandPool pool,
                 VkQueue graphics_queue,
                 VkBuffer src_buffer,
                 VkBuffer dst_buffer,
                 VkDeviceSize size);
Data::BufferCreate_Info Generate_BufferCreate_Info(VkDeviceSize size, VkBufferUsageFlags usage);
SngoEngine::Core::Data::BufferCreate_Info Generate_BufferMemoryAllocate_Info(
    const SngoEngine::Core::Device::LogicalDevice::EngineDevice* device,
    VkBuffer buffer,
    VkMemoryPropertyFlags properties);

struct EngineBuffer
{
  EngineBuffer() = default;
  EngineBuffer(EngineBuffer&&) noexcept = default;
  EngineBuffer& operator=(EngineBuffer&&) noexcept = default;
  template <class... Args>
  explicit EngineBuffer(const Device::LogicalDevice::EngineDevice* _device, Args... args)
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
  ~EngineBuffer()
  {
    destroyer();
  }
  void destroyer();

  VkBuffer buffer{};
  VkDeviceMemory buffer_memory{};
  const Device::LogicalDevice::EngineDevice* device{};

 private:
  void creator(const Device::LogicalDevice::EngineDevice* _device,
               Data::BufferCreate_Info _buffer_info,
               VkMemoryPropertyFlags properties,
               const VkAllocationCallbacks* alloc = nullptr);
  const VkAllocationCallbacks* Alloc{};
};

}  // namespace SngoEngine::Core::Source::Buffer
#endif