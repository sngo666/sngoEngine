#include "Pipeline.hpp"

#include <glslang/Public/ShaderLang.h>
#include <vulkan/vulkan_core.h>

#include "src/Core/Device/LogicalDevice.hpp"
#include "src/Core/Render/RenderPass.hpp"
#include "src/Core/Utils/Utils.hpp"

VkPipelineShaderStageCreateInfo SngoEngine::Core::Source::Pipeline::Get_VertexShader_CreateInfo(
    const Device::LogicalDevice::EngineDevice* device,
    const std::string& _pName,
    const std::string& code_file)
{
  std::string code{Utils::read_file(code_file).data()};
  auto vertex_shader_module{
      Utils::Glsl_ShaderCompiler(device->logical_device, EShLangVertex, code)};

  VkPipelineShaderStageCreateInfo vertex_create_info{};
  vertex_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertex_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertex_create_info.module = vertex_shader_module;
  vertex_create_info.pName = _pName.c_str();

  return vertex_create_info;
}

VkPipelineShaderStageCreateInfo SngoEngine::Core::Source::Pipeline::Get_VertexShader_CreateInfo(
    const std::string& _pName,
    const VkShaderModule& compiled_code)
{
  VkPipelineShaderStageCreateInfo vertex_create_info{};
  vertex_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertex_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertex_create_info.module = compiled_code;
  vertex_create_info.pName = _pName.c_str();

  return vertex_create_info;
}

VkPipelineShaderStageCreateInfo SngoEngine::Core::Source::Pipeline::Get_FragmentShader_CreateInfo(
    const Device::LogicalDevice::EngineDevice* device,
    const std::string& _pName,
    const std::string& code_file)
{
  std::string code{Utils::read_file(code_file).data()};
  auto fragment_shader_module{
      Utils::Glsl_ShaderCompiler(device->logical_device, EShLangFragment, code)};

  VkPipelineShaderStageCreateInfo fragment_create_info{};
  fragment_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragment_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragment_create_info.module = fragment_shader_module;
  fragment_create_info.pName = _pName.c_str();

  return fragment_create_info;
}

VkPipelineShaderStageCreateInfo SngoEngine::Core::Source::Pipeline::Get_FragmentShader_CreateInfo(
    const std::string& _pName,
    const VkShaderModule& compiled_code)
{
  VkPipelineShaderStageCreateInfo fragment_create_info{};
  fragment_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragment_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragment_create_info.module = compiled_code;
  fragment_create_info.pName = _pName.c_str();

  return fragment_create_info;
}

void SngoEngine::Core::Source::Pipeline::EnginePipelineLayout::creator(
    const Device::LogicalDevice::EngineDevice* _device,
    const std::vector<VkDescriptorSetLayout>& descriptor_set_layout,
    const std::vector<VkPushConstantRange>& push_constant_range,
    const VkAllocationCallbacks* alloc)
{
  destroyer();
  device = _device;
  Alloc = alloc;

  VkPipelineLayoutCreateInfo pipeline_layout_info{};
  pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipeline_layout_info.setLayoutCount = descriptor_set_layout.size();
  pipeline_layout_info.pSetLayouts = descriptor_set_layout.data();

  pipeline_layout_info.pushConstantRangeCount = push_constant_range.size();
  if (!push_constant_range.empty())
    pipeline_layout_info.pPushConstantRanges = push_constant_range.data();

  if (vkCreatePipelineLayout(device->logical_device, &pipeline_layout_info, Alloc, &pipeline_layout)
      != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create pipeline layout!");
    }
}

void SngoEngine::Core::Source::Pipeline::EnginePipelineLayout::destroyer()
{
  if (pipeline_layout != VK_NULL_HANDLE)
    vkDestroyPipelineLayout(device->logical_device, pipeline_layout, Alloc);
}

void SngoEngine::Core::Source::Pipeline::EngineGraphicPipeline::creator(
    const Device::LogicalDevice::EngineDevice* _device,
    const EnginePipelineLayout* layout,
    Render::RenderPass::EngineRenderPass* _render_pass,
    const std::vector<VkPipelineShaderStageCreateInfo>& shader_stages,
    const Data::PipelinePreparation_Info* _info,
    uint32_t _subpass,
    const VkAllocationCallbacks* alloc)
{
  if (pipeline != VK_NULL_HANDLE)
    vkDestroyPipeline(device->logical_device, pipeline, Alloc);
  Alloc = alloc;
  device = _device;
  render_pass = _render_pass;

  VkGraphicsPipelineCreateInfo graphic_pipeline_create_info{};
  graphic_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  graphic_pipeline_create_info.stageCount = shader_stages.size();
  graphic_pipeline_create_info.pStages = shader_stages.data();
  graphic_pipeline_create_info.pVertexInputState = &_info->vertex_input;
  graphic_pipeline_create_info.pInputAssemblyState = &_info->input_assembly;
  graphic_pipeline_create_info.pViewportState = &_info->view_state;
  graphic_pipeline_create_info.pRasterizationState = &_info->rasterizer;
  graphic_pipeline_create_info.pMultisampleState = &_info->multisampling;
  graphic_pipeline_create_info.pDepthStencilState = &_info->depth_stencil;
  graphic_pipeline_create_info.pColorBlendState = &_info->color_blend;
  graphic_pipeline_create_info.pDynamicState = &_info->dynamic_state;
  graphic_pipeline_create_info.layout = layout->pipeline_layout;
  graphic_pipeline_create_info.renderPass = render_pass->render_pass;
  graphic_pipeline_create_info.subpass = _subpass;
  graphic_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
  graphic_pipeline_create_info.basePipelineIndex = -1;

  if (vkCreateGraphicsPipelines(device->logical_device,
                                VK_NULL_HANDLE,
                                1,
                                &graphic_pipeline_create_info,
                                Alloc,
                                &pipeline)
      != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create graphic pipeline!");
    }
}

void SngoEngine::Core::Source::Pipeline::EngineGraphicPipeline::destroyer()
{
  if (pipeline != VK_NULL_HANDLE)
    vkDestroyPipeline(device->logical_device, pipeline, Alloc);
}