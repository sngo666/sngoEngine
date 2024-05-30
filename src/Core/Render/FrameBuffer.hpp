#ifndef __SNGO_FRAMEBUFFER_H
#define __SNGO_FRAMEBUFFER_H

#include <vulkan/vulkan_core.h>

#include <cstddef>
#include <vector>

#include "src/Core/Data.h"
#include "src/Core/Device/LogicalDevice.hpp"

namespace SngoEngine::Core::Render
{

//===========================================================================================================================
// EngineFrameBuffer
//===========================================================================================================================

struct EngineFrameBuffer
{
  EngineFrameBuffer() = default;

  template <typename... Args>
  explicit EngineFrameBuffer(const SngoEngine::Core::Device::LogicalDevice::EngineDevice* _device,
                             Args... args)
  {
    creator(_device, args...);
  }
  template <typename... Args>
  void init(const SngoEngine::Core::Device::LogicalDevice::EngineDevice* _device, Args... args)
  {
    creator(_device, args...);
  }
  VkFramebuffer operator()() const
  {
    return frame_buffer;
  }
  ~EngineFrameBuffer()
  {
    destroyer();
  }
  void destroyer();

  VkFramebuffer frame_buffer{};
  const Device::LogicalDevice::EngineDevice* device{};

 private:
  void creator(const SngoEngine::Core::Device::LogicalDevice::EngineDevice* _device,
               VkRenderPass _render_pass,
               const std::vector<VkImageView>& _attachments,
               VkExtent2D _extent,
               uint32_t _layers = 1,
               const VkAllocationCallbacks* alloc = nullptr);
  void creator(const SngoEngine::Core::Device::LogicalDevice::EngineDevice* _device,
               Data::FrameBufferCreate_Info _info,
               const VkAllocationCallbacks* alloc = nullptr);
  const VkAllocationCallbacks* Alloc{};
};

//===========================================================================================================================
// EngineFrameBuffers
//===========================================================================================================================

struct EngineFrameBuffers
{
  EngineFrameBuffers() = default;
  EngineFrameBuffers(EngineFrameBuffers&&) noexcept = default;
  EngineFrameBuffers& operator=(EngineFrameBuffers&&) noexcept = default;
  template <typename... Args>
  explicit EngineFrameBuffers(const SngoEngine::Core::Device::LogicalDevice::EngineDevice* _device,
                              Args... args)
  {
    creator(_device, args...);
  }
  template <typename... Args>
  void init(const SngoEngine::Core::Device::LogicalDevice::EngineDevice* _device, Args... args)
  {
    creator(_device, args...);
  }
  ~EngineFrameBuffers()
  {
    destroyer();
  }

  VkFramebuffer& operator[](size_t t);
  VkFramebuffer* data();
  void resize(size_t t);
  [[nodiscard]] size_t size() const;
  std::vector<VkFramebuffer>::iterator begin();
  std::vector<VkFramebuffer>::iterator end();
  void destroyer();

  std::vector<VkFramebuffer> frame_buffers;
  const Device::LogicalDevice::EngineDevice* device{};

 private:
  void creator(const SngoEngine::Core::Device::LogicalDevice::EngineDevice* _device,
               VkRenderPass _render_pass,
               const std::vector<std::vector<VkImageView>>& _attachments,
               VkExtent2D _extent,
               uint32_t _layers = 1,
               const VkAllocationCallbacks* alloc = nullptr);
  void creator(const SngoEngine::Core::Device::LogicalDevice::EngineDevice* _device,
               const std::vector<VkFramebufferCreateInfo>& _infos,
               const VkAllocationCallbacks* alloc = nullptr);
  const VkAllocationCallbacks* Alloc{};
};

}  // namespace SngoEngine::Core::Render
#endif