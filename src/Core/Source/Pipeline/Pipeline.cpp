#include "Pipeline.hpp"

#include <glslang/Public/ShaderLang.h>
#include <vulkan/vulkan_core.h>

#include "src/Core/Data.h"
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

VkSpecializationMapEntry SngoEngine::Core::Source::Pipeline::Get_SpecMapEntry(uint32_t constantID,
                                                                              uint32_t offset,
                                                                              size_t size)
{
  VkSpecializationMapEntry specializationMapEntry{};
  specializationMapEntry.constantID = constantID;
  specializationMapEntry.offset = offset;
  specializationMapEntry.size = size;
  return specializationMapEntry;
}

VkSpecializationInfo SngoEngine::Core::Source::Pipeline::Get_SpecializationInfo(
    uint32_t mapEntryCount,
    const VkSpecializationMapEntry* mapEntries,
    size_t dataSize,
    const void* data)
{
  VkSpecializationInfo specializationInfo{};
  specializationInfo.mapEntryCount = mapEntryCount;
  specializationInfo.pMapEntries = mapEntries;
  specializationInfo.dataSize = dataSize;
  specializationInfo.pData = data;
  return specializationInfo;
}

VkSpecializationInfo SngoEngine::Core::Source::Pipeline::Get_SpecializationInfo(
    const std::vector<VkSpecializationMapEntry>& mapEntries,
    size_t dataSize,
    const void* data)
{
  VkSpecializationInfo specializationInfo{};
  specializationInfo.mapEntryCount = static_cast<uint32_t>(mapEntries.size());
  specializationInfo.pMapEntries = mapEntries.data();
  specializationInfo.dataSize = dataSize;
  specializationInfo.pData = data;
  return specializationInfo;
}

//===========================================================================================================================
// EnginePipelineLayout
//===========================================================================================================================
SngoEngine::Core::Source::Pipeline::EngineShaderStage::EngineShaderStage(
    const Device::LogicalDevice::EngineDevice* _device,
    const std::string& _vert_file,
    const std::string& _frag_file,
    const std::string& _vert_name,
    const std::string& _frag_name)
{
  vert_name = _vert_name;
  frag_name = _frag_name;

  std::string vert_code{Utils::read_file(_vert_file).data()};
  vertex_shader_module = {
      Utils::Glsl_ShaderCompiler(_device->logical_device, EShLangVertex, vert_code)};
  stages[0] = {Source::Pipeline::Get_VertexShader_CreateInfo(vert_name, vertex_shader_module)};

  std::string frag_code{Utils::read_file(_frag_file).data()};
  fragment_shader_module = {
      Utils::Glsl_ShaderCompiler(_device->logical_device, EShLangFragment, frag_code)};
  stages[1] = {Source::Pipeline::Get_FragmentShader_CreateInfo(frag_name, fragment_shader_module)};
}
//===========================================================================================================================
// EnginePipelineLayout
//===========================================================================================================================

void SngoEngine::Core::Source::Pipeline::EnginePipelineLayout::creator(
    const Device::LogicalDevice::EngineDevice* _device,
    const std::vector<VkDescriptorSetLayout>& _layouts,
    const std::vector<VkPushConstantRange>& push_constant_range,
    const VkAllocationCallbacks* alloc)
{
  destroyer();
  device = _device;
  Alloc = alloc;

  VkPipelineLayoutCreateInfo pipeline_layout_info{};
  pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipeline_layout_info.setLayoutCount = _layouts.size();
  pipeline_layout_info.pSetLayouts = _layouts.data();

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
  destroyer();
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

void SngoEngine::Core::Source::Pipeline::EngineGraphicPipeline::creator(
    const Device::LogicalDevice::EngineDevice* _device,
    std::vector<VkGraphicsPipelineCreateInfo>& _infos,
    VkPipelineCache _cahce,
    const VkAllocationCallbacks* alloc)
{
  destroyer();
  Alloc = alloc;
  device = _device;

  if (vkCreateGraphicsPipelines(
          device->logical_device, _cahce, _infos.size(), _infos.data(), Alloc, &pipeline)
      != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create graphic pipeline!");
    }
}

void SngoEngine::Core::Source::Pipeline::EngineGraphicPipeline::creator(
    const Device::LogicalDevice::EngineDevice* _device,
    VkGraphicsPipelineCreateInfo _info,
    VkPipelineCache _cahce,
    const VkAllocationCallbacks* alloc)
{
  destroyer();
  Alloc = alloc;
  device = _device;

  if (vkCreateGraphicsPipelines(device->logical_device, _cahce, 1, &_info, Alloc, &pipeline)
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