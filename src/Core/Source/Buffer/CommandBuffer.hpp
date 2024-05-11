#ifndef __SNGO_COMMANDBUFFER_H
#define __SNGO_COMMANDBUFFER_H

#include <vulkan/vulkan_core.h>

#include <cstddef>
#include <vector>

#include "src/Core/Data.h"
#include "src/Core/Device/LogicalDevice.hpp"
#include "src/Core/Device/PhysicalDevice.hpp"

namespace SngoEngine::Core::Source::Buffer
{

//===========================================================================================================================
// EngineCommandPool
//===========================================================================================================================

struct EngineCommandPool
{
  EngineCommandPool() = default;
  EngineCommandPool(EngineCommandPool&&) noexcept = default;
  EngineCommandPool& operator=(EngineCommandPool&&) noexcept = default;
  EngineCommandPool(const Device::LogicalDevice::EngineDevice* _device,
                    Data::CommandPoolCreate_Info _info,
                    const VkAllocationCallbacks* alloc = nullptr);
  void operator()(const Device::LogicalDevice::EngineDevice* _device,
                  Data::CommandPoolCreate_Info _info,
                  const VkAllocationCallbacks* alloc = nullptr);
  template <typename U>
  U& operator=(U&) = delete;
  ~EngineCommandPool()
  {
    destroyer();
  }
  void destroyer();

  VkCommandPool command_pool{};
  const Device::LogicalDevice::EngineDevice* device{};

 private:
  void creator(const Device::LogicalDevice::EngineDevice* _device,
               Data::CommandPoolCreate_Info _info,
               const VkAllocationCallbacks* alloc);
  const VkAllocationCallbacks* Alloc{};
};

//===========================================================================================================================
// EngineCommandPools
//===========================================================================================================================

struct EngineCommandPools
{
  EngineCommandPools() = default;
  EngineCommandPools(EngineCommandPools&&) noexcept = default;
  EngineCommandPools& operator=(EngineCommandPools&&) noexcept = default;
  template <typename... Args>
  explicit EngineCommandPools(const Device::LogicalDevice::EngineDevice* _device, Args... args)
  {
    creator(_device, args...);
  }
  template <typename... Args>
  void operator()(const Device::LogicalDevice::EngineDevice* _device, Args... args)
  {
    creator(_device, args...);
  }
  template <typename U>
  U& operator=(U&) = delete;
  ~EngineCommandPools()
  {
    for (auto& pool : command_pools)
      if (pool != VK_NULL_HANDLE)
        vkDestroyCommandPool(device->logical_device, pool, Alloc);
  }
  VkCommandPool& operator[](size_t t);
  VkCommandPool* data();
  void resize(size_t t);
  size_t size();

  std::vector<VkCommandPool>::iterator begin();
  std::vector<VkCommandPool>::iterator end();

  std::vector<VkCommandPool> command_pools;
  const Device::LogicalDevice::EngineDevice* device{};

 private:
  void creator(const Device::LogicalDevice::EngineDevice* _device,
               Data::CommandPoolCreate_Info _info,
               size_t _size,
               const VkAllocationCallbacks* alloc = nullptr);
  const VkAllocationCallbacks* Alloc{};
};

//===========================================================================================================================
// EngineOnceCommandBuffer
//===========================================================================================================================

VkCommandBuffer Get_OneTimeSubimit_CommandBuffer(VkDevice logical_device,
                                                 VkCommandPool command_pool);
void End_OneTimeSubimit_CommandBuffer(VkDevice logical_device,
                                      VkCommandPool command_pool,
                                      VkCommandBuffer command_buffer,
                                      VkQueue queue);

struct EngineOnceCommandBuffer
{
  EngineOnceCommandBuffer() = default;
  EngineOnceCommandBuffer(EngineOnceCommandBuffer&&) noexcept = default;
  EngineOnceCommandBuffer& operator=(EngineOnceCommandBuffer&&) noexcept = default;
  EngineOnceCommandBuffer(const Device::LogicalDevice::EngineDevice* _device,
                          VkCommandPool _command_pool,
                          VkQueue _queue);
  void operator()(const Device::LogicalDevice::EngineDevice* _device,
                  VkCommandPool _command_pool,
                  VkQueue _queue);
  template <typename U>
  U& operator=(U&) = delete;
  ~EngineOnceCommandBuffer()
  {
    destroyer();
  }
  void destroyer();
  void end_buffer();

  VkCommandBuffer command_buffer;
  const Device::LogicalDevice::EngineDevice* device;

 private:
  void creator(const Device::LogicalDevice::EngineDevice* _device,
               VkCommandPool _command_pool,
               VkQueue _queue);
  VkCommandPool command_pool;
  VkQueue queue;
};

//===========================================================================================================================
// EngineCommandBuffer
//===========================================================================================================================

struct EngineCommandBuffer
{
  EngineCommandBuffer() = default;
  EngineCommandBuffer(EngineCommandBuffer&&) noexcept = default;
  EngineCommandBuffer& operator=(EngineCommandBuffer&&) noexcept = default;
  EngineCommandBuffer(const Device::LogicalDevice::EngineDevice* _device,
                      VkCommandPool _command_pool);
  void operator()(const Device::LogicalDevice::EngineDevice* _device, VkCommandPool _command_pool);
  template <typename U>
  U& operator=(U&) = delete;
  ~EngineCommandBuffer()
  {
    if (command_pool != VK_NULL_HANDLE)
      vkFreeCommandBuffers(device->logical_device, command_pool, 1, &command_buffer);
  }

  VkCommandBuffer command_buffer{};
  const Device::LogicalDevice::EngineDevice* device{};

 private:
  void creator(const Device::LogicalDevice::EngineDevice* _device,
               VkCommandPool _command_pool,
               VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
  VkCommandPool command_pool{};
};

//===========================================================================================================================
// EngineCommandBuffers
//===========================================================================================================================

struct EngineCommandBuffers
{
  EngineCommandBuffers() = default;
  EngineCommandBuffers(EngineCommandBuffers&&) noexcept = default;
  EngineCommandBuffers& operator=(EngineCommandBuffers&&) noexcept = default;
  EngineCommandBuffers(const Device::LogicalDevice::EngineDevice* _device,
                       VkCommandPool _command_pool,
                       size_t _size);
  void operator()(const Device::LogicalDevice::EngineDevice* _device,
                  VkCommandPool _command_pool,
                  size_t _size);
  template <typename U>
  U& operator=(U&) = delete;
  ~EngineCommandBuffers()
  {
    if (command_pool != VK_NULL_HANDLE)
      vkFreeCommandBuffers(device->logical_device, command_pool, size(), command_buffers.data());
  }

  VkCommandBuffer& operator[](size_t t);
  VkCommandBuffer* data();
  void resize(size_t t);
  size_t size();
  void recreate(size_t n, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
  std::vector<VkCommandBuffer>::iterator begin();
  std::vector<VkCommandBuffer>::iterator end();

  std::vector<VkCommandBuffer> command_buffers;
  const Device::LogicalDevice::EngineDevice* device{};

 private:
  void creator(const Device::LogicalDevice::EngineDevice* _device,
               VkCommandPool _command_pool,
               size_t _size,
               VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
  VkCommandPool command_pool{};
};

}  // namespace SngoEngine::Core::Source::Buffer

#endif