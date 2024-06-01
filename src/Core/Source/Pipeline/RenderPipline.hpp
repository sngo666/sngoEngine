#ifndef __SNGO_RENDERPIPELINE_H
#define __SNGO_RENDERPIPELINE_H

#include <vulkan/vulkan_core.h>

#include <vector>

#include "src/Core/Device/LogicalDevice.hpp"
#include "src/Core/Render/FrameBuffer.hpp"
#include "src/Core/Render/RenderPass.hpp"
#include "src/Core/Source/Buffer/CommandBuffer.hpp"
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

Core::Data::PipelinePreparation_Info Default_Pipeline(VkExtent2D _extent,
                                                      VkPipelineVertexInputStateCreateInfo v_input);

Core::Data::PipelinePreparation_Info Skybox_Pipeline(VkExtent2D _extent,
                                                     VkPipelineVertexInputStateCreateInfo v_input);

Core::Data::PipelinePreparation_Info Bloomfilter_Pipeline(
    VkExtent2D _extent,
    VkPipelineVertexInputStateCreateInfo v_input);

SngoEngine::Core::Data::PipelinePreparation_Info ShadowMapping_Pipeline(
    VkExtent2D _extent,
    VkPipelineVertexInputStateCreateInfo v_input);

//===========================================================================================================================
// EngineFrameBufferAttachment
//===========================================================================================================================

struct EngineFrameBufferAttachment
{
  Image::EngineImage img;
  ImageView::EngineImageView view;
  VkFormat format{VK_FORMAT_UNDEFINED};
  const Device::LogicalDevice::EngineDevice* device{};

  EngineFrameBufferAttachment() = default;
  ~EngineFrameBufferAttachment()
  {
    destroyer();
  }

  void init(const Device::LogicalDevice::EngineDevice* _device,
            VkFormat _format,
            VkImageUsageFlagBits _usage,
            VkExtent2D _extent,
            VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT,
            uint32_t _mipmap = 1,
            uint32_t _arraylayers = 1,
            VkImageCreateFlags _flags = 0);
  void destroyer();
};

//===========================================================================================================================
// EngineOffscreenHDR_RenderPass
//===========================================================================================================================

struct EngineOffscreenHDR_RenderPass
{
  EngineFrameBufferAttachment attchment_FloatingPoint_0{};
  EngineFrameBufferAttachment attchment_FloatingPoint_1{};
  EngineFrameBufferAttachment attchment_Depth{};
  EngineFrameBufferAttachment attchment_Resolve_0{};
  EngineFrameBufferAttachment attchment_Resolve_1{};

  Render::EngineFrameBuffer framebuffer{};
  Render::RenderPass::EngineRenderPass renderpass{};
  Image::EngineSampler sampler{};

  const Device::LogicalDevice::EngineDevice* device;

  VkExtent2D extent{};

  void init(const Device::LogicalDevice::EngineDevice* _device,
            VkExtent2D _extent,
            VkSampleCountFlagBits samplers,
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

  Core::Source::Descriptor::EngineDescriptorSetLayout bloom_setlayout;
  Core::Source::Descriptor::EngineDescriptorSet bloom_set;

  Pipeline::EnginePipelineLayout pipeline_layout;
  std::vector<Pipeline::EngineGraphicPipeline> pipelines{2};

  const Device::LogicalDevice::EngineDevice* device{};

  VkExtent2D extent{};

  void destroyer();

  void init(const Device::LogicalDevice::EngineDevice* _device,
            VkExtent2D _extent,
            const VkAllocationCallbacks* alloc = nullptr);
  void construct_descriptor(Descriptor::EngineDescriptorPool* _pool,
                            std::vector<VkDescriptorImageInfo>& imginfos,
                            const VkAllocationCallbacks* alloc = nullptr);
  void construct_pipeline(std::vector<VkPipelineShaderStageCreateInfo>& _shader_stage,
                          Pipeline::EnginePipelineLayout* _layout,
                          Render::RenderPass::EngineRenderPass* _render_pass,
                          VkSampleCountFlagBits _sampler_flag,
                          const VkAllocationCallbacks* alloc = nullptr);
  ~EngineBloomFilter_RenderPass()
  {
    destroyer();
  }
};

//===========================================================================================================================
// EngineMSAA_RenderPass
//===========================================================================================================================

VkSampleCountFlagBits MSAA_maxAvailableSampleCount(Device::LogicalDevice::EngineDevice* _device);

struct EngineMSAA_RenderPass
{
  EngineFrameBufferAttachment attchment_Color{};
  EngineFrameBufferAttachment attchment_Depth{};

  Render::EngineFrameBuffers framebuffers{};
  Render::RenderPass::EngineRenderPass renderpass{};

  Core::Source::Descriptor::EngineDescriptorSetLayout msaa_setlayout;
  Core::Source::Descriptor::EngineDescriptorSet msaa_set;

  Pipeline::EnginePipelineLayout pipeline_layout;
  Pipeline::EngineGraphicPipeline pipeline;

  VkExtent2D extent{};

  Device::LogicalDevice::EngineDevice* device;

  void init(Device::LogicalDevice::EngineDevice* _device,
            VkExtent2D _extent,
            VkFormat color_format,
            VkSampleCountFlagBits samplers,
            SwapChain::EngineSwapChain* swap_chain,
            bool contaion_gui_subpass = true,
            const VkAllocationCallbacks* alloc = nullptr);
  void destroyer();

  void construct_descriptor(Descriptor::EngineDescriptorPool* _pool,
                            std::vector<VkDescriptorImageInfo>& imginfos,
                            const VkAllocationCallbacks* alloc = nullptr);

  void construct_pipeline(std::vector<VkPipelineShaderStageCreateInfo>& _shader_stage,
                          VkSampleCountFlagBits _sampler_flags,
                          const VkAllocationCallbacks* alloc = nullptr);

  ~EngineMSAA_RenderPass()
  {
    destroyer();
  }
};

//===========================================================================================================================
// EngineShadowMap_RenderPass
//===========================================================================================================================

struct EngineShadowMap_RenderPass
{
  EngineFrameBufferAttachment attchment_Depth{};

  Device::LogicalDevice::EngineDevice* device;
  Render::EngineFrameBuffer framebuffer{};
  Render::RenderPass::EngineRenderPass renderpass{};
  Image::EngineSampler sampler{};

  VkDescriptorImageInfo shadowMap_descriptor;

  Core::Source::Descriptor::EngineDescriptorSetLayout SM_setlayout;
  struct
  {
    Core::Source::Descriptor::EngineDescriptorSet debug;
    Core::Source::Descriptor::EngineDescriptorSet offscreen;
    Core::Source::Descriptor::EngineDescriptorSet scene;
  } sets;

  Pipeline::EnginePipelineLayout pipeline_layout;

  struct
  {
    Pipeline::EngineGraphicPipeline Debug;
    Pipeline::EngineGraphicPipeline PCF;
    Pipeline::EngineGraphicPipeline NoPCF;
    Pipeline::EngineGraphicPipeline Offscreen;
  } pipelines;

  VkExtent2D extent{};

  void destroyer();

  void init(Device::LogicalDevice::EngineDevice* _device,
            VkExtent2D _extent,
            const VkAllocationCallbacks* alloc = nullptr);
  void construct_descriptor(Descriptor::EngineDescriptorPool* _pool,
                            VkDescriptorBufferInfo debug_buffer_info,
                            VkDescriptorBufferInfo offscreen_buffer_info,
                            VkDescriptorBufferInfo scene_buffer_info,
                            const VkAllocationCallbacks* alloc = nullptr);
  void construct_pipeline(std::vector<VkPipelineShaderStageCreateInfo>& _debug_stage,
                          std::vector<VkPipelineShaderStageCreateInfo>& _scene_stage,
                          std::vector<VkPipelineShaderStageCreateInfo>& _offscreen_stage,
                          VkPipelineVertexInputStateCreateInfo v_input,
                          const VkAllocationCallbacks* alloc = nullptr);
};

//===========================================================================================================================
// EngineBRDF_RenderPass
//===========================================================================================================================

struct EngineBRDF_RenderPass
{
  uint32_t dim;
  Buffer::EngineCommandPool cmd_pool;

  EngineFrameBufferAttachment attachment_lutBRDF{};
  Image::EngineSampler sampler{};

  Render::RenderPass::EngineRenderPass renderpass{};
  Render::EngineFrameBuffer framebuffer{};

  Core::Source::Descriptor::EngineDescriptorSetLayout setlayout;
  Core::Source::Descriptor::EngineDescriptorSet set;

  Pipeline::EnginePipelineLayout pipeline_layout;
  Pipeline::EngineGraphicPipeline pipeline;

  Device::LogicalDevice::EngineDevice* device;
  VkExtent2D extent{};

  void init(Device::LogicalDevice::EngineDevice* _device,
            VkExtent2D _extent,
            const VkAllocationCallbacks* alloc = nullptr);

  void construct_decriptor(Descriptor::EngineDescriptorPool* _pool,
                           const VkAllocationCallbacks* alloc = nullptr);

  void construct_pipeline(std::vector<VkPipelineShaderStageCreateInfo>& _stage,
                          VkPipelineVertexInputStateCreateInfo v_input,
                          const VkAllocationCallbacks* alloc = nullptr);

  void render();

  void destroyer();
};

//===========================================================================================================================
// EngineBRDF_RenderPass
//===========================================================================================================================

struct Block_IrradianceCube
{
  glm::mat4 mvp{};
  // Sampling deltas
  float deltaPhi = (2.0f * float(M_PI)) / 180.0f;
  float deltaTheta = (0.5f * float(M_PI)) / 64.0f;
};

struct EngineIrradianceCube_RenderPass
{
  uint32_t dim;
  uint32_t numMips;

  EngineFrameBufferAttachment attachment_CubeMap{};
  EngineFrameBufferAttachment attachment_Offscreen{};

  VkDescriptorImageInfo CubeMap_descriptor;

  Buffer::EngineCommandPool cmd_pool;

  Image::EngineSampler sampler{};

  Render::RenderPass::EngineRenderPass renderpass{};
  // Render::EngineFrameBuffer framebuffer{};
  Render::EngineFrameBuffer framebuffer_offscreen{};

  Core::Source::Descriptor::EngineDescriptorSetLayout setlayout;
  Core::Source::Descriptor::EngineDescriptorSet set;

  Pipeline::EnginePipelineLayout pipeline_layout;
  Pipeline::EngineGraphicPipeline pipeline;

  VkExtent2D extent{};
  Device::LogicalDevice::EngineDevice* device;

  void init(Device::LogicalDevice::EngineDevice* _device,
            VkExtent2D _extent,
            const VkAllocationCallbacks* alloc = nullptr);

  void construct_decriptor(Descriptor::EngineDescriptorPool* _pool,
                           VkDescriptorImageInfo _img_descriptor,
                           const VkAllocationCallbacks* alloc = nullptr);

  void construct_pipeline(std::vector<VkPipelineShaderStageCreateInfo>& _stage,
                          VkPipelineVertexInputStateCreateInfo v_input,
                          const VkAllocationCallbacks* alloc = nullptr);

  void render(void draw_fn(void));
};

}  // namespace SngoEngine::Core::Source::RenderPipeline

#endif