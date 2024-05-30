#include "Imgui.h"

#include <minwindef.h>
#include <vulkan/vulkan_core.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <ctime>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "fmt/core.h"
#include "src/Core/Data.h"
#include "src/Core/Device/LogicalDevice.hpp"
#include "src/Core/Device/PhysicalDevice.hpp"
#include "src/Core/Instance/DebugMessenger.hpp"
#include "src/Core/Instance/Instance.hpp"
#include "src/Core/Macro.h"
#include "src/Core/Render/RenderPass.hpp"
#include "src/Core/Signalis/Fence.hpp"
#include "src/Core/Signalis/Semaphore.hpp"
#include "src/Core/Source/Buffer/CommandBuffer.hpp"
#include "src/Core/Source/Buffer/Descriptor.hpp"
#include "src/Core/Source/Buffer/VertexBuffer.hpp"
#include "src/Core/Source/Model/Camera.hpp"
#include "src/Core/Source/Model/Model.hpp"
#include "src/Core/Source/Pipeline/Pipeline.hpp"
#include "src/Core/Source/Pipeline/RenderPipline.hpp"
#include "src/Core/Source/SwapChain/SwapChain.hpp"
#include "src/Core/Utils/Utils.hpp"
#include "src/GLFWEXT/Surface.h"
#include "src/IMGUI/include/imgui.h"
#include "src/IMGUI/include/imgui_impl_glfw.h"

static VKAPI_ATTR VkBool32 VKAPI_CALL
debug_call_back(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                VkDebugUtilsMessageTypeFlagsEXT message_type,
                const VkDebugUtilsMessengerCallbackDataEXT* p_call_back_data,
                void* p_user_data)
{
  if (message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
      std::cerr << "validation layers: " << p_call_back_data->pMessage << '\n';
    }
  return VK_FALSE;
}

static void SngoEngine::Imgui::check_vk_result(VkResult err)
{
  if (err == 0)
    return;
  fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
  if (err < 0)
    abort();
}

std::vector<VkSubpassDependency> SngoEngine::Imgui::GUI_SUBPASS_DEPENDENCY()
{
  std::vector<VkSubpassDependency> deps;

  VkSubpassDependency dependency_1st{};
  dependency_1st.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency_1st.dstSubpass = 0;
  dependency_1st.srcStageMask =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  dependency_1st.srcAccessMask = 0;
  dependency_1st.dstStageMask =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  dependency_1st.dstAccessMask =
      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

  deps.emplace_back(dependency_1st);

  VkSubpassDependency dependency_2nd{};
  dependency_2nd.srcSubpass = 0;
  dependency_2nd.dstSubpass = 1;
  dependency_2nd.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency_2nd.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency_2nd.srcAccessMask = 0;
  dependency_2nd.dstAccessMask =
      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
  deps.emplace_back(dependency_2nd);

  return deps;
}

int SngoEngine::Imgui::ImguiApplication::init()
{
  glfwSetErrorCallback(Default_Glfw_Error_Callback);
  if (!glfwInit())
    return 1;

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  if (!glfwVulkanSupported())
    {
      printf("GLFW: Vulkan Not Supported\n");
      return 1;
    }
  gui_Instance.init("imgui_pbrt", IMGUI_REQUIRED_DEVICE_EXTS);

  fmt::println("instance created");

  gui_DebugMessenger.init(&gui_Instance, debug_call_back);
  fmt::println("debug created");

  gui_Surface(&gui_Instance, mainWindow_extent, window_name);
  fmt::println("gui_Surface created");

  gui_PhysicalDevice.init(&gui_Instance, gui_Surface.surface);
  fmt::println("gui_PhysicalDevice created");

  auto device_EXTs{IMGUI_REQUIRED_DEVICE_EXTS};
  auto device_LAYERs{IMGUI_REQUIRED_DEVICE_LAYERS};
#ifdef VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME

  if (Core::Device::PhysicalDevice::Check_PhysicalDevice_ExtensionSupport(
          gui_PhysicalDevice.physical_device, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME))
    device_EXTs.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
#endif

  gui_Device.init(&gui_PhysicalDevice, gui_Surface.surface, device_EXTs, device_LAYERs);
  fmt::println("gui_Device created");

  create_IMGUI_DescriptorPoor();
  fmt::println("gui_DescriptorPool created");

  Core::Source::SwapChain::SwapChainRequirement swap_chain_requirements{&gui_Device,
                                                                        gui_Surface.window};

  gui_SwapChain(&gui_Device, swap_chain_requirements);

  fmt::println("gui_SwapChain created");

  // --------------------  RenderPasses  --------------------

  semaphore_count = Core::Macro::MAX_FRAMES_IN_FLIGHT;
  Image_count = swap_chain_requirements.img_count;
  sampler_flag = Core::Source::RenderPipeline::MSAA_maxAvailableSampleCount(&gui_Device);

  msaa_renderpass.init(&gui_Device,
                       gui_SwapChain.extent,
                       gui_SwapChain.image_format,
                       sampler_flag,
                       &gui_SwapChain,
                       true);

  fmt::println("main_RenderPass created");
  // --------------------  FrameBuffers  --------------------

  fmt::println("FrameBuffers created");

  // ---------------------  Command  ------------------------

  gui_CommandPool.init(
      &gui_Device,
      Core::Data::CommandPoolCreate_Info(gui_Device.queue_family.graphicsFamily.value()));
  fmt::println("gui_CommandPool created");

  gui_CommandBuffers.resize(Core::Macro::MAX_FRAMES_IN_FLIGHT);
  for (int i = 0; i < Core::Macro::MAX_FRAMES_IN_FLIGHT; i++)
    {
      gui_CommandBuffers[i].init(&gui_Device, gui_CommandPool.command_pool);
    }
  fmt::println("gui_CommandBuffers created");

  // ---------------------  Unibuffer  ------------------------

  model_UniBuffer.init(&gui_Device, gui_Device.graphics_queue);
  // prepare for uniform binding
  {
    std::vector<VkDescriptorPoolSize> poolSizes = {
        Core::Source::Descriptor::Get_DescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4),
        Core::Source::Descriptor::Get_DescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                         6)};

    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
        Core::Source::Descriptor::GetLayoutBinding(
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0)};

    uni_pool.init(&gui_Device, 4, poolSizes);
    uni_setlayout.init(&gui_Device, setLayoutBindings);
    uni_set.init(&gui_Device, &uni_setlayout, &uni_pool);

    std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
        // Binding 0 : Vertex shader uniform buffer
        Core::Source::Descriptor::GetDescriptSet_Write(uni_set.descriptor_set,
                                                       VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                       0,
                                                       &model_UniBuffer.descriptor)};
    uni_set.updateWrite(writeDescriptorSets);
  }

  // ------------------  HDR renderpass     ---------------------
  {
    hdr_renderpass.init(&gui_Device, gui_SwapChain.extent, sampler_flag);
  }

  // ------------------ bloom filter  ---------------------
  {
    bloom_renderpass.init(&gui_Device, gui_SwapChain.extent);
    std::vector<VkDescriptorImageInfo> img_infos{
        VkDescriptorImageInfo{hdr_renderpass.sampler.sampler,
                              hdr_renderpass.attchment_Resolve_0.view.image_view,
                              VK_IMAGE_LAYOUT_GENERAL},
        VkDescriptorImageInfo{hdr_renderpass.sampler.sampler,
                              hdr_renderpass.attchment_Resolve_1.view.image_view,
                              VK_IMAGE_LAYOUT_GENERAL},
    };
    bloom_renderpass.construct_descriptor(&uni_pool, img_infos);
  }

  // MSAA descriptor
  {
    std::vector<VkDescriptorImageInfo> img_infos{
        VkDescriptorImageInfo{hdr_renderpass.sampler.sampler,
                              hdr_renderpass.attchment_Resolve_0.view.image_view,
                              VK_IMAGE_LAYOUT_GENERAL},
        VkDescriptorImageInfo{bloom_renderpass.sampler.sampler,
                              bloom_renderpass.attchment_FloatingPoint.view.image_view,
                              VK_IMAGE_LAYOUT_GENERAL},
    };
    msaa_renderpass.construct_descriptor(&uni_pool, img_infos);
  }

  // ---------------------  Models  ------------------------

  load_model();

  // --------------------- Pipeline ------------------------

  construct_pipeline();

  fmt::println("model and skybox pipeline constructed");

  auto bloom_stage{Core::Source::Pipeline::EngineShaderStage(
      &gui_Device, BLOOM_VertexShader_code, BLOOM_FragmentShader_code)};
  auto msaa_stage{Core::Source::Pipeline::EngineShaderStage(
      &gui_Device, MSAA_VertexShader_code, MSAA_FragmentShader_code)};

  msaa_renderpass.construct_pipeline(msaa_stage.stages, sampler_flag);

  fmt::println("msaa_renderpass pipeline constructed");

  bloom_renderpass.construct_pipeline(bloom_stage.stages,
                                      &msaa_renderpass.pipeline_layout,
                                      &msaa_renderpass.renderpass,
                                      sampler_flag);

  fmt::println("bloom_renderpass pipeline constructed");

  // --------------------- Semaphore ------------------------

  gui_Fences.init(&gui_Device, Core::Macro::MAX_FRAMES_IN_FLIGHT);
  fmt::println("gui_Fences created");

  gui_ImageAcquiredSemaphores.init(&gui_Device, semaphore_count);
  gui_RenderCompleteSemaphores.init(&gui_Device, semaphore_count);
  fmt::println("gui_ImageAcquiredSemaphores&gui_RenderCompleteSemaphores created");

  // --------------------- Camera ------------------------

  main_Camera = {EngineCamera::CameraType::firstperson,
                 glm::vec3(0.0f, 0.0f, -4.8f),
                 glm::vec3(4.5f, -380.0f, 0.0f)};
  main_Camera.setPerspective(
      30.0f, (float)mainWindow_extent.width / (float)mainWindow_extent.height, 0.1f, 256.0f);
  main_Camera.setPosition({0.0f, 0.0f, 0.0f});
  main_Camera.setMovementSpeed(4.0f);
  main_Camera.setRotationSpeed(0.3f);

  // ---------------------------------------------------------

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls
  // binding_keymapping(io);
  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForVulkan(gui_Surface.window, TRUE);

  ImGui_ImplVulkan_InitInfo init_info = {};
  init_info.Instance = gui_Instance.instance;
  init_info.PhysicalDevice = gui_PhysicalDevice.physical_device;
  init_info.Device = gui_Device.logical_device;
  init_info.QueueFamily = gui_Device.queue_family.graphicsFamily.value();
  init_info.Queue = gui_Device.graphics_queue;
  init_info.PipelineCache = gui_PipelineCache;
  init_info.DescriptorPool = gui_DescriptorPool.descriptor_pool;
  init_info.RenderPass = msaa_renderpass.renderpass.render_pass;
  init_info.Subpass = 1;
  init_info.MinImageCount = frames_InFlight;
  init_info.ImageCount = Core::Macro::MAX_FRAMES_IN_FLIGHT;
  init_info.MSAASamples = sampler_flag;
  init_info.Allocator = nullptr;
  init_info.CheckVkResultFn = check_vk_result;

  ImGui_ImplVulkan_Init(&init_info);

  bool show_demo_window = false;
  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
  float move_speed = 1 / 30.0f;
  float region_x = io.MousePos.x;
  float region_y = io.MousePos.y;

  while (!glfwWindowShouldClose(gui_Surface.window))
    {
      // Poll and handle events (inputs, window resize, etc.)
      // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui
      // wants to use your inputs.
      // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main
      // application, or clear/overwrite your copy of the mouse data.
      // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main
      // application, or clear/overwrite your copy of the keyboard data. Generally you may always
      // pass all inputs to dear imgui, and hide them from your application based on those two
      // flags.
      auto now = clock();

      glfwPollEvents();

      // if (33 > 1000.0f / io.Framerate)
      //   {
      //     while (clock() - now < (static_cast<uint32_t>(33 - 1000.0f / io.Framerate)))
      //       ;
      //   }

      // Resize swap chain?
      if (gui_SwapChainRebuild)
        {
          int width = 0, height = 0;
          glfwGetFramebufferSize(gui_Surface.window, &width, &height);
          while (width == 0 || height == 0)
            {
              glfwGetFramebufferSize(gui_Surface.window, &width, &height);
              glfwWaitEvents();
            }
          vkDeviceWaitIdle(gui_Device.logical_device);

          gui_SwapChain.Recreate_Self(gui_Surface.window);

          gui_SwapChainRebuild = false;
        }

      // Start the Dear ImGui frame
      ImGui_ImplVulkan_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();
      io = ImGui::GetIO();

      // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You
      // can browse its code to learn more about Dear ImGui!).
      if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);

      // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a
      // named window.
      auto time_start = std::chrono::high_resolution_clock::now();
      bool viewUpdated = false;
      bool camera_Updated = false;

      {
        ImGui::Begin("Sngo Engine!");

        ImGui::Checkbox("Render Skybox", &render_skybox);
        ImGui::Checkbox("Bloom", &will_bloom);

        ImGui::InputFloat("Exposure", &exposure, 0.025f, 3);
        // if (ImGui::Checkbox("use MSAA", &use_sampler_shading))
        //   {
        // TODO: runtime toggle MSAA not support, waitting for better structure
        // construct_pipeline();
        // }

        // if (main_Camera.type == EngineCamera::firstperson)
        {
          main_Camera.keys.up = ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_W));
          main_Camera.keys.down = ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_S));
          main_Camera.keys.left = ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_A));
          main_Camera.keys.right = ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_D));

          ImGui::Text("mouseR: ");
          if (ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_MouseRight)))
            {
              ImVec2 pos = ImGui::GetMousePos();
              ImGui::SameLine();
              ImGui::Text("%.1f %.1f", pos.x, pos.y);
              camera_Updated = true;
            }
        }

        if (main_Camera.moving())
          {
            viewUpdated = true;
          }

        ImGui::CheckboxFlags("io.ConfigFlags: NavEnableKeyboard",
                             (unsigned int*)&io.ConfigFlags,
                             ImGuiConfigFlags_NavEnableKeyboard);
        ImGui::Text("Vulkan API %i.%i.%i",
                    VK_API_VERSION_MAJOR(gui_Device.pPD->properties.apiVersion),
                    VK_API_VERSION_MINOR(gui_Device.pPD->properties.apiVersion),
                    VK_API_VERSION_PATCH(gui_Device.pPD->properties.apiVersion));

        // driver infos
        {
          VkPhysicalDeviceDriverProperties driverProperties{};
          if (gui_Device.ext_supported(VK_KHR_DRIVER_PROPERTIES_EXTENSION_NAME))
            {
              VkPhysicalDeviceProperties2 deviceProperties2 = {};
              deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
              deviceProperties2.pNext = &driverProperties;
              driverProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES;
              vkGetPhysicalDeviceProperties2(gui_Device.pPD->physical_device, &deviceProperties2);
            }
          ImGui::Text("%s %s", driverProperties.driverName, driverProperties.driverInfo);
        }
        ImGui::SliderFloat("move speed", &main_Camera.movementSpeed, 1.0f, 10.0f);
        ImGui::SliderFloat("view speed", &main_Camera.rotationSpeed, 0.05f, 1.5f);
        ImGui::Text("camera:");
        ImGui::SameLine();
        ImGui::Text("%.1f %.1f %.1f",
                    main_Camera.position.x,
                    main_Camera.position.y,
                    main_Camera.position.z);

        // Edit 3 floats representing a color
        ImGui::ColorEdit3("clear color", (float*)&clear_color);

        ImGui::Text(
            "Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

        ImGui::End();
      }

      if (viewUpdated)
        {
          auto time_end = std::chrono::high_resolution_clock::now();

          auto t{std::chrono::duration<float, std::milli>(time_end - time_start).count() / 200.0f};
          ImGui::Text("frame time: %f", t);

          main_Camera.update(t);
        }

      ImGui::Text("dx, dy: ");
      if (camera_Updated)
        {
          ImVec2 pos = io.MouseDelta;
          float dx = pos.x;
          float dy = pos.y;
          main_Camera.rotate(
              glm::vec3(dy * -main_Camera.rotationSpeed, dx * main_Camera.rotationSpeed, 0.0f));

          ImGui::SameLine();
          ImGui::Text("%f, %f", dx, dy);
          ImGui::Text("pos: ");
          ImGui::SameLine();
          ImGui::Text("(%f, %f)", pos.x, pos.x);

          camera_Updated = false;
          // main_Camera.translate(glm::vec3(-0.0f, 0.0f, dy * .005f));
        }

      // Rendering
      ImGui::Render();
      ImDrawData* draw_data = ImGui::GetDrawData();
      const bool is_minimized =
          (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
      if (!is_minimized)
        {
          VkClearValue color{{{clear_color.x * clear_color.w,
                               clear_color.y * clear_color.w,
                               clear_color.z * clear_color.w,
                               clear_color.w}}};

          gui_Clearvalue[0].color = color.color;
          gui_Clearvalue[1].color = color.color;
          gui_Clearvalue[2].depthStencil = {1.0f, 0};
          gui_Clearvalue[3].color = color.color;
          gui_Clearvalue[4].color = color.color;

          Render_Frame(draw_data, gui_Clearvalue);
          Present_Frame();
        }
    }
  check_vk_result(vkDeviceWaitIdle(gui_Device.logical_device));

  // gui_SwapChain.CleanUp_Self();
  fmt::println("destory end");

  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(gui_Surface.window);
  glfwTerminate();

  return 0;
}

void SngoEngine::Imgui::ImguiApplication::Render_Frame(ImDrawData* draw_data,
                                                       std::vector<VkClearValue>& gui_Clearvalue)
{
  VkSemaphore img_ac_semaphore{gui_ImageAcquiredSemaphores[Frame_Index]};
  VkSemaphore render_ok_semaphore{gui_RenderCompleteSemaphores[Frame_Index]};

  auto err = vkWaitForFences(gui_Device.logical_device,
                             1,
                             &gui_Fences[Frame_Index],
                             VK_TRUE,
                             UINT64_MAX);  // wait indefinitely instead of periodically checking
  check_vk_result(err);

  err = vkAcquireNextImageKHR(gui_Device.logical_device,
                              gui_SwapChain.swap_chain,
                              UINT64_MAX,
                              img_ac_semaphore,
                              VK_NULL_HANDLE,
                              &imageIndex);

  if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
    {
      gui_SwapChainRebuild = true;
      return;
    }
  check_vk_result(err);

  {
    update_uniform_buffer(Frame_Index);

    err = vkResetFences(gui_Device.logical_device, 1, &gui_Fences[Frame_Index]);
    check_vk_result(err);
  }

  {
    err = vkResetCommandBuffer(gui_CommandBuffers[Frame_Index].command_buffer, 0);
    check_vk_result(err);
    // vkResetCommandBuffer(gui_CommandBuffers[Frame_Index].command_buffer, 0);

    VkCommandBufferBeginInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.flags |= VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    err = vkBeginCommandBuffer(gui_CommandBuffers[Frame_Index].command_buffer, &info);
    check_vk_result(err);
  }

  VkRenderPassBeginInfo render_pass_begin_info{};

  render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_begin_info.renderPass = hdr_renderpass.renderpass();
  render_pass_begin_info.framebuffer = hdr_renderpass.framebuffer();
  render_pass_begin_info.renderArea.offset = {0, 0};
  render_pass_begin_info.renderArea.extent = gui_SwapChain.extent;
  render_pass_begin_info.clearValueCount = gui_Clearvalue.size();
  render_pass_begin_info.pClearValues = gui_Clearvalue.data();

  vkCmdBeginRenderPass(gui_CommandBuffers[Frame_Index].command_buffer,
                       &render_pass_begin_info,
                       VK_SUBPASS_CONTENTS_INLINE);

  {
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(gui_SwapChain.extent.width);
    viewport.height = static_cast<float>(gui_SwapChain.extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(gui_CommandBuffers[Frame_Index].command_buffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = gui_SwapChain.extent;
    vkCmdSetScissor(gui_CommandBuffers[Frame_Index].command_buffer, 0, 1, &scissor);

    // skybox_pipeline
    vkCmdBindPipeline(gui_CommandBuffers[Frame_Index].command_buffer,
                      VK_PIPELINE_BIND_POINT_GRAPHICS,
                      skybox_GraphicPipeline.pipeline);

    if (render_skybox)
      {
        vkCmdBindDescriptorSets(gui_CommandBuffers[Frame_Index].command_buffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                skybox_Pipelinelayout.pipeline_layout,
                                1,
                                1,
                                &skybox_set.descriptor_set,
                                0,
                                nullptr);

        vkCmdBindDescriptorSets(gui_CommandBuffers[Frame_Index].command_buffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                skybox_Pipelinelayout.pipeline_layout,
                                0,
                                1,
                                &uni_set.descriptor_set,
                                0,
                                nullptr);

        sky_box.draw(gui_CommandBuffers[Frame_Index].command_buffer,
                     skybox_Pipelinelayout.pipeline_layout,
                     1);
      }

    // render model
    vkCmdBindPipeline(gui_CommandBuffers[Frame_Index].command_buffer,
                      VK_PIPELINE_BIND_POINT_GRAPHICS,
                      model_GraphicPipeline.pipeline);

    vkCmdBindDescriptorSets(gui_CommandBuffers[Frame_Index].command_buffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            model_Pipelinelayout.pipeline_layout,
                            0,
                            1,
                            &uni_set.descriptor_set,
                            0,
                            nullptr);

    old_school.bind_buffers(gui_CommandBuffers[Frame_Index].command_buffer);
    old_school.draw(gui_CommandBuffers[Frame_Index].command_buffer,
                    model_Pipelinelayout.pipeline_layout);
  }

  // Submit command buffer
  vkCmdEndRenderPass(gui_CommandBuffers[Frame_Index].command_buffer);

  if (will_bloom)
    {
      std::vector<VkClearValue> clearValues{2};
      clearValues[0].color = {{0.0f, 0.0f, 0.0f, 0.0f}};
      clearValues[1].depthStencil = {1.0f, 0};

      // Bloom filter
      VkRenderPassBeginInfo renderPassBeginInfo{};
      renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
      renderPassBeginInfo.framebuffer = bloom_renderpass.framebuffer();
      renderPassBeginInfo.renderPass = bloom_renderpass.renderpass();
      renderPassBeginInfo.clearValueCount = 1;
      renderPassBeginInfo.renderArea.extent.width = bloom_renderpass.extent.width;
      renderPassBeginInfo.renderArea.extent.height = bloom_renderpass.extent.height;
      renderPassBeginInfo.pClearValues = clearValues.data();

      vkCmdBeginRenderPass(
          gui_CommandBuffers[Frame_Index](), &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

      VkViewport viewport = Core::Render::RenderPass::Get_ViewPort(
          (float)bloom_renderpass.extent.width, (float)bloom_renderpass.extent.height, 0.0f, 1.0f);
      vkCmdSetViewport(gui_CommandBuffers[Frame_Index](), 0, 1, &viewport);

      VkRect2D scissor = Core::Render::RenderPass::Get_Rect2D(
          bloom_renderpass.extent.width, bloom_renderpass.extent.height, 0, 0);
      vkCmdSetScissor(gui_CommandBuffers[Frame_Index](), 0, 1, &scissor);

      vkCmdBindDescriptorSets(gui_CommandBuffers[Frame_Index](),
                              VK_PIPELINE_BIND_POINT_GRAPHICS,
                              bloom_renderpass.pipeline_layout(),
                              0,
                              1,
                              &bloom_renderpass.bloom_set.descriptor_set,
                              0,
                              nullptr);

      vkCmdBindPipeline(gui_CommandBuffers[Frame_Index](),
                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                        bloom_renderpass.pipelines[1]());
      vkCmdDraw(gui_CommandBuffers[Frame_Index](), 3, 1, 0, 0);

      vkCmdEndRenderPass(gui_CommandBuffers[Frame_Index]());
    }

  {
    VkClearValue clearValues[3];
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 0.0f}};
    clearValues[1].color = {{0.0f, 0.0f, 0.0f, 0.0f}};
    clearValues[2].depthStencil = {1.0f, 0};

    // Final composition
    VkRenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.framebuffer = msaa_renderpass.framebuffers[imageIndex];
    renderPassBeginInfo.renderPass = msaa_renderpass.renderpass();
    renderPassBeginInfo.clearValueCount = 3;
    renderPassBeginInfo.renderArea.extent.width = msaa_renderpass.extent.width;
    renderPassBeginInfo.renderArea.extent.height = msaa_renderpass.extent.height;
    renderPassBeginInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(
        gui_CommandBuffers[Frame_Index](), &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = Core::Render::RenderPass::Get_ViewPort(
        (float)msaa_renderpass.extent.width, (float)msaa_renderpass.extent.height, 0.0f, 1.0f);
    vkCmdSetViewport(gui_CommandBuffers[Frame_Index](), 0, 1, &viewport);

    VkRect2D scissor = Core::Render::RenderPass::Get_Rect2D(
        msaa_renderpass.extent.width, msaa_renderpass.extent.height, 0, 0);

    vkCmdSetScissor(gui_CommandBuffers[Frame_Index](), 0, 1, &scissor);

    vkCmdBindDescriptorSets(gui_CommandBuffers[Frame_Index](),
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            msaa_renderpass.pipeline_layout(),
                            0,
                            1,
                            &msaa_renderpass.msaa_set.descriptor_set,
                            0,
                            nullptr);

    // Scene
    vkCmdBindPipeline(gui_CommandBuffers[Frame_Index](),
                      VK_PIPELINE_BIND_POINT_GRAPHICS,
                      msaa_renderpass.pipeline());
    vkCmdDraw(gui_CommandBuffers[Frame_Index](), 3, 1, 0, 0);

    vkCmdNextSubpass(gui_CommandBuffers[Frame_Index].command_buffer, VK_SUBPASS_CONTENTS_INLINE);
    // Record dear imgui primitives into command buffer
    ImGui_ImplVulkan_RenderDrawData(draw_data, gui_CommandBuffers[Frame_Index]());

    vkCmdEndRenderPass(gui_CommandBuffers[Frame_Index].command_buffer);
  }

  {
    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &img_ac_semaphore;
    info.pWaitDstStageMask = &wait_stage;
    info.commandBufferCount = 1;
    info.pCommandBuffers = &gui_CommandBuffers[Frame_Index].command_buffer;
    info.signalSemaphoreCount = 1;
    info.pSignalSemaphores = &render_ok_semaphore;

    err = vkEndCommandBuffer(gui_CommandBuffers[Frame_Index].command_buffer);
    check_vk_result(err);
    err = vkQueueSubmit(gui_Device.graphics_queue, 1, &info, gui_Fences[Frame_Index]);
    check_vk_result(err);
  }
}

void SngoEngine::Imgui::ImguiApplication::Present_Frame()
{
  if (gui_SwapChainRebuild)
    return;
  VkSemaphore render_complete_semaphore{gui_RenderCompleteSemaphores[Frame_Index]};
  VkPresentInfoKHR info = {};
  info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  info.waitSemaphoreCount = 1;
  info.pWaitSemaphores = &render_complete_semaphore;
  info.swapchainCount = 1;
  info.pSwapchains = &gui_SwapChain.swap_chain;
  info.pImageIndices = &imageIndex;
  VkResult err = vkQueuePresentKHR(gui_Device.graphics_queue, &info);
  if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
    {
      gui_SwapChainRebuild = true;
      return;
    }
  check_vk_result(err);
  Frame_Index = (Frame_Index + 1) % Core::Macro::MAX_FRAMES_IN_FLIGHT;
}

void SngoEngine::Imgui::ImguiApplication::construct_pipeline()
{
  std::vector<VkVertexInputBindingDescription> binding_description = {
      Core::Source::Model::GLTF_EngineModelVertexData::getBindingDescription()};
  auto attribute_descriptions =
      Core::Source::Model::GLTF_EngineModelVertexData::getAttributeDescriptions(
          Core::Source::Model::GLTF_EngineModelVertexData::POS
              | Core::Source::Model::GLTF_EngineModelVertexData::NORMAL
              | Core::Source::Model::GLTF_EngineModelVertexData::UV
              | Core::Source::Model::GLTF_EngineModelVertexData::TANGENT,
          0);
  VkPipelineVertexInputStateCreateInfo vertex_input{
      Core::Data::GetVertexInput_Info(binding_description, attribute_descriptions)};

  Core::Data::PipelinePreparation_Info pipeline_info{
      Core::Source::RenderPipeline::Default_Pipeline(gui_SwapChain.extent, vertex_input)};

  std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates = {
      Core::Data::GetColorBlend_DEFAULT(),
      Core::Data::GetColorBlend_DEFAULT(),
  };
  pipeline_info.color_blend = Core::Data::DEFAULT_COLORBLEND_INFO(blendAttachmentStates);
  pipeline_info.color_blend.attachmentCount = 2;
  if (sampler_flag != VK_SAMPLE_COUNT_1_BIT)
    {
      pipeline_info.multisampling = Core::Data::MULTISAMPLING_INFO_ENABLED(sampler_flag, 0.25f);
    }

  // -------------------- pipeline layout ---------------------

  std::vector<VkPushConstantRange> ranges{};
  model_Pipelinelayout.init(
      &gui_Device,
      std::vector<VkDescriptorSetLayout>{uni_setlayout.layout, old_school.layouts.texture.layout},
      ranges);

  skybox_Pipelinelayout.init(
      &gui_Device,
      std::vector<VkDescriptorSetLayout>{uni_setlayout.layout, skybox_setlayout.layout},
      ranges);

  // -------------------- shader code ---------------------

  auto model_shader_stages{Core::Source::Pipeline::EngineShaderStage(
      &gui_Device, MODEL_VertexShader_code, MODEL_FragmentShader_code)};

  auto skybox_shader_stages{Core::Source::Pipeline::EngineShaderStage(
      &gui_Device, SKYBOX_VertexShader_code, SKYBOX_FragmentShader_code)};

  // -------------------- pipeline initialization ---------------------

  model_GraphicPipeline.init(&gui_Device,
                             &model_Pipelinelayout,
                             &hdr_renderpass.renderpass,
                             model_shader_stages.stages,
                             &pipeline_info,
                             0);

  auto sky_box_attribute_descriptions =
      Core::Source::Model::GLTF_EngineModelVertexData::getAttributeDescriptions(
          Core::Source::Model::GLTF_EngineModelVertexData::POS, 0);
  VkPipelineVertexInputStateCreateInfo sky_box_vertex_input{
      Core::Data::GetVertexInput_Info(binding_description, sky_box_attribute_descriptions)};

  pipeline_info.vertex_input = sky_box_vertex_input;
  pipeline_info.depth_stencil = Core::Data::DEFAULT_DEPTHSTENCIL_DISABLED_INFO();
  pipeline_info.rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;

  skybox_GraphicPipeline.init(&gui_Device,
                              &skybox_Pipelinelayout,
                              &hdr_renderpass.renderpass,
                              skybox_shader_stages.stages,
                              &pipeline_info,
                              0);
}

void SngoEngine::Imgui::ImguiApplication::load_model()
{
  old_school.init(MAIN_OLD_SCHOOL, &gui_Device, &gui_CommandPool);
  sky_box.init(CUBEMAP_FILE, CUBEMAP_TEXTURE, &gui_Device, &gui_CommandPool);

  sky_box.generate_descriptor(uni_pool, skybox_setlayout, skybox_set, 1);
}

void SngoEngine::Imgui::ImguiApplication::update_uniform_buffer(uint32_t current_frame)
{
  Core::Source::Buffer::UniformBuffer_Trans ubo{};
  ubo.projection = main_Camera.matrices.perspective;
  ubo.modelView = main_Camera.matrices.view;
  ubo.inverseModelview = glm::inverse(main_Camera.matrices.view);
  ubo.lodBias = exposure;

  memcpy(model_UniBuffer.mapped, &ubo, sizeof(ubo));
}

void SngoEngine::Imgui::ImguiApplication::binding_keymapping(ImGuiIO& io)
{
  io.KeyMap[ImGuiKey_Tab] = VK_TAB;
  io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
  io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
  io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
  io.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
  io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
  io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
  io.KeyMap[ImGuiKey_Space] = VK_SPACE;
  io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
}
// Create DescriptorPool for m_ImGuiDescriptorPool
void SngoEngine::Imgui::ImguiApplication::create_IMGUI_DescriptorPoor()
{
  VkDescriptorPoolSize pool_sizes[] = {{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
                                       {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
                                       {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
                                       {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
                                       {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
                                       {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
                                       {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
                                       {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
                                       {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
                                       {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
                                       {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

  VkDescriptorPoolCreateInfo pool_info = {};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
  pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
  pool_info.pPoolSizes = pool_sizes;

  if (vkCreateDescriptorPool(
          gui_Device.logical_device, &pool_info, nullptr, &gui_DescriptorPool.descriptor_pool)
      != VK_SUCCESS)
    throw std::runtime_error("Create DescriptorPool for m_ImGuiDescriptorPool failed!");
}

void SngoEngine::Imgui::ImguiApplication::destroyer()
{
  check_vk_result(vkDeviceWaitIdle(gui_Device.logical_device));
  gui_Fences.destroyer();
  gui_ImageAcquiredSemaphores.destroyer();
  gui_RenderCompleteSemaphores.destroyer();

  model_GraphicPipeline.destroyer();
  model_Pipelinelayout.destroyer();
  skybox_GraphicPipeline.destroyer();
  skybox_Pipelinelayout.destroyer();

  uni_setlayout.destroyer();
  skybox_setlayout.destroyer();
  gui_DescriptorPool.destroyer();
  uni_pool.destroyer();

  old_school.destroyer();
  sky_box.destroyer();
  gui_CommandPool.destroyer();

  gui_SwapChain.destroyer();

  gui_Device.destroyer();
  gui_Surface.destroyer();

  gui_DebugMessenger.destroyer();
  gui_Instance.destroyer();
}
