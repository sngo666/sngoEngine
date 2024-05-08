#include "Fence.h"

#include <cstddef>

SngoEngine::Core::Siganlis::EngineFences::EngineFences(
    const Device::LogicalDevice::EngineDevice* _device,
    size_t _size,
    VkFenceCreateFlags flags,
    const VkAllocationCallbacks* alloc)
    : EngineFences()
{
  creator(_device, _size, flags, alloc);
}

void SngoEngine::Core::Siganlis::EngineFences::operator()(
    const Device::LogicalDevice::EngineDevice* _device,
    size_t _size,
    VkFenceCreateFlags flags,
    const VkAllocationCallbacks* alloc)
{
  creator(_device, _size, flags, alloc);
}

void SngoEngine::Core::Siganlis::EngineFences::creator(
    const Device::LogicalDevice::EngineDevice* _device,
    size_t _size,
    VkFenceCreateFlags flags,
    const VkAllocationCallbacks* alloc)
{
  for (auto& fence : fences)
    {
      if (fence != VK_NULL_HANDLE)
        vkDestroyFence(device->logical_device, fence, Alloc);
    }
  Alloc = alloc;
  device = _device;

  fences.resize(_size);
  VkFenceCreateInfo fence_info{};
  fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_info.flags = flags;

  for (int i = 0; i < size(); i++)
    {
      if (vkCreateFence(device->logical_device, &fence_info, nullptr, &fences[i]) != VK_SUCCESS)
        {
          throw std::runtime_error("failed to create semaphores!");
        }
    }
}

void SngoEngine::Core::Siganlis::EngineFences::destroyer()
{
  for (auto& fence : fences)
    {
      if (fence != VK_NULL_HANDLE)
        vkDestroyFence(device->logical_device, fence, Alloc);
    }
  fences.clear();
}

VkFence& SngoEngine::Core::Siganlis::EngineFences::operator[](size_t t)
{
  return fences[t];
}

const VkFence* SngoEngine::Core::Siganlis::EngineFences::data()
{
  return fences.data();
}

void SngoEngine::Core::Siganlis::EngineFences::resize(size_t t)
{
  fences.resize(t);
}
size_t SngoEngine::Core::Siganlis::EngineFences::size()
{
  return fences.size();
}

std::vector<VkFence>::iterator SngoEngine::Core::Siganlis::EngineFences::begin()
{
  return fences.begin();
}
std::vector<VkFence>::iterator SngoEngine::Core::Siganlis::EngineFences::end()
{
  return fences.end();
}
