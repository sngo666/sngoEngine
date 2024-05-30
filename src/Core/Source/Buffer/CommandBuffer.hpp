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
  template <typename... Args>
  explicit EngineCommandPool(const Device::LogicalDevice::EngineDevice* _device, Args... args)
  {
    creator(_device, args...);
  }
  template <typename... Args>
  void init(const Device::LogicalDevice::EngineDevice* _device, Args... args)
  {
    creator(_device, args...);
  }
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
               const VkAllocationCallbacks* alloc = nullptr);
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
  void init(const Device::LogicalDevice::EngineDevice* _device, Args... args)
  {
    creator(_device, args...);
  }
  template <typename U>
  U& operator=(U&) = delete;
  ~EngineCommandPools() {}
  VkCommandPool& operator[](size_t t);
  VkCommandPool* data();
  void resize(size_t t);
  size_t size();

  std::vector<VkCommandPool>::iterator begin();
  std::vector<VkCommandPool>::iterator end();
  void destroyer();

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
  template <typename... Args>
  explicit EngineOnceCommandBuffer(const Device::LogicalDevice::EngineDevice* _device, Args... args)
  {
    creator(_device, args...);
  }
  template <typename... Args>
  void init(const Device::LogicalDevice::EngineDevice* _device, Args... args)
  {
    creator(_device, args...);
  }
  ~EngineOnceCommandBuffer()
  {
    destroyer();
  }
  void destroyer();
  void end_buffer();

  VkCommandBuffer command_buffer{};
  const Device::LogicalDevice::EngineDevice* device{};

 private:
  void creator(const Device::LogicalDevice::EngineDevice* _device,
               VkCommandPool _command_pool,
               VkQueue _queue);
  VkCommandPool command_pool{};
  VkQueue queue{};
};

//===========================================================================================================================
// EngineCommandBuffer
//===========================================================================================================================

struct EngineCommandBuffer
{
  EngineCommandBuffer() = default;
  EngineCommandBuffer(EngineCommandBuffer&&) noexcept = default;
  EngineCommandBuffer& operator=(EngineCommandBuffer&&) noexcept = default;
  template <typename... Args>
  explicit EngineCommandBuffer(const Device::LogicalDevice::EngineDevice* _device, Args... args)
  {
    creator(_device, args...);
  }
  template <typename... Args>
  void init(const Device::LogicalDevice::EngineDevice* _device, Args... args)
  {
    creator(_device, args...);
  }
  VkCommandBuffer operator()() const
  {
    return command_buffer;
  }
  ~EngineCommandBuffer()
  {
    destroyer();
  }
  void destroyer();

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
  template <typename... Args>
  explicit EngineCommandBuffers(const Device::LogicalDevice::EngineDevice* _device, Args... args)
  {
    creator(_device, args...);
  }
  template <typename... Args>
  void init(const Device::LogicalDevice::EngineDevice* _device, Args... args)
  {
    creator(_device, args...);
  }
  ~EngineCommandBuffers()
  {
    destroyer();
  }

  VkCommandBuffer& operator[](size_t t);
  VkCommandBuffer* data();
  void resize(size_t t);
  size_t size();
  void recreate(size_t n, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
  std::vector<VkCommandBuffer>::iterator begin();
  std::vector<VkCommandBuffer>::iterator end();
  void destroyer();

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