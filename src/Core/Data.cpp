#include "Data.h"

#include <vulkan/vulkan_core.h>

#include <memory>
#include <stdexcept>
#include <vector>

VkPipelineVertexInputStateCreateInfo SngoEngine::Core::Data::GetVertexInput_Info(
    const std::vector<VkVertexInputBindingDescription>& bindings,
    const std::vector<VkVertexInputAttributeDescription>& attributes)
{
  VkPipelineVertexInputStateCreateInfo vertex_input_info{};
  vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertex_input_info.vertexAttributeDescriptionCount = attributes.size();
  vertex_input_info.vertexBindingDescriptionCount = bindings.size();
  vertex_input_info.pVertexAttributeDescriptions = attributes.data();
  vertex_input_info.pVertexBindingDescriptions = bindings.data();

  return vertex_input_info;
}

VkPipelineInputAssemblyStateCreateInfo SngoEngine::Core::Data::GetInputAssembly_Info()
{
  VkPipelineInputAssemblyStateCreateInfo input_assembly{};
  input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  input_assembly.primitiveRestartEnable = VK_FALSE;

  return input_assembly;
}

VkPipelineViewportStateCreateInfo SngoEngine::Core::Data::GetViewportState_Info(VkExtent2D _extent)
{
  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(_extent.width);
  viewport.height = static_cast<float>(_extent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = _extent;

  VkPipelineViewportStateCreateInfo viewport_state_info{};
  viewport_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewport_state_info.viewportCount = 1;
  viewport_state_info.scissorCount = 1;
  viewport_state_info.pViewports = &viewport;
  viewport_state_info.pScissors = &scissor;

  return viewport_state_info;
}

VkPipelineDynamicStateCreateInfo SngoEngine::Core::Data::GetDynamicState_Info(
    const std::vector<VkDynamicState>& dynamic_states)
{
  VkPipelineDynamicStateCreateInfo dynamic_state_info{};
  dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamic_state_info.dynamicStateCount = dynamic_states.size();
  if (!dynamic_states.empty())
    dynamic_state_info.pDynamicStates = dynamic_states.data();

  return dynamic_state_info;
}

VkPipelineRasterizationStateCreateInfo SngoEngine::Core::Data::DEFAULT_RASTERIZER_INFO()
{
  VkPipelineRasterizationStateCreateInfo rasterizer_info{};

  rasterizer_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer_info.depthClampEnable = VK_FALSE;
  rasterizer_info.rasterizerDiscardEnable = VK_FALSE;
  rasterizer_info.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer_info.lineWidth = 1.0f;
  rasterizer_info.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizer_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizer_info.depthBiasEnable = VK_FALSE;
  rasterizer_info.depthBiasConstantFactor = 0.0f;
  rasterizer_info.depthBiasSlopeFactor = 0.0f;
  rasterizer_info.depthBiasClamp = 0.0f;

  return rasterizer_info;
}

VkPipelineMultisampleStateCreateInfo SngoEngine::Core::Data::MULTISAMPLING_INFO_DEFAULT()
{
  VkPipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampling.minSampleShading = 1.0f;
  multisampling.pSampleMask = nullptr;
  multisampling.alphaToCoverageEnable = VK_FALSE;
  multisampling.alphaToOneEnable = VK_FALSE;
  multisampling.flags = 0;

  return multisampling;
}

VkPipelineMultisampleStateCreateInfo SngoEngine::Core::Data::MULTISAMPLING_INFO_ENABLED(
    VkSampleCountFlagBits _samplecount,
    float _minsampleshading)
{
  VkPipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_TRUE;
  multisampling.rasterizationSamples = _samplecount;
  multisampling.minSampleShading = _minsampleshading;
  multisampling.pSampleMask = nullptr;
  multisampling.alphaToCoverageEnable = VK_FALSE;
  multisampling.alphaToOneEnable = VK_FALSE;

  return multisampling;
}

VkPipelineDepthStencilStateCreateInfo SngoEngine::Core::Data::DEFAULT_DEPTHSTENCIL_INFO()
{
  VkPipelineDepthStencilStateCreateInfo depth_stencil_info{};
  depth_stencil_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depth_stencil_info.depthTestEnable = VK_TRUE;
  depth_stencil_info.depthWriteEnable = VK_TRUE;
  depth_stencil_info.depthCompareOp = VK_COMPARE_OP_LESS;
  depth_stencil_info.depthBoundsTestEnable = VK_FALSE;
  depth_stencil_info.minDepthBounds = 0.0f;
  depth_stencil_info.maxDepthBounds = 1.0f;
  depth_stencil_info.stencilTestEnable = VK_FALSE;

  return depth_stencil_info;
}

VkPipelineDepthStencilStateCreateInfo SngoEngine::Core::Data::DEFAULT_DEPTHSTENCIL_INFO(
    VkCompareOp compare_op)
{
  VkPipelineDepthStencilStateCreateInfo depth_stencil_info{};
  depth_stencil_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depth_stencil_info.depthTestEnable = VK_TRUE;
  depth_stencil_info.depthWriteEnable = VK_TRUE;
  depth_stencil_info.depthCompareOp = compare_op;
  depth_stencil_info.depthBoundsTestEnable = VK_FALSE;
  depth_stencil_info.minDepthBounds = 0.0f;
  depth_stencil_info.maxDepthBounds = 1.0f;
  depth_stencil_info.stencilTestEnable = VK_FALSE;

  return depth_stencil_info;
}

VkPipelineDepthStencilStateCreateInfo SngoEngine::Core::Data::DEFAULT_DEPTHSTENCIL_DISABLED_INFO()
{
  VkPipelineDepthStencilStateCreateInfo depth_stencil_info{};
  depth_stencil_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depth_stencil_info.depthTestEnable = VK_FALSE;
  depth_stencil_info.depthWriteEnable = VK_FALSE;
  depth_stencil_info.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
  depth_stencil_info.depthBoundsTestEnable = VK_FALSE;
  depth_stencil_info.minDepthBounds = 0.0f;
  depth_stencil_info.maxDepthBounds = 1.0f;
  depth_stencil_info.stencilTestEnable = VK_FALSE;

  return depth_stencil_info;
}

VkPipelineColorBlendAttachmentState SngoEngine::Core::Data::GetColorBlend_DEFAULT()
{
  VkPipelineColorBlendAttachmentState color_blend_attachment_info{};
  color_blend_attachment_info.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                                               | VK_COLOR_COMPONENT_B_BIT
                                               | VK_COLOR_COMPONENT_A_BIT;
  color_blend_attachment_info.blendEnable = VK_FALSE;

  color_blend_attachment_info.colorBlendOp = VK_BLEND_OP_ADD;
  color_blend_attachment_info.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
  color_blend_attachment_info.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;

  color_blend_attachment_info.alphaBlendOp = VK_BLEND_OP_ADD;
  color_blend_attachment_info.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  color_blend_attachment_info.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

  return color_blend_attachment_info;
}

VkPipelineColorBlendAttachmentState SngoEngine::Core::Data::GetColorBlend_BLOOMFILTER()
{
  VkPipelineColorBlendAttachmentState color_blend_attachment_info{};

  color_blend_attachment_info.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                                               | VK_COLOR_COMPONENT_B_BIT
                                               | VK_COLOR_COMPONENT_A_BIT;
  color_blend_attachment_info.blendEnable = VK_TRUE;

  color_blend_attachment_info.colorBlendOp = VK_BLEND_OP_ADD;
  color_blend_attachment_info.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
  color_blend_attachment_info.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;

  color_blend_attachment_info.alphaBlendOp = VK_BLEND_OP_ADD;
  color_blend_attachment_info.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  color_blend_attachment_info.dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;

  return color_blend_attachment_info;
}

VkPipelineColorBlendStateCreateInfo SngoEngine::Core::Data::DEFAULT_COLORBLEND_INFO(
    VkPipelineColorBlendAttachmentState* p_color_blend_attachment_info)
{
  VkPipelineColorBlendStateCreateInfo color_blend_info{};
  color_blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  color_blend_info.logicOpEnable = VK_FALSE;
  color_blend_info.logicOp = VK_LOGIC_OP_COPY;
  color_blend_info.attachmentCount = 1;
  color_blend_info.pAttachments = p_color_blend_attachment_info;
  color_blend_info.blendConstants[0] = 0.0f;
  color_blend_info.blendConstants[1] = 0.0f;
  color_blend_info.blendConstants[2] = 0.0f;
  color_blend_info.blendConstants[3] = 0.0f;

  return color_blend_info;
}

VkPipelineColorBlendStateCreateInfo SngoEngine::Core::Data::DEFAULT_COLORBLEND_INFO(
    std::vector<VkPipelineColorBlendAttachmentState>& _attachments)
{
  VkPipelineColorBlendStateCreateInfo color_blend_info{};
  color_blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  color_blend_info.logicOpEnable = VK_FALSE;
  color_blend_info.logicOp = VK_LOGIC_OP_COPY;
  color_blend_info.attachmentCount = _attachments.size();
  color_blend_info.pAttachments = _attachments.data();
  color_blend_info.blendConstants[0] = 0.0f;
  color_blend_info.blendConstants[1] = 0.0f;
  color_blend_info.blendConstants[2] = 0.0f;
  color_blend_info.blendConstants[3] = 0.0f;

  return color_blend_info;
}
