#include "Semaphore.h"

#include <vulkan/vulkan_core.h>

#include <cstddef>
#include <stdexcept>

SngoEngine::Core::Siganlis::EngineSemaphores::EngineSemaphores(
    const Device::LogicalDevice::EngineDevice* _device,
    size_t _size,
    const VkAllocationCallbacks* alloc)
    : SngoEngine::Core::Siganlis::EngineSemaphores()
{
  creator(_device, _size, alloc);
}

void SngoEngine::Core::Siganlis::EngineSemaphores::operator()(
    const Device::LogicalDevice::EngineDevice* _device,
    size_t _size,
    const VkAllocationCallbacks* alloc)
{
  creator(_device, _size, alloc);
}

void SngoEngine::Core::Siganlis::EngineSemaphores::creator(
    const Device::LogicalDevice::EngineDevice* _device,
    size_t _size,
    const VkAllocationCallbacks* alloc)
{
  for (auto& sema : semaphores)
    {
      if (sema != VK_NULL_HANDLE)
        vkDestroySemaphore(device->logical_device, sema, Alloc);
    }
  device = _device;
  Alloc = alloc;

  semaphores.resize(_size);
  VkSemaphoreCreateInfo semaphore_info{};
  semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  for (int i = 0; i < size(); i++)
    {
      if (vkCreateSemaphore(device->logical_device, &semaphore_info, nullptr, &semaphores[i])
          != VK_SUCCESS)
        {
          throw std::runtime_error("failed to create semaphores!");
        }
    }
}

VkSemaphore& SngoEngine::Core::Siganlis::EngineSemaphores::operator[](size_t t)
{
  return semaphores[t];
}

const VkSemaphore* SngoEngine::Core::Siganlis::EngineSemaphores::data()
{
  return semaphores.data();
}

void SngoEngine::Core::Siganlis::EngineSemaphores::resize(size_t t)
{
  semaphores.resize(t);
}
size_t SngoEngine::Core::Siganlis::EngineSemaphores::size()
{
  return semaphores.size();
}
std::vector<VkSemaphore>::iterator SngoEngine::Core::Siganlis::EngineSemaphores::begin()
{
  return semaphores.begin();
}
std::vector<VkSemaphore>::iterator SngoEngine::Core::Siganlis::EngineSemaphores::end()
{
  return semaphores.end();
}

void SngoEngine::Core::Siganlis::EngineSemaphores::destrpyer()
{
  for (auto& sema : semaphores)
    {
      if (sema != VK_NULL_HANDLE)
        vkDestroySemaphore(device->logical_device, sema, Alloc);
    }

  semaphores.clear();
}
