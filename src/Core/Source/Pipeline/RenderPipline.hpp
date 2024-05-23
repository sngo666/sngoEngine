#ifndef __SNGO_RENDERPIPELINE_H
#define __SNGO_RENDERPIPELINE_H

#include <vulkan/vulkan_core.h>

#include "src/Core/Device/LogicalDevice.hpp"
#include "src/Core/Render/FrameBuffer.hpp"
#include "src/Core/Render/RenderPass.hpp"
#include "src/Core/Source/Buffer/Descriptor.hpp"
#include "src/Core/Source/Image/Image.hpp"
#include "src/Core/Source/Image/ImageVIew.hpp"
#include "src/Core/Source/Image/Sampler.hpp"
#include "src/Core/Source/Pipeline/Pipeline.hpp"
#include "src/Core/Source/SwapChain/SwapChain.hpp"

namespace SngoEngine::Core::Source::RenderPipeline
{

//===========================================================================================================================
// pipeline info function
//===========================================================================================================================

Core::Data::PipelinePreparation_Info Skybox_Pipeline(VkExtent2D _extent,
                                                     VkPipelineVertexInputStateCreateInfo v_input);

//===========================================================================================================================
// EngineFrameBufferAttachment
//===========================================================================================================================

struct EngineFrameBufferAttachment
{
  Image::EngineImage img;
  ImageView::EngineImageView view;
  VkFormat format{VK_FORMAT_UNDEFINED};

  EngineFrameBufferAttachment() = default;
  void init(Device::LogicalDevice::EngineDevice* _device,
            VkFormat _format,
            VkImageUsageFlagBits _usage,
            VkExtent2D _extent,
            VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT);
};

//===========================================================================================================================
// EngineOffscreenHDR_RenderPass
//===========================================================================================================================

struct EngineOffscreenHDR_RenderPass
{
  EngineFrameBufferAttachment attchment_FloatingPoint_0{};
  EngineFrameBufferAttachment attchment_FloatingPoint_1{};
  EngineFrameBufferAttachment attchment_Depth{};

  Render::EngineFrameBuffer framebuffer{};
  Render::RenderPass::EngineRenderPass renderpass{};
  Image::EngineSampler sampler{};

  VkExtent2D extent{};

  void init(Device::LogicalDevice::EngineDevice* _device,
            VkExtent2D _extent,
            const VkAllocationCallbacks* alloc = nullptr);
};

//===========================================================================================================================
// EngineBloomFilter_RenderPass
//===========================================================================================================================

struct EngineBloomFilter_RenderPass
{
  EngineFrameBufferAttachment attchment_FloatingPoint{};

  Render::EngineFrameBuffer framebuffer{};
  Render::RenderPass::EngineRenderPass renderpass{};
  Image::EngineSampler sampler{};

  VkExtent2D extent{};

  void init(Device::LogicalDevice::EngineDevice* _device,
            VkExtent2D _extent,
            const VkAllocationCallbacks* alloc = nullptr);
};

//===========================================================================================================================
// EngineMSAA_RenderPass
//===========================================================================================================================

struct EngineMSAA_RenderPass
{
  EngineFrameBufferAttachment attchment_Color{};
  EngineFrameBufferAttachment attchment_Depth{};

  Render::EngineFrameBuffers framebuffers{};
  Render::RenderPass::EngineRenderPass renderpass{};

  VkExtent2D extent{};

  void init(Device::LogicalDevice::EngineDevice* _device,
            VkExtent2D _extent,
            VkFormat color_format,
            VkSampleCountFlagBits samplers,
            SwapChain::EngineSwapChain* swap_chain,
            const VkAllocationCallbacks* alloc = nullptr);
};

}  // namespace SngoEngine::Core::Source::RenderPipeline

#endif