#ifndef __SNGO_FENCE_H
#define __SNGO_FENCE_H
#include <vulkan/vulkan_core.h>

#include "src/Core/Device/LogicalDevice.hpp"

namespace SngoEngine::Core::Siganlis
{

struct EngineFences
{
  EngineFences() = default;
  EngineFences(EngineFences&&) noexcept = default;
  EngineFences& operator=(EngineFences&&) noexcept = default;
  EngineFences(const Device::LogicalDevice::EngineDevice* _device,
               size_t _size,
               VkFenceCreateFlags flags = VK_FENCE_CREATE_SIGNALED_BIT,
               const VkAllocationCallbacks* alloc = nullptr);
  void operator()(const Device::LogicalDevice::EngineDevice* _device,
                  size_t _size,
                  VkFenceCreateFlags flags = VK_FENCE_CREATE_SIGNALED_BIT,
                  const VkAllocationCallbacks* alloc = nullptr);
  template <typename U>
  U& operator=(U&) = delete;
  ~EngineFences()
  {
    destroyer();
  }

  VkFence& operator[](size_t t);
  const VkFence* data();
  void resize(size_t t);
  size_t size();
  std::vector<VkFence>::iterator begin();
  std::vector<VkFence>::iterator end();
  void destroyer();

  std::vector<VkFence> fences;
  const Device::LogicalDevice::EngineDevice* device{};

 private:
  void creator(const Device::LogicalDevice::EngineDevice* _device,
               size_t _size,
               VkFenceCreateFlags flags,
               const VkAllocationCallbacks* alloc);
  const VkAllocationCallbacks* Alloc{};
};
}  // namespace SngoEngine::Core::Siganlis
#endif