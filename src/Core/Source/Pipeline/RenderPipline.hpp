#ifndef __SNGO_RENDERPIPELINE_H
#define __SNGO_RENDERPIPELINE_H

#include "src/Core/Render/RenderPass.hpp"
#include "src/Core/Source/Buffer/Descriptor.hpp"
#include "src/Core/Source/Pipeline/Pipeline.hpp"

namespace SngoEngine::Core::Source::RenderPipeline
{

struct EngineRenderPipeline
{
  Descriptor::EngineDescriptorSetLayout* descriptor_set_layout;
  Pipeline::EnginePipelineLayout* pipeline_layout;
  Pipeline::EngineGraphicPipeline* pipeline;
};

}  // namespace SngoEngine::Core::Source::RenderPipeline

#endif