#include "VertexBuffer.hpp"

VkPipelineVertexInputStateCreateInfo SngoEngine::Core::Source::Buffer::Get_EmptyVertexInputState()
{
  VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo{};
  pipelineVertexInputStateCreateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  return pipelineVertexInputStateCreateInfo;
}
