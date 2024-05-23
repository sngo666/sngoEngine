#include "FrameBuffer.hpp"

#include <vulkan/vulkan_core.h>

#include <cstddef>
#include <stdexcept>

#include "src/Core/Device/LogicalDevice.hpp"

//===========================================================================================================================
// EngineFrameBuffer
//===========================================================================================================================

void SngoEngine::Core::Render::EngineFrameBuffer::creator(
    const SngoEngine::Core::Device::LogicalDevice::EngineDevice* _device,
    VkRenderPass _render_pass,
    const std::vector<VkImageView>& _attachments,
    VkExtent2D _extent,
    uint32_t _layers,
    const VkAllocationCallbacks* alloc)
{
  device = _device;
  Alloc = alloc;

  VkFramebufferCreateInfo frame_buffer_info{};
  frame_buffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  frame_buffer_info.renderPass = _render_pass;
  frame_buffer_info.attachmentCount = _attachments.size();
  frame_buffer_info.pAttachments = _attachments.data();
  frame_buffer_info.width = _extent.width;
  frame_buffer_info.height = _extent.height;
  frame_buffer_info.layers = _layers;

  if (vkCreateFramebuffer(device->logical_device, &frame_buffer_info, Alloc, &frame_buffer)
      != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create frame buffer!");
    }
}

void SngoEngine::Core::Render::EngineFrameBuffer::creator(
    const SngoEngine::Core::Device::LogicalDevice::EngineDevice* _device,
    Data::FrameBufferCreate_Info _info,
    const VkAllocationCallbacks* alloc)
{
  device = _device;
  Alloc = alloc;

  if (vkCreateFramebuffer(device->logical_device, &_info, Alloc, &frame_buffer) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create frame buffer!");
    }
}

void SngoEngine::Core::Render::EngineFrameBuffer::destroyer()
{
  if (!frame_buffer && device)
    vkDestroyFramebuffer(device->logical_device, frame_buffer, Alloc);
}

//===========================================================================================================================
// EngineFrameBuffers
//===========================================================================================================================

void SngoEngine::Core::Render::EngineFrameBuffers::creator(
    const SngoEngine::Core::Device::LogicalDevice::EngineDevice* _device,
    const std::vector<VkFramebufferCreateInfo>& _infos,
    const VkAllocationCallbacks* alloc)
{
  destroyer();
  Alloc = alloc;
  device = _device;

  frame_buffers.resize(_infos.size());

  for (size_t i = 0; i < _infos.size(); i++)
    {
      if (vkCreateFramebuffer(device->logical_device, &_infos[i], Alloc, &frame_buffers[i])
          != VK_SUCCESS)
        {
          throw std::runtime_error("failed to create frame buffer!");
        }
    }
}

void SngoEngine::Core::Render::EngineFrameBuffers::creator(
    const SngoEngine::Core::Device::LogicalDevice::EngineDevice* _device,
    VkRenderPass _render_pass,
    const std::vector<std::vector<VkImageView>>& _attachments,
    VkExtent2D _extent,
    uint32_t _layers,
    const VkAllocationCallbacks* alloc)
{
  destroyer();

  Alloc = alloc;
  device = _device;

  frame_buffers.resize(_attachments.size());

  for (size_t i = 0; i < _attachments.size(); i++)

    {
      VkFramebufferCreateInfo frame_buffer_info{};
      frame_buffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      frame_buffer_info.renderPass = _render_pass;
      frame_buffer_info.attachmentCount = _attachments[i].size();
      frame_buffer_info.pAttachments = _attachments[i].data();
      frame_buffer_info.width = _extent.width;
      frame_buffer_info.height = _extent.height;
      frame_buffer_info.layers = _layers;

      if (vkCreateFramebuffer(device->logical_device, &frame_buffer_info, Alloc, &frame_buffers[i])
          != VK_SUCCESS)
        {
          throw std::runtime_error("failed to create frame buffer!");
        }
    }
}

void SngoEngine::Core::Render::EngineFrameBuffers::destroyer()
{
  for (auto& buffer : frame_buffers)
    if (!buffer && device)
      vkDestroyFramebuffer(device->logical_device, buffer, Alloc);

  frame_buffers.clear();
}

VkFramebuffer& SngoEngine::Core::Render::EngineFrameBuffers::operator[](size_t t)
{
  return frame_buffers[t];
}

VkFramebuffer* SngoEngine::Core::Render::EngineFrameBuffers::data()
{
  return frame_buffers.data();
}

void SngoEngine::Core::Render::EngineFrameBuffers::resize(size_t t)
{
  frame_buffers.resize(t);
}

size_t SngoEngine::Core::Render::EngineFrameBuffers::size() const
{
  return frame_buffers.size();
}

std::vector<VkFramebuffer>::iterator SngoEngine::Core::Render::EngineFrameBuffers::begin()
{
  return frame_buffers.begin();
}
std::vector<VkFramebuffer>::iterator SngoEngine::Core::Render::EngineFrameBuffers::end()
{
  return frame_buffers.end();
}