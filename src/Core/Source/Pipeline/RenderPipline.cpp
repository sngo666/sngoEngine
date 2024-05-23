#include "RenderPipline.hpp"

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <vector>

#include "src/Core/Data.h"
#include "src/Core/Source/Image/Image.hpp"
#include "src/Core/Source/Image/Sampler.hpp"
#include "src/Core/Source/Model/Model.hpp"
#include "src/Core/Source/SwapChain/SwapChain.hpp"
#include "src/Core/Utils/Utils.hpp"

//===========================================================================================================================
// pipeline info function
//===========================================================================================================================

SngoEngine::Core::Data::PipelinePreparation_Info
SngoEngine::Core::Source::RenderPipeline::Skybox_Pipeline(
    VkExtent2D _extent,
    VkPipelineVertexInputStateCreateInfo v_input)
{
  VkPipelineInputAssemblyStateCreateInfo input_assembly{Core::Data::GetInputAssembly_Info()};
  VkPipelineViewportStateCreateInfo viewport_state{Core::Data::GetViewportState_Info(_extent)};
  std::vector<VkDynamicState> dynamic_states{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dynamic_state{Core::Data::GetDynamicState_Info(dynamic_states)};
  VkPipelineRasterizationStateCreateInfo raterizer{Core::Data::DEFAULT_RASTERIZER_INFO()};
  VkPipelineMultisampleStateCreateInfo multisampling{Core::Data::DEFAULT_MULTISAMPLING_INFO()};
  VkPipelineDepthStencilStateCreateInfo depth_stencil{
      Core::Data::DEFAULT_DEPTHSTENCIL_DISABLED_INFO()};
  auto color_blend_attachment_info{Core::Data::GetDefaultColorBlend_Attachment()};
  VkPipelineColorBlendStateCreateInfo color_blend{
      Core::Data::DEFAULT_COLORBLEND_INFO(color_blend_attachment_info)};

  raterizer.cullMode = VK_CULL_MODE_FRONT_BIT;

  return {v_input,
          input_assembly,
          viewport_state,
          dynamic_state,
          raterizer,
          multisampling,
          depth_stencil,
          color_blend};
}

//===========================================================================================================================
// EngineFrameBufferAttachment
//===========================================================================================================================

void SngoEngine::Core::Source::RenderPipeline::EngineFrameBufferAttachment::init(
    Device::LogicalDevice::EngineDevice* _device,
    VkFormat _format,
    VkImageUsageFlagBits _usage,
    VkExtent2D _extent,
    VkSampleCountFlagBits samples)
{
  VkImageAspectFlags aspectMask{VK_IMAGE_ASPECT_NONE};
  VkImageLayout imageLayout{VK_IMAGE_LAYOUT_UNDEFINED};
  format = _format;

  if (_usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
    {
      aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
  if (_usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
    {
      aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

      if (format >= VK_FORMAT_D16_UNORM_S8_UINT)
        aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
      imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }
  assert(aspectMask > 0);

  img.init(_device,
           Data::ImageCreate_Info{format,
                                  VkExtent3D{_extent.width, _extent.height, 1},
                                  VK_IMAGE_TILING_OPTIMAL,
                                  (VkImageUsageFlags)(_usage | VK_IMAGE_USAGE_SAMPLED_BIT),
                                  1,
                                  1,
                                  samples},
           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  view.init(
      _device,
      Data::ImageViewCreate_Info{img.image, format, Data::ImageSubresourceRange_Info{aspectMask}});
}

//===========================================================================================================================
// EngineFrameBufferAttachment
//===========================================================================================================================

void SngoEngine::Core::Source::RenderPipeline::EngineOffscreenHDR_RenderPass::init(
    Device::LogicalDevice::EngineDevice* _device,
    VkExtent2D _extent,
    const VkAllocationCallbacks* alloc)

{
  extent = _extent;
  const uint32_t attach_count{3};

  // attchments initialization
  {
    attchment_FloatingPoint_0.init(
        _device, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, extent);
    attchment_FloatingPoint_1.init(
        _device, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, extent);
    attchment_Depth.init(_device,
                         Image::Find_DepthFormat(_device->pPD->physical_device),
                         VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                         extent);
  }

  // Attachment Description
  std::vector<VkAttachmentDescription> attachmentDescs{attach_count};
  {
    // attachment_0 : color
    {
      attachmentDescs[0].samples = VK_SAMPLE_COUNT_1_BIT;

      attachmentDescs[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      attachmentDescs[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;

      attachmentDescs[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      attachmentDescs[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

      attachmentDescs[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      attachmentDescs[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      attachmentDescs[0].format = attchment_FloatingPoint_0.format;
    }

    // attachment_1 : color
    {
      attachmentDescs[1].samples = VK_SAMPLE_COUNT_1_BIT;

      attachmentDescs[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      attachmentDescs[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;

      attachmentDescs[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      attachmentDescs[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

      attachmentDescs[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      attachmentDescs[1].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      attachmentDescs[1].format = attchment_FloatingPoint_1.format;
    }

    // attachment_3 : depth stencil
    {
      attachmentDescs[2].samples = VK_SAMPLE_COUNT_1_BIT;

      attachmentDescs[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      attachmentDescs[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;

      attachmentDescs[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      attachmentDescs[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

      attachmentDescs[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      attachmentDescs[2].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      attachmentDescs[2].format = attchment_Depth.format;
    }
  }

  // color references
  std::vector<VkAttachmentReference> colorReferences{};
  std::vector<VkAttachmentReference> depthReference{};
  {
    colorReferences.push_back({0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
    colorReferences.push_back({1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});

    depthReference.push_back({2, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL});
  }

  // subpass
  VkSubpassDescription subpass{};
  {
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.pColorAttachments = colorReferences.data();
    subpass.colorAttachmentCount = 2;

    subpass.pDepthStencilAttachment = depthReference.data();
  }

  // dependency
  std::vector<VkSubpassDependency> dependencies{2};
  {
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;

    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    //--------------------------------------------------------------------------------------

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;

    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

    dependencies[1].srcAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
  }

  // attachments
  std::vector<VkImageView> attachments{attach_count};
  {
    attachments[0] = attchment_FloatingPoint_0.view.image_view;
    attachments[1] = attchment_FloatingPoint_1.view.image_view;
    attachments[2] = attchment_Depth.view.image_view;
  }

  // sampler info
  auto sampler_info{Image::Get_Default_Sampler(_device)};
  {
    sampler_info.magFilter = VK_FILTER_NEAREST;
    sampler_info.minFilter = VK_FILTER_NEAREST;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.mipLodBias = 0.0f;
    sampler_info.maxAnisotropy = 1.0f;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = 1.0f;
    sampler_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
  }

  renderpass.init(_device, dependencies, &subpass, attachmentDescs, alloc);
  framebuffer.init(
      _device,
      Data::FrameBufferCreate_Info{
          renderpass.render_pass, attachments, VkExtent3D{extent.width, extent.height, 1}},
      alloc);

  sampler.init(_device, sampler_info, alloc);
}

//===========================================================================================================================
// EngineFrameBufferAttachment
//===========================================================================================================================

void SngoEngine::Core::Source::RenderPipeline::EngineBloomFilter_RenderPass::init(
    Device::LogicalDevice::EngineDevice* _device,
    VkExtent2D _extent,
    const VkAllocationCallbacks* alloc)
{
  extent = _extent;
  const uint32_t attach_count{1};

  // attchments initialization
  {
    attchment_FloatingPoint.init(
        _device, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, extent);
  }

  // Attachment Description
  std::vector<VkAttachmentDescription> attachmentDescs{attach_count};
  {
    // attachment_0 : color
    {
      attachmentDescs[0].samples = VK_SAMPLE_COUNT_1_BIT;

      attachmentDescs[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      attachmentDescs[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;

      attachmentDescs[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      attachmentDescs[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

      attachmentDescs[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      attachmentDescs[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      attachmentDescs[0].format = attchment_FloatingPoint.format;
    }
  }

  // color references
  std::vector<VkAttachmentReference> colorReferences{};
  {
    colorReferences.push_back({0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
  }

  // subpass
  VkSubpassDescription subpass{};
  {
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.pColorAttachments = colorReferences.data();
    subpass.colorAttachmentCount = 1;
  }

  // dependency
  std::vector<VkSubpassDependency> dependencies{2};
  {
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;

    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    //--------------------------------------------------------------------------------------

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;

    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

    dependencies[1].srcAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
  }

  // attachments
  std::vector<VkImageView> attachments{attach_count};
  {
    attachments[0] = attchment_FloatingPoint.view.image_view;
  }

  // sampler_infomation
  auto sampler_info{Image::Get_Default_Sampler(_device)};
  {
    sampler_info.magFilter = VK_FILTER_NEAREST;
    sampler_info.minFilter = VK_FILTER_NEAREST;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.mipLodBias = 0.0f;
    sampler_info.maxAnisotropy = 1.0f;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = 1.0f;
    sampler_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
  }

  renderpass.init(_device, dependencies, &subpass, attachmentDescs, alloc);
  framebuffer.init(
      _device,
      Data::FrameBufferCreate_Info{
          renderpass.render_pass, attachments, VkExtent3D{extent.width, extent.height, 1}},
      alloc);
  sampler.init(_device, sampler_info, alloc);
}

//===========================================================================================================================
// EngineMSAA_RenderPass
//===========================================================================================================================

void SngoEngine::Core::Source::RenderPipeline::EngineMSAA_RenderPass::init(
    Device::LogicalDevice::EngineDevice* _device,
    VkExtent2D _extent,
    VkFormat color_format,
    VkSampleCountFlagBits samplers,
    SwapChain::EngineSwapChain* swap_chain,
    const VkAllocationCallbacks* alloc)
{
  assert((_device->pPD->properties.limits.framebufferColorSampleCounts & samplers)
         && (_device->pPD->properties.limits.framebufferDepthSampleCounts & samplers));

  extent = _extent;
  const uint32_t attach_count{3};

  // attchments initialization
  {
    attchment_Color.init(_device,
                         color_format,
                         VkImageUsageFlagBits(VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT
                                              | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT),
                         extent,
                         samplers);

    attchment_Depth.init(_device,
                         Image::Find_DepthFormat(_device->pPD->physical_device),
                         VkImageUsageFlagBits(VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT
                                              | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT),
                         extent,
                         samplers);
  }

  // Attachment Description
  std::vector<VkAttachmentDescription> attachmentDescs{attach_count};
  {
    // attachment_0 : color
    {
      attachmentDescs[0].samples = samplers;

      attachmentDescs[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      attachmentDescs[0].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

      attachmentDescs[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      attachmentDescs[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

      attachmentDescs[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      attachmentDescs[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      attachmentDescs[0].format = attchment_Color.format;
    }
    // attachment_1 : color
    {
      attachmentDescs[1].samples = VK_SAMPLE_COUNT_1_BIT;

      attachmentDescs[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      attachmentDescs[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;

      attachmentDescs[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      attachmentDescs[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

      attachmentDescs[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      attachmentDescs[1].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
      attachmentDescs[1].format = attchment_Color.format;
    }
    // attachment_2 : depth
    {
      attachmentDescs[2].samples = samplers;

      attachmentDescs[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      attachmentDescs[2].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

      attachmentDescs[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      attachmentDescs[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

      attachmentDescs[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      attachmentDescs[2].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      attachmentDescs[2].format = attchment_Depth.format;
    }
  }

  // color references
  VkAttachmentReference colorReference{};
  VkAttachmentReference resolveReference{};
  VkAttachmentReference depthReference{};
  {
    colorReference = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    resolveReference = {1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    depthReference = {2, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
  }

  // subpass
  VkSubpassDescription subpass{};
  {
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorReference;

    subpass.pDepthStencilAttachment = &depthReference;
    subpass.pResolveAttachments = &resolveReference;
  }

  // dependency
  std::vector<VkSubpassDependency> dependencies{2};
  {
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;

    dependencies[0].srcStageMask =
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

    dependencies[0].dstStageMask =
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

    dependencies[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[0].dstAccessMask =
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    dependencies[0].dependencyFlags = 0;

    //--------------------------------------------------------------------------------------

    dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].dstSubpass = 0;

    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    dependencies[1].srcAccessMask = 0;
    dependencies[1].dstAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

    dependencies[1].dependencyFlags = 0;
  }

  // attachments
  std::vector<VkImageView> attachments{attach_count};
  {
    attachments[0] = attchment_Color.view.image_view;
    attachments[2] = attchment_Depth.view.image_view;
  }

  VkFramebufferCreateInfo frameBufferCreateInfo = {};
  {
    frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frameBufferCreateInfo.pNext = nullptr;
    frameBufferCreateInfo.renderPass = renderpass.render_pass;
    frameBufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    frameBufferCreateInfo.pAttachments = attachments.data();
    frameBufferCreateInfo.width = extent.width;
    frameBufferCreateInfo.height = extent.height;
    frameBufferCreateInfo.layers = 1;
  }

  // Create frame buffers for every swap chain image
  framebuffers.resize(swap_chain->images.size());
  for (uint32_t i = 0; i < framebuffers.size(); i++)
    {
      attachments[1] = swap_chain->image_views[i];
      Utils::Vk_Exception(vkCreateFramebuffer(
          _device->logical_device, &frameBufferCreateInfo, nullptr, &framebuffers[i]));
    }

  renderpass.init(_device, dependencies, &subpass, attachmentDescs, alloc);
}
