#ifndef __SNGO_PIPELINE_H
#define __SNGO_PIPELINE_H

#include <cstdint>
#include <string>

#include "src/Core/Data.h"
#include "src/Core/Device/LogicalDevice.hpp"
#include "src/Core/Render/RenderPass.hpp"
#include "vulkan/vulkan_core.h"

namespace SngoEngine::Core::Source::Pipeline
{
VkPipelineShaderStageCreateInfo Get_VertexShader_CreateInfo(
    const Device::LogicalDevice::EngineDevice* device,
    const std::string& _pName,
    const std::string& code_file);
VkPipelineShaderStageCreateInfo Get_VertexShader_CreateInfo(const std::string& _pName,
                                                            const VkShaderModule& compiled_code);

VkPipelineShaderStageCreateInfo Get_FragmentShader_CreateInfo(
    const Device::LogicalDevice::EngineDevice* device,
    const std::string& _pName,
    const std::string& code_file);
VkPipelineShaderStageCreateInfo Get_FragmentShader_CreateInfo(const std::string& _pName,
                                                              const VkShaderModule& compiled_code);

//===========================================================================================================================
// EnginePipelineLayout
//===========================================================================================================================

struct EnginePipelineLayout
{
  EnginePipelineLayout() = default;
  EnginePipelineLayout(EnginePipelineLayout&&) noexcept = default;
  EnginePipelineLayout& operator=(EnginePipelineLayout&&) noexcept = default;
  template <typename... Args>
  explicit EnginePipelineLayout(const Device::LogicalDevice::EngineDevice* _device, Args... args)
  {
    creator(_device, args...);
  }
  template <typename... Args>
  void operator()(const Device::LogicalDevice::EngineDevice* _device, Args... args)
  {
    creator(_device, args...);
  }
  ~EnginePipelineLayout()
  {
    destroyer();
  }
  void destroyer();

  VkPipelineLayout pipeline_layout{};
  const Device::LogicalDevice::EngineDevice* device{};

 private:
  void creator(const Device::LogicalDevice::EngineDevice* _device,
               const std::vector<VkDescriptorSetLayout>& descriptor_set_layout,
               const std::vector<VkPushConstantRange>& push_constant_range,
               const VkAllocationCallbacks* alloc = nullptr);
  const VkAllocationCallbacks* Alloc{};
};

//===========================================================================================================================
// EngineGraphicPipeline
//===========================================================================================================================

struct EngineGraphicPipeline
{
  EngineGraphicPipeline() = default;
  EngineGraphicPipeline(EngineGraphicPipeline&&) noexcept = default;
  EngineGraphicPipeline& operator=(EngineGraphicPipeline&&) noexcept = default;
  template <typename... Args>
  EngineGraphicPipeline(const Device::LogicalDevice::EngineDevice* _device,
                        const EnginePipelineLayout* layout,
                        Args... args)
  {
    creator(_device, layout, args...);
  }
  template <typename... Args>
  void init(const Device::LogicalDevice::EngineDevice* _device,
            const EnginePipelineLayout* layout,
            Args... args)
  {
    creator(_device, layout, args...);
  }
  ~EngineGraphicPipeline()
  {
    destroyer();
  }
  void destroyer();

  VkPipeline pipeline{};
  Render::RenderPass::EngineRenderPass* render_pass{};
  const Device::LogicalDevice::EngineDevice* device{};

 private:
  void creator(const Device::LogicalDevice::EngineDevice* _device,
               const EnginePipelineLayout* layout,
               Render::RenderPass::EngineRenderPass* _render_pass,
               const std::vector<VkPipelineShaderStageCreateInfo>& shader_stages,
               const Data::PipelinePreparation_Info* _info,
               uint32_t _subpass,
               const VkAllocationCallbacks* alloc = nullptr);
  const VkAllocationCallbacks* Alloc{};
};

}  // namespace SngoEngine::Core::Source::Pipeline

#endif