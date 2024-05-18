#ifndef __SNGO_VERTEX_BUFFER_H
#define __SNGO_VERTEX_BUFFER_H

#include <array>
#include <concepts>
#include <type_traits>
#include <vector>

#include "fmt/core.h"
#include "src/Core/Device/LogicalDevice.hpp"
#include "src/Core/Source/Buffer/Buffer.hpp"
#include "src/Core/Source/Buffer/CommandBuffer.hpp"

#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <vulkan/vulkan_core.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

template <typename T, int N>
concept VertexStructure = requires(T t1, T t2) {
  {
    t1.getBindingDescription()
  } -> std::same_as<VkVertexInputBindingDescription>;
  {
    t1.getAttributeDescriptions()
  } -> std::same_as<std::vector<VkVertexInputAttributeDescription>>;
  {
    t1.operator==(t2)
  };
};

namespace SngoEngine::Core::Source::Buffer
{

//===========================================================================================================================
// Vertex Struct
//===========================================================================================================================

struct DEFAULT_EngineModelVertexData
{
  glm::vec3 pos;
  glm::vec3 color;
  glm::vec2 tex_coord;

  static const int attribute_count = 3;

  static VkVertexInputBindingDescription getBindingDescription()
  {
    VkVertexInputBindingDescription binding_description{};
    binding_description.binding = 0;
    binding_description.stride = sizeof(DEFAULT_EngineModelVertexData);
    binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return binding_description;
  }

  static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions()
  {
    std::vector<VkVertexInputAttributeDescription> attribute_descriptions{3};

    attribute_descriptions[0].binding = 0;
    attribute_descriptions[0].location = 0;
    attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attribute_descriptions[0].offset = offsetof(DEFAULT_EngineModelVertexData, pos);

    attribute_descriptions[1].binding = 0;
    attribute_descriptions[1].location = 1;
    attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attribute_descriptions[1].offset = offsetof(DEFAULT_EngineModelVertexData, color);

    attribute_descriptions[2].binding = 0;
    attribute_descriptions[2].location = 2;
    attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attribute_descriptions[2].offset = offsetof(DEFAULT_EngineModelVertexData, tex_coord);

    return attribute_descriptions;
  }

  bool operator==(const DEFAULT_EngineModelVertexData& data) const
  {
    return (pos == data.pos && color == data.color && tex_coord == data.tex_coord);
  }
};

template <typename T, int N>
  requires(VertexStructure<T, N>)
struct EngineVertexBuffer
{
  EngineVertexBuffer() = default;
  EngineVertexBuffer(EngineVertexBuffer&&) noexcept = default;
  EngineVertexBuffer& operator=(EngineVertexBuffer&&) noexcept = default;
  template <class... Args>
  explicit EngineVertexBuffer(const Device::LogicalDevice::EngineDevice* _device, Args... args)
  {
    creator(_device, args...);
  }
  template <class... Args>
  void operator()(const Device::LogicalDevice::EngineDevice* _device, Args... args)
  {
    creator(_device, args...);
  }
  ~EngineVertexBuffer()
  {
    destroyer();
  }
  void destroyer()
  {
    if (buffer != VK_NULL_HANDLE)
      {
        vkDestroyBuffer(device->logical_device, buffer, Alloc);
        vkFreeMemory(device->logical_device, buffer_memory, Alloc);
        buffer = VK_NULL_HANDLE;
        buffer_memory = VK_NULL_HANDLE;
      }
  }

  VkBuffer buffer{};
  VkDeviceMemory buffer_memory{};
  const Device::LogicalDevice::EngineDevice* device{};

 private:
  void creator(const Device::LogicalDevice::EngineDevice* _device,
               VkCommandPool vk_pool,
               VkQueue _graphic_queue,
               std::vector<T> _vertex_data,
               const VkAllocationCallbacks* alloc = nullptr)
  {
    destroyer();
    device = _device;
    Alloc = alloc;

    VkDeviceSize bufferSize = sizeof(T) * _vertex_data.size();
    EngineBuffer staging_buffer(
        device,
        Data::BufferCreate_Info{
            bufferSize,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        },
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        Alloc);

    void* data;
    vkMapMemory(device->logical_device, staging_buffer.buffer_memory, 0, bufferSize, 0, &data);
    memcpy(data, _vertex_data.data(), (size_t)bufferSize);
    vkUnmapMemory(device->logical_device, staging_buffer.buffer_memory);

    EngineBuffer vertex_buffer(
        device,
        Data::BufferCreate_Info{
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        },
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        Alloc);

    copy_buffer(
        device, vk_pool, _graphic_queue, staging_buffer.buffer, vertex_buffer.buffer, bufferSize);

    buffer = vertex_buffer.buffer;
    buffer_memory = vertex_buffer.buffer_memory;

    vertex_buffer.buffer = VK_NULL_HANDLE;
    vertex_buffer.buffer_memory = VK_NULL_HANDLE;
    staging_buffer.destroyer();
  }
  void creator(const Device::LogicalDevice::EngineDevice* _device,
               const EngineCommandPool* _pool,
               VkQueue _graphic_queue,
               std::vector<T> _vertex_data,
               const VkAllocationCallbacks* alloc = nullptr)
  {
    creator(_device, _pool->command_pool, _graphic_queue, _vertex_data, alloc);
  }

  const VkAllocationCallbacks* Alloc{};
};

}  // namespace SngoEngine::Core::Source::Buffer

namespace std
{
template <>
struct hash<SngoEngine::Core::Source::Buffer::DEFAULT_EngineModelVertexData>
{
  size_t operator()(
      SngoEngine::Core::Source::Buffer::DEFAULT_EngineModelVertexData const& vertex) const
  {
    return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1)
           ^ (hash<glm::vec2>()(vertex.tex_coord) << 1);
  }
};

}  // namespace std

#endif