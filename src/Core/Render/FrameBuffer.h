#ifndef __SNGO_FRAMEBUFFER_H
#define __SNGO_FRAMEBUFFER_H

#include <vulkan/vulkan_core.h>

#include <cstddef>
#include <vector>

#include "src/Core/Data.h"
#include "src/Core/Device/LogicalDevice.hpp"

namespace SngoEngine::Core::Render
{

struct EngineFrameBuffer
{
  EngineFrameBuffer() = delete;
  ~EngineFrameBuffer()
  {
    vkDestroyFramebuffer(device->logical_device, frame_buffer, Alloc);
  }

  EngineFrameBuffer(const SngoEngine::Core::Device::LogicalDevice::EngineDevice* _device,
                    const VkRenderPass* _render_pass,
                    const std::vector<VkImageView>& _attachments,
                    VkExtent2D _extent,
                    uint32_t _layers = 1,
                    const VkAllocationCallbacks* alloc = nullptr);
  EngineFrameBuffer(const SngoEngine::Core::Device::LogicalDevice::EngineDevice* _device,
                    Data::FrameBufferCreate_Info _info,
                    const VkAllocationCallbacks* alloc = nullptr);

  VkFramebuffer frame_buffer;
  const Device::LogicalDevice::EngineDevice* device;

 private:
  const VkAllocationCallbacks* Alloc;
};

struct EngineFrameBuffers
{
  EngineFrameBuffers() = default;
  EngineFrameBuffers(EngineFrameBuffers&&) noexcept = default;
  EngineFrameBuffers& operator=(EngineFrameBuffers&&) noexcept = default;
  EngineFrameBuffers(const SngoEngine::Core::Device::LogicalDevice::EngineDevice* _device,
                     const VkRenderPass* _render_pass,
                     const std::vector<std::vector<VkImageView>>& _attachments,
                     VkExtent2D _extent,
                     uint32_t _layers = 1,
                     const VkAllocationCallbacks* alloc = nullptr);
  void operator()(const SngoEngine::Core::Device::LogicalDevice::EngineDevice* _device,
                  const VkRenderPass* _render_pass,
                  const std::vector<std::vector<VkImageView>>& _attachments,
                  VkExtent2D _extent,
                  uint32_t _layers = 1,
                  const VkAllocationCallbacks* alloc = nullptr);
  template <typename U>
  U& operator=(U&) = delete;
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
               const VkRenderPass* _render_pass,
               const std::vector<std::vector<VkImageView>>& _attachments,
               VkExtent2D _extent,
               uint32_t _layers,
               const VkAllocationCallbacks* alloc);
  const VkAllocationCallbacks* Alloc{};
};

}  // namespace SngoEngine::Core::Render
#endif