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

VkViewport Get_ViewPort(float width, float height, float minDepth, float maxDepth);
VkRect2D Get_Rect2D(uint32_t width, uint32_t height, int32_t offsetX, int32_t offsetY);

//===========================================================================================================================
// EngineRenderPass
//===========================================================================================================================

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
  void init(const Device::LogicalDevice::EngineDevice* _device, Args... args)
  {
    creator(_device, args...);
  }
  VkRenderPass operator()() const
  {
    return render_pass;
  }
  ~EngineRenderPass()
  {
    destroyer();
  }
  void destroyer();

  VkRenderPass render_pass{};
  const Device::LogicalDevice::EngineDevice* device{};

 private:
  void creator(const Device::LogicalDevice::EngineDevice* _device,
               const std::vector<VkSubpassDependency>& _dependency,
               const VkSubpassDescription* _subpass,
               std::vector<VkAttachmentDescription>& _attachments,
               const VkAllocationCallbacks* alloc = nullptr);
  void creator(const Device::LogicalDevice::EngineDevice* _device,
               const std::vector<VkSubpassDependency>& _dependency,
               const std::vector<VkSubpassDescription>& _subpasses,
               std::vector<VkAttachmentDescription>& _attachments,
               const VkAllocationCallbacks* alloc = nullptr);
  const VkAllocationCallbacks* Alloc{};
};

}  // namespace SngoEngine::Core::Render::RenderPass

#endif