#ifndef __SNGO_RENDERPASS_H
#define __SNGO_RENDERPASS_H

#include <vulkan/vulkan_core.h>

#include <vector>

#include "src/Core/Device/LogicalDevice.hpp"

namespace SngoEngine::Core::Render::RenderPass
{

Data::AttachmentDscription_Info Default_ColorAttachment(VkFormat format);
Data::AttachmentDscription_Info Default_DepthAttachment(VkPhysicalDevice physical_device);
Data::AttachmentDscription_Info Default_GUIColorAttachment(VkFormat format);
std::vector<Data::SubpassDependency_Info> DEFAULT_SUBPASS_DEPENDENCY();
SngoEngine::Core::Data::AttachmentData_Info DEFAULT_ATTACHMENTDATA(
    VkFormat format,
    VkPhysicalDevice physical_device);

struct EngineRenderPass
{
  EngineRenderPass() = default;
  EngineRenderPass(EngineRenderPass&&) noexcept = default;
  EngineRenderPass& operator=(EngineRenderPass&&) noexcept = default;

  template <typename... Args>
  explicit EngineRenderPass(const Device::LogicalDevice::EngineDevice* _device, Args... args)
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
  ~EngineRenderPass()
  {
    if (render_pass != VK_NULL_HANDLE)
      vkDestroyRenderPass(device->logical_device, render_pass, Alloc);
  }

  VkRenderPass render_pass{};
  const Device::LogicalDevice::EngineDevice* device{};

 private:
  void creator(const Device::LogicalDevice::EngineDevice* _device,
               const std::vector<Data::SubpassDependency_Info>& _dependency,
               const VkSubpassDescription* _subpass,
               std::vector<VkAttachmentDescription>& _attachments,
               const VkAllocationCallbacks* alloc = nullptr);
  void creator(const Device::LogicalDevice::EngineDevice* _device,
               const std::vector<Data::SubpassDependency_Info>& _dependency,
               const std::vector<VkSubpassDescription>& _subpasses,
               std::vector<VkAttachmentDescription>& _attachments,
               const VkAllocationCallbacks* alloc = nullptr);
  const VkAllocationCallbacks* Alloc{};
};

}  // namespace SngoEngine::Core::Render::RenderPass

#endif