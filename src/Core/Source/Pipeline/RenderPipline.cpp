#include "RenderPipline.hpp"

#include <vulkan/vulkan_core.h>

#include <cassert>
#include <cstdint>
#include <vector>

#include "src/Core/Data.h"
#include "src/Core/Source/Buffer/Descriptor.hpp"
#include "src/Core/Source/Buffer/VertexBuffer.hpp"
#include "src/Core/Source/Image/Image.hpp"
#include "src/Core/Source/Image/Sampler.hpp"
#include "src/Core/Source/Model/Model.hpp"
#include "src/Core/Source/Pipeline/Pipeline.hpp"
#include "src/Core/Source/SwapChain/SwapChain.hpp"
#include "src/Core/Utils/Utils.hpp"

//===========================================================================================================================
// pipeline info function
//===========================================================================================================================

SngoEngine::Core::Data::PipelinePreparation_Info
SngoEngine::Core::Source::RenderPipeline::Default_Pipeline(
    VkExtent2D _extent,
    VkPipelineVertexInputStateCreateInfo v_input)
{
  Data::PipelinePreparation_Info _info{};

  _info.vertex_input = v_input;
  _info.input_assembly = Core::Data::GetInputAssembly_Info();
  _info.view_state = Core::Data::GetViewportState_Info(_extent);
  _info.dynamic_states = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
  _info.dynamic_state = {Core::Data::GetDynamicState_Info(_info.dynamic_states)};
  _info.rasterizer = {Core::Data::DEFAULT_RASTERIZER_INFO()};
  _info.multisampling = {Core::Data::MULTISAMPLING_INFO_DEFAULT()};
  _info.depth_stencil = {Core::Data::DEFAULT_DEPTHSTENCIL_DISABLED_INFO()};
  _info.color_blend_attachment_info = {Core::Data::GetColorBlend_DEFAULT()};
  _info.color_blend = {Core::Data::DEFAULT_COLORBLEND_INFO(&_info.color_blend_attachment_info)};

  return _info;
}

SngoEngine::Core::Data::PipelinePreparation_Info
SngoEngine::Core::Source::RenderPipeline::Skybox_Pipeline(
    VkExtent2D _extent,
    VkPipelineVertexInputStateCreateInfo v_input)
{
  Data::PipelinePreparation_Info _info{};

  _info.vertex_input = v_input;
  _info.input_assembly = Core::Data::GetInputAssembly_Info();
  _info.view_state = Core::Data::GetViewportState_Info(_extent);
  _info.dynamic_states = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
  _info.dynamic_state = {Core::Data::GetDynamicState_Info(_info.dynamic_states)};
  _info.rasterizer = {Core::Data::DEFAULT_RASTERIZER_INFO()};
  _info.multisampling = {Core::Data::MULTISAMPLING_INFO_DEFAULT()};
  _info.depth_stencil = {Core::Data::DEFAULT_DEPTHSTENCIL_DISABLED_INFO()};
  _info.color_blend_attachment_info = {Core::Data::GetColorBlend_DEFAULT()};
  _info.color_blend = {Core::Data::DEFAULT_COLORBLEND_INFO(&_info.color_blend_attachment_info)};

  _info.rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;

  return _info;
}

SngoEngine::Core::Data::PipelinePreparation_Info
SngoEngine::Core::Source::RenderPipeline::Bloomfilter_Pipeline(
    VkExtent2D _extent,
    VkPipelineVertexInputStateCreateInfo v_input)
{
  Data::PipelinePreparation_Info _info{};

  _info.vertex_input = v_input;
  _info.input_assembly = Core::Data::GetInputAssembly_Info();
  _info.view_state = Core::Data::GetViewportState_Info(_extent);
  _info.dynamic_states = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
  _info.dynamic_state = {Core::Data::GetDynamicState_Info(_info.dynamic_states)};
  _info.rasterizer = {Core::Data::DEFAULT_RASTERIZER_INFO()};
  _info.multisampling = {Core::Data::MULTISAMPLING_INFO_DEFAULT()};
  _info.depth_stencil = {Core::Data::DEFAULT_DEPTHSTENCIL_DISABLED_INFO()};
  _info.color_blend_attachment_info = Core::Data::GetColorBlend_BLOOMFILTER();
  _info.color_blend = {Core::Data::DEFAULT_COLORBLEND_INFO(&_info.color_blend_attachment_info)};

  return _info;
}

SngoEngine::Core::Data::PipelinePreparation_Info
SngoEngine::Core::Source::RenderPipeline::ShadowMapping_Pipeline(
    VkExtent2D _extent,
    VkPipelineVertexInputStateCreateInfo v_input)
{
  Data::PipelinePreparation_Info _info{};

  _info.vertex_input = v_input;
  _info.input_assembly = Core::Data::GetInputAssembly_Info();
  _info.view_state = Core::Data::GetViewportState_Info(_extent);
  _info.dynamic_states = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
  _info.dynamic_state = {Core::Data::GetDynamicState_Info(_info.dynamic_states)};
  _info.rasterizer = {Core::Data::DEFAULT_RASTERIZER_INFO()};
  _info.multisampling = {Core::Data::MULTISAMPLING_INFO_DEFAULT()};
  _info.depth_stencil = {Core::Data::DEFAULT_DEPTHSTENCIL_INFO(VK_COMPARE_OP_LESS_OR_EQUAL)};
  _info.color_blend_attachment_info = Core::Data::GetColorBlend_DEFAULT();
  _info.color_blend = {Core::Data::DEFAULT_COLORBLEND_INFO(&_info.color_blend_attachment_info)};

  return _info;
}

//===========================================================================================================================
// EngineFrameBufferAttachment
//===========================================================================================================================

void SngoEngine::Core::Source::RenderPipeline::EngineFrameBufferAttachment::init(
    const Device::LogicalDevice::EngineDevice* _device,
    VkFormat _format,
    VkImageUsageFlagBits _usage,
    VkExtent2D _extent,
    VkSampleCountFlagBits samples,
    uint32_t _mipmap)
{
  VkImageAspectFlags aspectMask{VK_IMAGE_ASPECT_NONE};
  VkImageLayout imageLayout{VK_IMAGE_LAYOUT_UNDEFINED};
  format = _format;
  device = _device;

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

  if (!(_usage & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT))
    {
      _usage = VkImageUsageFlagBits(_usage | VK_IMAGE_USAGE_SAMPLED_BIT);
    }

  img.init(_device,
           Data::ImageCreate_Info{format,
                                  VkExtent3D{_extent.width, _extent.height, 1},
                                  VK_IMAGE_TILING_OPTIMAL,
                                  _usage,
                                  _mipmap,
                                  1,
                                  samples},
           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  view.init(
      _device,
      Data::ImageViewCreate_Info{img.image, format, Data::ImageSubresourceRange_Info{aspectMask}});
}

void SngoEngine::Core::Source::RenderPipeline::EngineFrameBufferAttachment::destroyer()
{
  if (device)
    {
      img.destroyer();
      view.destroyer();
    }
}

//===========================================================================================================================
// EngineOffscreenHDR_RenderPass
//===========================================================================================================================

void SngoEngine::Core::Source::RenderPipeline::EngineOffscreenHDR_RenderPass::init(
    const Device::LogicalDevice::EngineDevice* _device,
    VkExtent2D _extent,
    VkSampleCountFlagBits samplers,
    const VkAllocationCallbacks* alloc)

{
  extent = _extent;
  device = _device;
  const uint32_t attach_count{5};

  // attchments initialization
  {
    VkImageUsageFlagBits _usage{VkImageUsageFlagBits(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)};

    attchment_FloatingPoint_0.init(
        _device, VK_FORMAT_R32G32B32A32_SFLOAT, _usage, extent, samplers);
    attchment_FloatingPoint_1.init(
        _device, VK_FORMAT_R32G32B32A32_SFLOAT, _usage, extent, samplers);
    attchment_Depth.init(_device,
                         Image::Find_DepthFormat(_device->pPD->physical_device),
                         VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                         extent,
                         samplers);
    attchment_Resolve_0.init(
        _device, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, extent);
    attchment_Resolve_1.init(
        _device, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, extent);
  }

  // Attachment Description
  std::vector<VkAttachmentDescription> attachmentDescs{attach_count};
  {
    // attachment_0 : color
    {
      attachmentDescs[0].samples = samplers;

      attachmentDescs[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      attachmentDescs[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;

      attachmentDescs[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      attachmentDescs[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

      attachmentDescs[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      attachmentDescs[0].finalLayout = VK_IMAGE_LAYOUT_GENERAL;
      attachmentDescs[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    }

    // attachment_1 : color
    {
      attachmentDescs[1].samples = samplers;

      attachmentDescs[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      attachmentDescs[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;

      attachmentDescs[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      attachmentDescs[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

      attachmentDescs[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      attachmentDescs[1].finalLayout = VK_IMAGE_LAYOUT_GENERAL;
      attachmentDescs[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    }

    // attachment_3 : depth stencil
    {
      attachmentDescs[2].samples = samplers;

      attachmentDescs[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      attachmentDescs[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;

      attachmentDescs[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      attachmentDescs[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

      attachmentDescs[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      attachmentDescs[2].finalLayout = VK_IMAGE_LAYOUT_GENERAL;
      attachmentDescs[2].format = attchment_Depth.format;
    }

    // attachment_4 : resolve
    {
      attachmentDescs[3].samples = VK_SAMPLE_COUNT_1_BIT;

      attachmentDescs[3].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      attachmentDescs[3].storeOp = VK_ATTACHMENT_STORE_OP_STORE;

      attachmentDescs[3].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      attachmentDescs[3].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

      attachmentDescs[3].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      attachmentDescs[3].finalLayout = VK_IMAGE_LAYOUT_GENERAL;
      attachmentDescs[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    }

    // attachment_5 : resolve
    {
      attachmentDescs[4].samples = VK_SAMPLE_COUNT_1_BIT;

      attachmentDescs[4].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      attachmentDescs[4].storeOp = VK_ATTACHMENT_STORE_OP_STORE;

      attachmentDescs[4].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      attachmentDescs[4].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

      attachmentDescs[4].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      attachmentDescs[4].finalLayout = VK_IMAGE_LAYOUT_GENERAL;
      attachmentDescs[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    }
  }

  // color references
  std::vector<VkAttachmentReference> colorReferences{};
  std::vector<VkAttachmentReference> depthReference{};
  std::vector<VkAttachmentReference> resolveReference{};

  {
    colorReferences.push_back({0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
    colorReferences.push_back({1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});

    depthReference.push_back({2, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL});
    resolveReference.push_back({3, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
    resolveReference.push_back({4, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
  }

  // subpass
  VkSubpassDescription subpass{};
  {
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.pColorAttachments = colorReferences.data();
    subpass.colorAttachmentCount = 2;
    subpass.pResolveAttachments = resolveReference.data();
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
    attachments[3] = attchment_Resolve_0.view.image_view;
    attachments[4] = attchment_Resolve_1.view.image_view;
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
// EngineBloomFilter_RenderPass
//===========================================================================================================================

void SngoEngine::Core::Source::RenderPipeline::EngineBloomFilter_RenderPass::init(
    const Device::LogicalDevice::EngineDevice* _device,
    VkExtent2D _extent,
    const VkAllocationCallbacks* alloc)
{
  extent = _extent;
  device = _device;
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
      attachmentDescs[0].finalLayout = VK_IMAGE_LAYOUT_GENERAL;
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

void SngoEngine::Core::Source::RenderPipeline::EngineBloomFilter_RenderPass::construct_descriptor(
    Descriptor::EngineDescriptorPool* _pool,
    std::vector<VkDescriptorImageInfo>& imginfos,
    const VkAllocationCallbacks* alloc)
{
  assert(device);

  std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
      Core::Source::Descriptor::GetLayoutBinding(
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
      Core::Source::Descriptor::GetLayoutBinding(
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1)};

  bloom_setlayout.init(device, setLayoutBindings, alloc);
  bloom_set.init(device, &bloom_setlayout, _pool);

  std::vector<VkWriteDescriptorSet> writes{
      Descriptor::GetDescriptSet_Write(
          bloom_set.descriptor_set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &imginfos[0])[0],
      Descriptor::GetDescriptSet_Write(
          bloom_set.descriptor_set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &imginfos[1])[0]};

  bloom_set.updateWrite(writes);
}

void SngoEngine::Core::Source::RenderPipeline::EngineBloomFilter_RenderPass::construct_pipeline(
    std::vector<VkPipelineShaderStageCreateInfo>& _shader_stage,
    Pipeline::EnginePipelineLayout* _layout,
    Render::RenderPass::EngineRenderPass* _render_pass,
    VkSampleCountFlagBits _sampler_flag,
    const VkAllocationCallbacks* alloc)
{
  assert(device);

  pipeline_layout.init(device,
                       std::vector<VkDescriptorSetLayout>{bloom_setlayout.layout},
                       std::vector<VkPushConstantRange>{},
                       alloc);

  VkSpecializationMapEntry specMapEntry{Pipeline::Get_SpecMapEntry(0, 0, sizeof(uint32_t))};
  uint32_t dir{1};
  VkSpecializationInfo spec_info{
      Pipeline::Get_SpecializationInfo(1, &specMapEntry, sizeof(dir), &dir)};
  _shader_stage[1].pSpecializationInfo = &spec_info;

  // pipeline_0
  auto pipeline_info{Bloomfilter_Pipeline(extent, Buffer::Get_EmptyVertexInputState())};

  if (_sampler_flag != VK_SAMPLE_COUNT_1_BIT)
    {
      pipeline_info.multisampling = Data::MULTISAMPLING_INFO_ENABLED(_sampler_flag, 0.25f);
    }
  pipelines[0].init(device, _layout, _render_pass, _shader_stage, &pipeline_info, 0, alloc);

  // pipeline_1
  pipeline_info.multisampling = Data::MULTISAMPLING_INFO_DEFAULT();
  dir = 0;
  pipelines[1].init(device, _layout, &renderpass, _shader_stage, &pipeline_info, 0, alloc);
}

void SngoEngine::Core::Source::RenderPipeline::EngineBloomFilter_RenderPass::destroyer()
{
  if (device)
    {
      attchment_FloatingPoint.destroyer();

      framebuffer.destroyer();
      renderpass.destroyer();
      sampler.destroyer();

      bloom_setlayout.destroyer();
    }
}

//===========================================================================================================================
// EngineMSAA_RenderPass
//===========================================================================================================================

VkSampleCountFlagBits SngoEngine::Core::Source::RenderPipeline::MSAA_maxAvailableSampleCount(
    Device::LogicalDevice::EngineDevice* _device)
{
  VkSampleCountFlags supportedSampleCount =
      std::min(_device->pPD->properties.limits.framebufferColorSampleCounts,
               _device->pPD->properties.limits.framebufferDepthSampleCounts);
  std::vector<VkSampleCountFlagBits> possibleSampleCounts{VK_SAMPLE_COUNT_64_BIT,
                                                          VK_SAMPLE_COUNT_32_BIT,
                                                          VK_SAMPLE_COUNT_16_BIT,
                                                          VK_SAMPLE_COUNT_8_BIT,
                                                          VK_SAMPLE_COUNT_4_BIT,
                                                          VK_SAMPLE_COUNT_2_BIT};
  for (auto& possibleSampleCount : possibleSampleCounts)
    {
      if (supportedSampleCount & possibleSampleCount)
        {
          return possibleSampleCount;
        }
    }
  return VK_SAMPLE_COUNT_1_BIT;
}

void SngoEngine::Core::Source::RenderPipeline::EngineMSAA_RenderPass::init(
    Device::LogicalDevice::EngineDevice* _device,
    VkExtent2D _extent,
    VkFormat color_format,
    VkSampleCountFlagBits samplers,
    SwapChain::EngineSwapChain* swap_chain,
    bool contaion_gui_subpass,
    const VkAllocationCallbacks* alloc)
{
  extent = _extent;
  device = _device;
  const uint32_t attach_count{3};

  assert((device->pPD->properties.limits.framebufferColorSampleCounts & samplers)
         && (device->pPD->properties.limits.framebufferDepthSampleCounts & samplers));

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
      attachmentDescs[0].finalLayout = VK_IMAGE_LAYOUT_GENERAL;

      attachmentDescs[0].format = color_format;
    }
    // attachment_1 : resolve
    {
      attachmentDescs[1].samples = VK_SAMPLE_COUNT_1_BIT;

      attachmentDescs[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      attachmentDescs[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;

      attachmentDescs[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      attachmentDescs[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

      attachmentDescs[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      attachmentDescs[1].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
      attachmentDescs[1].format = color_format;
    }
    // attachment_2 : depth
    {
      attachmentDescs[2].samples = samplers;

      attachmentDescs[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      attachmentDescs[2].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

      attachmentDescs[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      attachmentDescs[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

      attachmentDescs[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      attachmentDescs[2].finalLayout = VK_IMAGE_LAYOUT_GENERAL;
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
  std::vector<VkSubpassDescription> subpasses{};
  {
    VkSubpassDescription subpass_0{};
    subpass_0.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_0.colorAttachmentCount = 1;
    subpass_0.pColorAttachments = &colorReference;
    subpass_0.pDepthStencilAttachment = &depthReference;
    subpass_0.pResolveAttachments = &resolveReference;
    subpasses.push_back(subpass_0);

    if (contaion_gui_subpass)
      {
        VkSubpassDescription subpass_1{};
        subpass_1.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass_1.colorAttachmentCount = 1;
        subpass_1.pColorAttachments = &colorReference;
        subpass_1.pDepthStencilAttachment = &depthReference;
        subpass_1.pResolveAttachments = &resolveReference;
        subpasses.push_back(subpass_1);
      }
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

    //--------------------------------------------------------------------------------------

    if (contaion_gui_subpass)
      {
        VkSubpassDependency depen{};
        depen.srcSubpass = 0;
        depen.dstSubpass = 1;

        depen.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        depen.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        depen.srcAccessMask = 0;
        depen.dstAccessMask =
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

        depen.dependencyFlags = 0;

        dependencies.push_back(depen);
      }
  }

  // attachments
  std::vector<VkImageView> attachments{attach_count};
  {
    attachments[0] = attchment_Color.view.image_view;

    attachments[2] = attchment_Depth.view.image_view;
  }

  renderpass.init(_device, dependencies, subpasses, attachmentDescs, alloc);

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
}

void SngoEngine::Core::Source::RenderPipeline::EngineMSAA_RenderPass::construct_descriptor(
    Descriptor::EngineDescriptorPool* _pool,
    std::vector<VkDescriptorImageInfo>& imginfos,
    const VkAllocationCallbacks* alloc)
{
  assert(device);

  std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
      Core::Source::Descriptor::GetLayoutBinding(
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
      Core::Source::Descriptor::GetLayoutBinding(
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1)};

  msaa_setlayout.init(device, setLayoutBindings, alloc);
  msaa_set.init(device, &msaa_setlayout, _pool);

  std::vector<VkWriteDescriptorSet> writes{
      Descriptor::GetDescriptSet_Write(
          msaa_set.descriptor_set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &imginfos[0])[0],
      Descriptor::GetDescriptSet_Write(
          msaa_set.descriptor_set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &imginfos[1])[0]};

  msaa_set.updateWrite(writes);
}

void SngoEngine::Core::Source::RenderPipeline::EngineMSAA_RenderPass::construct_pipeline(
    std::vector<VkPipelineShaderStageCreateInfo>& _shader_stage,
    VkSampleCountFlagBits _sampler_flags,
    const VkAllocationCallbacks* alloc)
{
  assert(device);

  pipeline_layout.init(device,
                       std::vector<VkDescriptorSetLayout>{msaa_setlayout.layout},
                       std::vector<VkPushConstantRange>{});

  auto _info{Default_Pipeline(extent, Buffer::Get_EmptyVertexInputState())};

  std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates = {
      Data::GetColorBlend_DEFAULT(),
      Data::GetColorBlend_DEFAULT(),
  };

  _info.color_blend.pAttachments = blendAttachmentStates.data();
  _info.rasterizer.cullMode = VK_CULL_MODE_NONE;
  if (_sampler_flags != VK_SAMPLE_COUNT_1_BIT)
    {
      _info.multisampling = Data::MULTISAMPLING_INFO_ENABLED(_sampler_flags, 0.25f);
    }

  pipeline.init(device, &pipeline_layout, &renderpass, _shader_stage, &_info, 0, alloc);
}

void SngoEngine::Core::Source::RenderPipeline::EngineMSAA_RenderPass::destroyer()
{
  if (device)
    {
      attchment_Color.destroyer();

      attchment_Depth.destroyer();
      framebuffers.destroyer();
      renderpass.destroyer();

      msaa_setlayout.destroyer();
    }
}

//===========================================================================================================================
// EngineShadowMap_RenderPass
//===========================================================================================================================

void SngoEngine::Core::Source::RenderPipeline::EngineShadowMap_RenderPass::init(
    Device::LogicalDevice::EngineDevice* _device,
    VkExtent2D _extent,
    const VkAllocationCallbacks* alloc)

{
  extent = _extent;
  device = _device;

  const uint32_t attach_count{1};
  auto depth_format{Image::Find_DepthFormat(device->pPD->operator()())};

  // attchments initialization
  {
    attchment_Depth.init(_device,
                         Image::Find_DepthFormat(_device->pPD->physical_device),
                         VkImageUsageFlagBits(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
                                              | VK_IMAGE_USAGE_SAMPLED_BIT),
                         extent,
                         VK_SAMPLE_COUNT_1_BIT);
  }

  // Attachment Description
  std::vector<VkAttachmentDescription> attachmentDescs{attach_count};
  {
    attachmentDescs[0].format = depth_format;
    attachmentDescs[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentDescs[0].loadOp =
        VK_ATTACHMENT_LOAD_OP_CLEAR;  // Clear depth at beginning of the render pass
    attachmentDescs[0].storeOp =
        VK_ATTACHMENT_STORE_OP_STORE;  // We will read from depth, so it's important to store the
                                       // depth attachment results
    attachmentDescs[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescs[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescs[0].initialLayout =
        VK_IMAGE_LAYOUT_UNDEFINED;  // We don't care about initial layout of the attachment
    attachmentDescs[0].finalLayout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;  // Attachment will be transitioned to
                                                          // shader read at render pass end
  }

  // color references
  VkAttachmentReference depthReference{};
  {
    depthReference = {0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
  }

  // subpass
  std::vector<VkSubpassDescription> subpasses{};
  {
    VkSubpassDescription subpass_0{};
    subpass_0.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_0.colorAttachmentCount = 0;
    subpass_0.pDepthStencilAttachment = &depthReference;
    subpasses.push_back(subpass_0);
  }

  // attachments
  std::vector<VkImageView> attachments{attach_count};
  {
    attachments[0] = attchment_Depth.view.image_view;
  }
  // dependency
  std::vector<VkSubpassDependency> dependencies{2};
  {
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
  }

  VkFilter shadowmap_filter =
      Image::FormatIs_Filterable(device->pPD->operator()(), depth_format, VK_IMAGE_TILING_OPTIMAL)
          ? VK_FILTER_LINEAR
          : VK_FILTER_NEAREST;
  VkSamplerCreateInfo _sampler_info{};
  {
    _sampler_info.magFilter = shadowmap_filter;
    _sampler_info.minFilter = shadowmap_filter;
    _sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    _sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    _sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    _sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    _sampler_info.mipLodBias = 0.0f;
    _sampler_info.maxAnisotropy = 1.0f;
    _sampler_info.minLod = 0.0f;
    _sampler_info.maxLod = 1.0f;
    _sampler_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
  }

  renderpass.init(device, dependencies, subpasses, attachmentDescs, alloc);
  sampler.init(device, _sampler_info, alloc);
  framebuffer.init(device, attachments, extent, 1, alloc);
}

void SngoEngine::Core::Source::RenderPipeline::EngineShadowMap_RenderPass::construct_descriptor(
    Descriptor::EngineDescriptorPool* _pool,
    VkDescriptorBufferInfo debug_buffer_info,
    VkDescriptorBufferInfo offscreen_buffer_info,
    VkDescriptorBufferInfo scene_buffer_info,
    const VkAllocationCallbacks* alloc)
{
  std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
      // Binding 0 : Vertex shader uniform buffer
      Core::Source::Descriptor::GetLayoutBinding(
          VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
          VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
          0),
      // Binding 1 : Fragment shader image sampler (shadow map)
      Core::Source::Descriptor::GetLayoutBinding(
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1)};

  SM_setlayout.init(device, setLayoutBindings, alloc);
  sets.debug.init(device, &SM_setlayout, _pool);
  sets.offscreen.init(device, &SM_setlayout, _pool);
  sets.scene.init(device, &SM_setlayout, _pool);

  VkDescriptorImageInfo shadowMap_descriptor = {
      sampler(), attchment_Depth.view(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL};

  // debug set
  std::vector<VkWriteDescriptorSet> writes = {
      // Binding 0 : Parameters uniform buffer
      Descriptor::GetDescriptSet_Write(
          sets.debug.descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &debug_buffer_info)[0],
      // Binding 1 : Fragment shader texture sampler
      Descriptor::GetDescriptSet_Write(sets.debug.descriptor_set,
                                       VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                       1,
                                       &shadowMap_descriptor)[0]};
  sets.debug.updateWrite(writes);

  // offscreen set
  writes = {// Binding 0 : Parameters uniform buffer
            Descriptor::GetDescriptSet_Write(sets.offscreen.descriptor_set,
                                             VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                             0,
                                             &offscreen_buffer_info)[0]};
  sets.offscreen.updateWrite(writes);

  // sceneset
  writes = {
      // Binding 0 : Parameters uniform buffer
      Descriptor::GetDescriptSet_Write(
          sets.scene.descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &scene_buffer_info)[0],
      // Binding 1 : Fragment shader texture sampler
      Descriptor::GetDescriptSet_Write(sets.scene.descriptor_set,
                                       VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                       1,
                                       &shadowMap_descriptor)[0]};
  sets.scene.updateWrite(writes);
}

void SngoEngine::Core::Source::RenderPipeline::EngineShadowMap_RenderPass::construct_pipeline(
    std::vector<VkPipelineShaderStageCreateInfo>& _debug_stage,
    std::vector<VkPipelineShaderStageCreateInfo>& _scene_stage,
    std::vector<VkPipelineShaderStageCreateInfo>& _offscreen_stage,
    VkPipelineVertexInputStateCreateInfo v_input,
    const VkAllocationCallbacks* alloc)
{
  assert(device);

  auto pipelie_info{ShadowMapping_Pipeline(extent, Buffer::Get_EmptyVertexInputState())};

  // debug pipeline
  pipelie_info.rasterizer.cullMode = VK_CULL_MODE_NONE;
  pipelines.Debug.init(
      device, &pipeline_layout, &renderpass, _debug_stage, &pipelie_info, 0, alloc);

  // PCF pipeline
  VkSpecializationMapEntry specMapEntry{Pipeline::Get_SpecMapEntry(0, 0, sizeof(uint32_t))};
  uint32_t use_PCF{1};
  VkSpecializationInfo spec_info{
      Pipeline::Get_SpecializationInfo(1, &specMapEntry, sizeof(use_PCF), &use_PCF)};
  pipelie_info.vertex_input = v_input;
  _scene_stage[1].pSpecializationInfo = &spec_info;

  pipelie_info.rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  pipelines.PCF.init(device, &pipeline_layout, &renderpass, _debug_stage, &pipelie_info, 0, alloc);

  // NoPCF pipeline
  use_PCF = 0;
  pipelines.NoPCF.init(
      device, &pipeline_layout, &renderpass, _debug_stage, &pipelie_info, 0, alloc);

  // Offscreen pipeline
  assert(_offscreen_stage.size() == 1);

  pipelie_info.color_blend.attachmentCount = 0;
  pipelie_info.rasterizer.cullMode = VK_CULL_MODE_NONE;
  pipelie_info.depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
  pipelie_info.rasterizer.depthBiasEnable = VK_TRUE;
  pipelie_info.dynamic_states.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);
  pipelie_info.dynamic_state = Core::Data::GetDynamicState_Info(pipelie_info.dynamic_states);

  pipelines.Offscreen.init(
      device, &pipeline_layout, &renderpass, _offscreen_stage, &pipelie_info, 0, alloc);
}
