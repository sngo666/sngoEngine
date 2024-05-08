#include "RenderPass.hpp"

#include <vulkan/vulkan_core.h>

#include <vector>

#include "src/Core/Data.h"
#include "src/Core/Source/Image/Image.hpp"

SngoEngine::Core::Data::AttachmentDscription_Info
SngoEngine::Core::Render::RenderPass::Default_ColorAttachment(VkFormat format)
{
  return {format,
          VK_SAMPLE_COUNT_1_BIT,
          VK_ATTACHMENT_LOAD_OP_CLEAR,
          VK_ATTACHMENT_STORE_OP_STORE,
          VK_ATTACHMENT_LOAD_OP_DONT_CARE,
          VK_ATTACHMENT_STORE_OP_DONT_CARE,
          VK_IMAGE_LAYOUT_UNDEFINED,
          VK_IMAGE_LAYOUT_PRESENT_SRC_KHR};
}

SngoEngine::Core::Data::AttachmentDscription_Info
SngoEngine::Core::Render::RenderPass::Default_GUIColorAttachment(VkFormat format)
{
  return {format,
          VK_SAMPLE_COUNT_1_BIT,
          VK_ATTACHMENT_LOAD_OP_CLEAR,
          VK_ATTACHMENT_STORE_OP_STORE,
          VK_ATTACHMENT_LOAD_OP_DONT_CARE,
          VK_ATTACHMENT_STORE_OP_DONT_CARE,
          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
          VK_IMAGE_LAYOUT_PRESENT_SRC_KHR};
}

SngoEngine::Core::Data::AttachmentDscription_Info
SngoEngine::Core::Render::RenderPass::Default_DepthAttachment(VkPhysicalDevice physical_device)
{
  return {SngoEngine::Core::Source::Image::Find_DepthFormat(physical_device),
          VK_SAMPLE_COUNT_1_BIT,
          VK_ATTACHMENT_LOAD_OP_CLEAR,
          VK_ATTACHMENT_STORE_OP_STORE,
          VK_ATTACHMENT_LOAD_OP_DONT_CARE,
          VK_ATTACHMENT_STORE_OP_DONT_CARE,
          VK_IMAGE_LAYOUT_UNDEFINED,
          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
}

std::vector<SngoEngine::Core::Data::SubpassDependency_Info>
SngoEngine::Core::Render::RenderPass::DEFAULT_SUBPASS_DEPENDENCY()
{
  std::vector<SngoEngine::Core::Data::SubpassDependency_Info> deps;

  VkSubpassDependency dependency{};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  dependency.dstAccessMask =
      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

  deps.emplace_back(dependency);

  return deps;
}

SngoEngine::Core::Data::AttachmentData_Info
SngoEngine::Core::Render::RenderPass::DEFAULT_ATTACHMENTDATA(VkFormat format,
                                                             VkPhysicalDevice physical_device)
{
  auto color_attachment{Default_ColorAttachment(format)};
  auto depth_attachment{Default_DepthAttachment(physical_device)};

  VkAttachmentReference attachment_ref{};
  attachment_ref.attachment = 0;
  attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depth_attachment_ref{};
  depth_attachment_ref.attachment = 1;
  depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  std::vector<SngoEngine::Core::Data::AttachmentDscription_Info> descriptions{color_attachment,
                                                                              depth_attachment};
  return {descriptions,
          {},
          attachment_ref,
          depth_attachment_ref,
          {VkAttachmentReference()},
          {VkAttachmentReference()},
          1,
          1,
          0,
          0,
          0};
}

void SngoEngine::Core::Render::RenderPass::EngineRenderPass::creator(
    const Device::LogicalDevice::EngineDevice* _device,
    const std::vector<Data::SubpassDependency_Info>& _dependency,
    const VkSubpassDescription* _subpass,
    std::vector<VkAttachmentDescription>& _attachments,
    const VkAllocationCallbacks* alloc)
{
  device = _device;
  Alloc = alloc;

  VkRenderPassCreateInfo render_pass_info{};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  render_pass_info.attachmentCount = _attachments.size();
  render_pass_info.pAttachments = _attachments.data();
  render_pass_info.subpassCount = 1;
  render_pass_info.pSubpasses = _subpass;
  render_pass_info.dependencyCount = _dependency.size();
  render_pass_info.pDependencies = _dependency.data();

  if (vkCreateRenderPass(device->logical_device, &render_pass_info, Alloc, &render_pass)
      != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create render pass!");
    }
}

void SngoEngine::Core::Render::RenderPass::EngineRenderPass::creator(
    const Device::LogicalDevice::EngineDevice* _device,
    const std::vector<Data::SubpassDependency_Info>& _dependency,
    const std::vector<VkSubpassDescription>& _subpasses,
    std::vector<VkAttachmentDescription>& _attachments,
    const VkAllocationCallbacks* alloc)
{
  device = _device;
  Alloc = alloc;

  VkRenderPassCreateInfo render_pass_info{};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  render_pass_info.attachmentCount = _attachments.size();
  render_pass_info.pAttachments = _attachments.data();
  render_pass_info.subpassCount = _subpasses.size();
  render_pass_info.pSubpasses = _subpasses.data();
  render_pass_info.dependencyCount = _dependency.size();
  render_pass_info.pDependencies = _dependency.data();

  if (vkCreateRenderPass(device->logical_device, &render_pass_info, Alloc, &render_pass)
      != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create render pass!");
    }
}
