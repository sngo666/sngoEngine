#ifndef __SNGO_GUI_H
#define __SNGO_GUI_H

#include <chrono>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include "src/Core/Device/LogicalDevice.hpp"
#include "src/Core/Device/PhysicalDevice.hpp"
#include "src/Core/Instance/DebugMessenger.hpp"
#include "src/Core/Instance/Instance.hpp"
#include "src/Core/Render/FrameBuffer.hpp"
#include "src/Core/Render/RenderPass.hpp"
#include "src/Core/Signalis/Fence.hpp"
#include "src/Core/Signalis/Semaphore.hpp"
#include "src/Core/Source/Buffer/CommandBuffer.hpp"
#include "src/Core/Source/Buffer/Descriptor.hpp"
#include "src/Core/Source/Buffer/UniformBuffer.hpp"
#include "src/Core/Source/Image/DepthResource.hpp"
#include "src/Core/Source/Model/Camera.hpp"
#include "src/Core/Source/Model/Model.hpp"
#include "src/Core/Source/Pipeline/Pipeline.hpp"
#include "src/Core/Source/SwapChain/SwapChain.hpp"
#include "src/GLFWEXT/Surface.h"
#include "src/IMGUI/include/imgui_impl_vulkan.h"
#include "vulkan/vulkan_core.h"

namespace SngoEngine::Imgui
{
const std::string MODEL_obj_file{"./source/viking_room.obj"};
const std::string MODEL_texture_directory{"./textures/viking_room"};

const std::string MODEL_VertexShader_code{"./shader/vertex_shader_normal.vs"};
const std::string MODEL_FragmentShader_code{"./shader/frag_shader_normal.fs"};

const std::string SKYBOX_VertexShader_code{"./shader/vertex_shader_cubemap.vs"};
const std::string SKYBOX_FragmentShader_code{"./shader/frag_shader_cubemap.fs"};

const std::string MAIN_OLD_SCHOOL{"./source/old_school/scene.gltf"};
const std::string CUBEMAP_FILE{"./source/cube.gltf"};
const std::string CUBEMAP_TEXTURE{"./textures/cubemap_space.ktx"};

using Glfw_Err_CallBack = void (*)(int, const char*);
static void check_vk_result(VkResult err);

const std::vector<const char*> IMGUI_REQIRED_INSTANCE_EXTS = {
    VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
    VK_KHR_SURFACE_EXTENSION_NAME,
    VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
    VK_EXT_DEBUG_REPORT_EXTENSION_NAME};

const std::vector<std::string> IMGUI_REQUIRED_DEVICE_LAYERS{
    "VK_LAYER_KHRONOS_validation",
};

const std::vector<const char*> validation_layers{"VK_LAYER_KHRONOS_validation"};

const std::vector<std::string> IMGUI_REQUIRED_DEVICE_EXTS = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

std::vector<VkSubpassDependency> GUI_SUBPASS_DEPENDENCY();

static Glfw_Err_CallBack Default_Glfw_Error_Callback{[](int error, const char* description) {
  throw std::runtime_error("[err] Glfw wrong: " + std::to_string(error) + description);
}};

struct ImguiApplication
{
  ImguiApplication() = default;
  ~ImguiApplication()
  {
    destroyer();
  }

  Core::Instance::EngineInstance gui_Instance;
  GlfwExt::EngineGlfwSurface gui_Surface;
  VkPipelineCache gui_PipelineCache{};
  Core::Instance::DebugMessenger::EngineDebugMessenger gui_DebugMessenger;
  Core::Device::PhysicalDevice::EnginePhysicalDevice gui_PhysicalDevice{};
  Core::Source::Descriptor::EngineDescriptorPool gui_DescriptorPool{};
  Core::Source::SwapChain::EngineSwapChain gui_SwapChain;
  Core::Render::EngineFrameBuffers swapchain_framebuffers;

  Core::Render::RenderPass::EngineRenderPass main_RenderPass{};
  Core::Source::DepthResource::EngineDepthResource gui_DepthResource;
  Core::Source::DepthResource::EngineDepthResource model_DepthResource;

  Core::Source::Buffer::EngineCommandPool gui_CommandPool;
  std::vector<Core::Source::Buffer::EngineCommandBuffer> gui_CommandBuffers;
  Core::Siganlis::EngineFences gui_Fences;
  Core::Siganlis::EngineSemaphores gui_ImageAcquiredSemaphores;
  Core::Siganlis::EngineSemaphores gui_RenderCompleteSemaphores;

  Core::Source::Pipeline::EnginePipelineLayout model_Pipelinelayout;
  Core::Source::Pipeline::EngineGraphicPipeline model_GraphicPipeline;
  Core::Source::Pipeline::EnginePipelineLayout skybox_Pipelinelayout;
  Core::Source::Pipeline::EngineGraphicPipeline skybox_GraphicPipeline;

  Core::Source::Buffer::TransUniBuffer model_UniBuffer;
  Core::Source::Descriptor::EngineDescriptorPool uni_pool;

  Core::Source::Descriptor::EngineDescriptorSetLayout uni_setlayout;
  Core::Source::Descriptor::EngineDescriptorSet uni_set;

  Core::Source::Descriptor::EngineDescriptorSetLayout skybox_setlayout;
  Core::Source::Descriptor::EngineDescriptorSet skybox_set;

  Core::Device::LogicalDevice::EngineDevice gui_Device;

  Core::Source::Model::EngineGltfModel old_school;
  Core::Source::Model::EngineCubeMap sky_box;
  EngineCamera main_Camera;

  std::array<VkClearValue, 2> gui_Clearvalue{};

  struct
  {
    struct
    {
      bool left = false;
      bool right = false;
      bool middle = false;
    } buttons;
    glm::vec2 position;
  } mouseState;

  void binding_keymapping(ImGuiIO& io);
  int init();
  void Render_Frame(ImDrawData* draw_data, std::array<VkClearValue, 2> gui_Clearvalue);
  void Present_Frame();
  void record_command_buffer(VkCommandBuffer m_command_buffer);
  void construct_pipeline();
  void load_model();
  void update_uniform_buffer(uint32_t current_frame);
  void create_IMGUI_DescriptorPoor();
  void destroyer();

  std::string window_name{"imgui_pbrt Window"};
  VkExtent2D mainWindow_extent{1280, 720};
  uint32_t frames_InFlight{2};
  uint32_t semaphore_count{};
  uint32_t Image_count{};
  uint32_t Frame_Index{};
  uint32_t Semaphore_Index{};
  uint32_t imageIndex{};

  bool gui_SwapChainRebuild = false;

 private:
  std::string mainWindow_Title;
  VkAllocationCallbacks* gui_Alloc{};
};

}  // namespace SngoEngine::Imgui

#endif