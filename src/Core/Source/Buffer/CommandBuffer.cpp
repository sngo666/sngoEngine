#include "CommandBuffer.hpp"

#include <vulkan/vulkan_core.h>

#include <cstddef>
#include <cstdint>
#include <stdexcept>

#include "src/Core/Data.h"
#include "src/Core/Signalis/Fence.hpp"
#include "src/Core/Utils/Utils.hpp"

void SngoEngine::Core::Source::Buffer::EngineCommandPool::destroyer()
{
  if (command_pool != VK_NULL_HANDLE)
    {
      vkDestroyCommandPool(device->logical_device, command_pool, Alloc);
      command_pool = VK_NULL_HANDLE;
    }
}

void SngoEngine::Core::Source::Buffer::EngineCommandPool::creator(
    const Device::LogicalDevice::EngineDevice* _device,
    Data::CommandPoolCreate_Info _info,
    const VkAllocationCallbacks* alloc)
{
  destroyer();
  Alloc = alloc;
  device = _device;

  VkCommandPoolCreateInfo command_pool_info{_info};

  if (vkCreateCommandPool(device->logical_device, &command_pool_info, Alloc, &command_pool)
      != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create command pool");
    }
}

void SngoEngine::Core::Source::Buffer::EngineCommandPools::creator(
    const Device::LogicalDevice::EngineDevice* _device,
    Data::CommandPoolCreate_Info _info,
    size_t _size,
    const VkAllocationCallbacks* alloc)
{
  for (auto& pool : command_pools)
    if (pool != VK_NULL_HANDLE)
      vkDestroyCommandPool(device->logical_device, pool, Alloc);

  VkCommandPoolCreateInfo command_pool_info{_info};
  command_pools.resize(_size);
  device = _device;
  Alloc = alloc;

  for (int i = 0; i < _size; i++)
    {
      if (vkCreateCommandPool(device->logical_device, &command_pool_info, Alloc, &command_pools[i])
          != VK_SUCCESS)
        {
          throw std::runtime_error("failed to create command pool");
        }
    }
}

VkCommandPool& SngoEngine::Core::Source::Buffer::EngineCommandPools::operator[](size_t t)
{
  return command_pools[t];
}
VkCommandPool* SngoEngine::Core::Source::Buffer::EngineCommandPools::data()
{
  return command_pools.data();
}
void SngoEngine::Core::Source::Buffer::EngineCommandPools::resize(size_t t)
{
  command_pools.resize(t);
}

size_t SngoEngine::Core::Source::Buffer::EngineCommandPools::size()
{
  return command_pools.size();
}

std::vector<VkCommandPool>::iterator SngoEngine::Core::Source::Buffer::EngineCommandPools::begin()
{
  return command_pools.begin();
}
std::vector<VkCommandPool>::iterator SngoEngine::Core::Source::Buffer::EngineCommandPools::end()
{
  return command_pools.end();
}

void SngoEngine::Core::Source::Buffer::EngineCommandPools::destroyer()
{
  for (auto& pool : command_pools)
    if (pool != VK_NULL_HANDLE)
      vkDestroyCommandPool(device->logical_device, pool, Alloc);
}

//===========================================================================================================================
// EngineOnceCommandBuffer
//===========================================================================================================================

VkCommandBuffer SngoEngine::Core::Source::Buffer::Get_OneTimeSubimit_CommandBuffer(
    VkDevice logical_device,
    VkCommandPool command_pool)
{
  VkCommandBufferAllocateInfo alloc_info{
      Data::CommandBufferAlloc_Info(command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY)};

  VkCommandBuffer command_buffer{};
  vkAllocateCommandBuffers(logical_device, &alloc_info, &command_buffer);

  VkCommandBufferBeginInfo begin_info{};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(command_buffer, &begin_info);
  return command_buffer;
}

void SngoEngine::Core::Source::Buffer::End_OneTimeSubimit_CommandBuffer(
    VkDevice logical_device,
    VkCommandPool command_pool,
    VkCommandBuffer command_buffer,
    VkQueue queue,
    bool _free)
{
  if (command_buffer == VK_NULL_HANDLE)
    return;
  vkEndCommandBuffer(command_buffer);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &command_buffer;

  Data::FenceCreate_Info fenceInfo{0};
  VkFence fence;
  Utils::Vk_Exception(vkCreateFence(logical_device, &fenceInfo, nullptr, &fence));
  Utils::Vk_Exception(vkWaitForFences(logical_device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));
  Utils::Vk_Exception(vkQueueSubmit(queue, 1, &submitInfo, fence));

  vkDestroyFence(logical_device, fence, nullptr);
  if (_free)
    {
      vkFreeCommandBuffers(logical_device, command_pool, 1, &command_buffer);
    }
}

void SngoEngine::Core::Source::Buffer::EngineOnceCommandBuffer::destroyer()
{
  if (command_buffer != VK_NULL_HANDLE)
    {
      vkFreeCommandBuffers(device->logical_device, command_pool, 1, &command_buffer);
      command_buffer = VK_NULL_HANDLE;
    }
}

void SngoEngine::Core::Source::Buffer::EngineOnceCommandBuffer::end_buffer()
{
  End_OneTimeSubimit_CommandBuffer(
      device->logical_device, command_pool, command_buffer, queue, true);
}

void SngoEngine::Core::Source::Buffer::EngineOnceCommandBuffer::creator(
    const Device::LogicalDevice::EngineDevice* _device,
    VkCommandPool _command_pool,
    VkQueue _queue)
{
  device = _device;
  queue = _queue;
  command_pool = _command_pool;
  command_buffer = Get_OneTimeSubimit_CommandBuffer(device->logical_device, command_pool);
}

//===========================================================================================================================
// EngineCommandBuffer
//===========================================================================================================================

void SngoEngine::Core::Source::Buffer::EngineCommandBuffer::creator(
    const Device::LogicalDevice::EngineDevice* _device,
    VkCommandPool _command_pool,
    VkCommandBufferLevel level)
{
  if (command_buffer != VK_NULL_HANDLE)
    {
      vkFreeCommandBuffers(device->logical_device, command_pool, 1, &command_buffer);
      command_buffer = VK_NULL_HANDLE;
    }
  device = _device;
  command_pool = _command_pool;

  VkCommandBufferAllocateInfo command_buffers_info{
      Data::CommandBufferAlloc_Info(command_pool, level)};
  if (vkAllocateCommandBuffers(device->logical_device, &command_buffers_info, &command_buffer)
      != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create command buffer!");
    }
}

void SngoEngine::Core::Source::Buffer::EngineCommandBuffer::destroyer()
{
  if (command_pool)
    vkFreeCommandBuffers(device->logical_device, command_pool, 1, &command_buffer);
}
//===========================================================================================================================
// EngineCommandBuffers
//===========================================================================================================================

void SngoEngine::Core::Source::Buffer::EngineCommandBuffers::creator(
    const Device::LogicalDevice::EngineDevice* _device,
    VkCommandPool _command_pool,
    size_t _size,
    VkCommandBufferLevel level)
{
  if (command_pool != VK_NULL_HANDLE)
    vkFreeCommandBuffers(device->logical_device, command_pool, _size, command_buffers.data());
  device = _device;
  command_pool = _command_pool;
  command_buffers.resize(_size);

  VkCommandBufferAllocateInfo command_buffers_info{
      Data::CommandBufferAlloc_Info(command_pool, level)};
  if (vkAllocateCommandBuffers(
          device->logical_device, &command_buffers_info, command_buffers.data())
      != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create command buffer!");
    }
}

VkCommandBuffer& SngoEngine::Core::Source::Buffer::EngineCommandBuffers::operator[](size_t t)
{
  return command_buffers[t];
}

VkCommandBuffer* SngoEngine::Core::Source::Buffer::EngineCommandBuffers::data()
{
  return command_buffers.data();
}

void SngoEngine::Core::Source::Buffer::EngineCommandBuffers::resize(size_t t)
{
  command_buffers.resize(t);
}

size_t SngoEngine::Core::Source::Buffer::EngineCommandBuffers::size()
{
  return command_buffers.size();
}

std::vector<VkCommandBuffer>::iterator
SngoEngine::Core::Source::Buffer::EngineCommandBuffers::begin()
{
  return command_buffers.begin();
}

std::vector<VkCommandBuffer>::iterator SngoEngine::Core::Source::Buffer::EngineCommandBuffers::end()
{
  return command_buffers.end();
}

void SngoEngine::Core::Source::Buffer::EngineCommandBuffers::destroyer()
{
  if (command_pool)
    vkFreeCommandBuffers(device->logical_device, command_pool, size(), command_buffers.data());
}

void SngoEngine::Core::Source::Buffer::EngineCommandBuffers::recreate(size_t n,
                                                                      VkCommandBufferLevel level)
{
  VkCommandBufferAllocateInfo command_buffers_info{
      Data::CommandBufferAlloc_Info(command_pool, level)};
  if (vkAllocateCommandBuffers(device->logical_device, &command_buffers_info, &command_buffers[n])
      != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create command buffer!");
    }
}
