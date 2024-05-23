#ifndef __SNGO_UNIFORM_BUFFER_H
#define __SNGO_UNIFORM_BUFFER_H
#include <vulkan/vulkan_core.h>

#include <array>
#include <concepts>
#include <glm/ext/matrix_float4x4.hpp>
#include <type_traits>
#include <vector>

#include "src/Core/Device/LogicalDevice.hpp"
#include "src/Core/Source/Buffer/Buffer.hpp"
#include "src/Core/Source/Buffer/CommandBuffer.hpp"

namespace SngoEngine::Core::Source::Buffer
{

struct UniformBuffer_Trans
{
  glm::mat4 projection{};
  glm::mat4 modelView{};
  glm::mat4 inverseModelview{};
  float lodBias = 0.0f;
};

//===========================================================================================================================
// EngineUniformBuffer
//===========================================================================================================================

template <typename T>
struct EngineUniformBuffer
{
  EngineUniformBuffer() = default;
  EngineUniformBuffer(EngineUniformBuffer&&) noexcept = default;
  EngineUniformBuffer& operator=(EngineUniformBuffer&&) noexcept = default;
  template <typename... Args>
  explicit EngineUniformBuffer(const Device::LogicalDevice::EngineDevice* _device, Args... args)
  {
    creator(_device, args...);
  }
  template <typename... Args>
  void init(const Device::LogicalDevice::EngineDevice* _device, Args... args)
  {
    creator(_device, args...);
  }
  ~EngineUniformBuffer()
  {
    destroyer();
  }
  void destroyer()
  {
    if (device)
      {
        vkDestroyBuffer(device->logical_device, buffer, Alloc);
        vkFreeMemory(device->logical_device, buffer_memory, Alloc);
        buffer = VK_NULL_HANDLE;
        buffer_memory = VK_NULL_HANDLE;
      }
  }

  VkBuffer buffer{};
  VkDeviceMemory buffer_memory{};
  void* mapped{};
  VkDescriptorBufferInfo descriptor{};
  const Device::LogicalDevice::EngineDevice* device{};

 private:
  void creator(const Device::LogicalDevice::EngineDevice* _device,
               VkQueue _graphic_queue,
               const VkAllocationCallbacks* alloc = nullptr)
  {
    destroyer();
    Alloc = alloc;
    device = _device;

    VkDeviceSize buffer_size{sizeof(T)};
    EngineBuffer temp_buffer(
        device,
        Data::BufferCreate_Info{buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT},
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        Alloc);

    buffer = temp_buffer.buffer;
    buffer_memory = temp_buffer.buffer_memory;

    descriptor.offset = 0;
    descriptor.buffer = buffer;
    descriptor.range = buffer_size;

    vkMapMemory(device->logical_device, buffer_memory, 0, buffer_size, 0, &mapped);
    temp_buffer.buffer = VK_NULL_HANDLE;
    temp_buffer.buffer_memory = VK_NULL_HANDLE;
  }
  const VkAllocationCallbacks* Alloc{};
};

using TransUniBuffer = EngineUniformBuffer<UniformBuffer_Trans>;
}  // namespace SngoEngine::Core::Source::Buffer
#endif
