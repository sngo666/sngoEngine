#ifndef __SNGO_SEMAPHORE_H
#define __SNGO_SEMAPHORE_H
#include <vulkan/vulkan_core.h>

#include <cstddef>
#include <vector>

#include "src/Core/Device/LogicalDevice.hpp"

namespace SngoEngine::Core::Siganlis
{

//===========================================================================================================================
// EngineSemaphores
//===========================================================================================================================

struct EngineSemaphores
{
  EngineSemaphores() = default;
  EngineSemaphores(EngineSemaphores&&) noexcept = default;
  EngineSemaphores& operator=(EngineSemaphores&&) noexcept = default;
  template <typename... Args>
  explicit EngineSemaphores(const Device::LogicalDevice::EngineDevice* _device, Args... args)
  {
    creator(_device, args...);
  }
  template <typename... Args>
  void init(const Device::LogicalDevice::EngineDevice* _device, Args... args)
  {
    creator(_device, args...);
  }
  ~EngineSemaphores()
  {
    destroyer();
  }

  VkSemaphore& operator[](size_t t);
  const VkSemaphore* data();
  void resize(size_t t);
  size_t size();
  std::vector<VkSemaphore>::iterator begin();
  std::vector<VkSemaphore>::iterator end();
  void destroyer();

  std::vector<VkSemaphore> semaphores;
  const Device::LogicalDevice::EngineDevice* device{};

 private:
  void creator(const Device::LogicalDevice::EngineDevice* _device,
               size_t _size,
               const VkAllocationCallbacks* alloc = nullptr);
  const VkAllocationCallbacks* Alloc{};
};

}  // namespace SngoEngine::Core::Siganlis

#endif