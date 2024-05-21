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
#include "src/Core/Instance/DebugMessenger.h"
#include "src/Core/Instance/Instance.h"
#include "src/Core/Macro.h"
#include "src/Core/Render/RenderPass.hpp"
#include "src/Core/Signalis/Fence.h"
#include "src/Core/Signalis/Semaphore.h"
#include "src/Core/Source/Buffer/CommandBuffer.hpp"
#include "src/Core/Source/Buffer/Descriptor.hpp"
#include "src/Core/Source/Buffer/VertexBuffer.hpp"
#include "src/Core/Source/Model/Camera.hpp"
#include "src/Core/Source/Model/Model.hpp"
#include "src/Core/Source/Pipeline/Pipeline.hpp"
#include "src/Core/Source/SwapChain/SwapChain.h"
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

std::vector<SngoEngine::Core::Data::SubpassDependency_Info>
SngoEngine::Imgui::GUI_SUBPASS_DEPENDENCY()
{
  std::vector<SngoEngine::Core::Data::SubpassDependency_Info> deps;

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
  gui_Instance("imgui_pbrt", IMGUI_REQUIRED_DEVICE_EXTS);

  fmt::println("instance created");

  gui_DebugMessenger(&gui_Instance, debug_call_back);
  fmt::println("debug created");

  gui_Surface(&gui_Instance, mainWindow_extent, window_name);
  fmt::println("gui_Surface created");

  gui_PhysicalDevice(&gui_Instance, gui_Surface.surface);
  fmt::println("gui_PhysicalDevice created");

  auto device_EXTs{IMGUI_REQUIRED_DEVICE_EXTS};
  auto device_LAYERs{IMGUI_REQUIRED_DEVICE_LAYERS};
#ifdef VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME

  if (Core::Device::PhysicalDevice::Check_PhysicalDevice_ExtensionSupport(
          gui_PhysicalDevice.physical_device, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME))
    device_EXTs.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
#endif

  gui_Device(&gui_PhysicalDevice, gui_Surface.surface, device_EXTs, device_LAYERs);
  fmt::println("gui_Device created");

  create_IMGUI_DescriptorPoor();
  fmt::println("gui_DescriptorPool created");

  Core::Source::SwapChain::SwapChainRequirement swap_chain_requirements{&gui_Device,
                                                                        gui_Surface.window};

  gui_SwapChain(&gui_Device, swap_chain_requirements);
  // model_SwapChain(&gui_Device, swap_chain_requirements);

  fmt::println("gui_SwapChain created");

  semaphore_count = Core::Macro::MAX_FRAMES_IN_FLIGHT;
  Image_count = swap_chain_requirements.img_count;

  std::vector<VkAttachmentDescription> model_attachments{
      Core::Render::RenderPass::Default_ColorAttachment(
          swap_chain_requirements.surface_format.format),
      Core::Render::RenderPass::Default_DepthAttachment(gui_Device.pPD->physical_device)};

  VkAttachmentReference color_attachment = {};
  color_attachment.attachment = 0;
  color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  VkAttachmentReference depth_attachment_ref{};
  depth_attachment_ref.attachment = 1;
  depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkSubpassDescription model_subpass{};
  model_subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  model_subpass.colorAttachmentCount = 1;
  model_subpass.pColorAttachments = &color_attachment;
  model_subpass.pDepthStencilAttachment = &depth_attachment_ref;

  VkSubpassDescription gui_subpass{};
  gui_subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  gui_subpass.colorAttachmentCount = 1;
  gui_subpass.pColorAttachments = &color_attachment;
  gui_subpass.pDepthStencilAttachment = &depth_attachment_ref;

  std::vector<VkSubpassDescription> total_subpass{model_subpass, gui_subpass};

  main_RenderPass(&gui_Device, GUI_SUBPASS_DEPENDENCY(), total_subpass, model_attachments);
  fmt::println("main_RenderPass created");

  gui_DepthResource(&gui_Device,
                    VkExtent3D{gui_SwapChain.extent.width, gui_SwapChain.extent.height, 1});

  model_DepthResource(&gui_Device,
                      VkExtent3D{gui_SwapChain.extent.width, gui_SwapChain.extent.height, 1});

  std::vector<VkImageView> additional_views{gui_DepthResource.engine_ImageView.image_view};

  // --------------------  FrameBuffers  --------------------

  gui_SwapChain.Create_FrameBuffers(additional_views, &main_RenderPass);
  fmt::println("FrameBuffers created");

  // ---------------------  Command  ------------------------

  gui_CommandPool(
      &gui_Device,
      Core::Data::CommandPoolCreate_Info(gui_Device.queue_family.graphicsFamily.value()));
  fmt::println("gui_CommandPool created");

  gui_CommandBuffers.resize(Core::Macro::MAX_FRAMES_IN_FLIGHT);
  for (int i = 0; i < Core::Macro::MAX_FRAMES_IN_FLIGHT; i++)
    {
      gui_CommandBuffers[i](&gui_Device, gui_CommandPool.command_pool);
    }
  fmt::println("gui_CommandBuffers created");

  // ---------------------  Unibuffer  ------------------------

  model_UniBuffer(&gui_Device, gui_Device.graphics_queue);
  // prepare for uniform binding
  {
    std::vector<VkDescriptorPoolSize> poolSizes = {
        Core::Source::Descriptor::Get_DescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
        Core::Source::Descriptor::Get_DescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                         1)};

    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
        Core::Source::Descriptor::GetLayoutBinding(
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0)};

    uni_pool(&gui_Device, 2, poolSizes);
    uni_setlayout(&gui_Device, setLayoutBindings);
    uni_set(&gui_Device, &uni_setlayout, &uni_pool);

    std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
        // Binding 0 : Vertex shader uniform buffer
        Core::Source::Descriptor::GetDescriptSet_Write(uni_set.descriptor_set,
                                                       VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                       0,
                                                       &model_UniBuffer.descriptor)};
    uni_set.updateWrite(writeDescriptorSets);
  }
  // ---------------------  Models  ------------------------

  load_model();

  // --------------------- Pipeline ------------------------

  construct_pipeline();

  // --------------------- Semaphore ------------------------

  gui_Fences(&gui_Device, Core::Macro::MAX_FRAMES_IN_FLIGHT);
  fmt::println("gui_Fences created");

  gui_ImageAcquiredSemaphores(&gui_Device, semaphore_count);
  gui_RenderCompleteSemaphores(&gui_Device, semaphore_count);
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
  init_info.RenderPass = main_RenderPass.render_pass;
  init_info.Subpass = 1;
  init_info.MinImageCount = frames_InFlight;
  init_info.ImageCount = Core::Macro::MAX_FRAMES_IN_FLIGHT;
  init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
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
          gui_SwapChain.Create_FrameBuffers({}, &main_RenderPass);

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

          auto t{std::chrono::duration<float, std::milli>(time_end - time_start).count() / 1000.0f};
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
          gui_Clearvalue[1].depthStencil = {1.0f, 0};

          Render_Frame(draw_data, gui_Clearvalue);
          Present_Frame();
        }
    }
  check_vk_result(vkDeviceWaitIdle(gui_Device.logical_device));

  gui_ImageAcquiredSemaphores.destroyer();
  gui_RenderCompleteSemaphores.destroyer();
  gui_DescriptorPool.destroyer();
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
                                                       std::array<VkClearValue, 2> gui_Clearvalue)
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
    info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    err = vkBeginCommandBuffer(gui_CommandBuffers[Frame_Index].command_buffer, &info);
    check_vk_result(err);
  }

  VkRenderPassBeginInfo render_pass_begin_info{};

  render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_begin_info.renderPass = main_RenderPass.render_pass;
  render_pass_begin_info.framebuffer = gui_SwapChain.frame_buffers[imageIndex];

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

    sky_box.model.bind_buffers(gui_CommandBuffers[Frame_Index].command_buffer);
    sky_box.model.draw(gui_CommandBuffers[Frame_Index].command_buffer,
                       skybox_Pipelinelayout.pipeline_layout);

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

    vkCmdNextSubpass(gui_CommandBuffers[Frame_Index].command_buffer, VK_SUBPASS_CONTENTS_INLINE);
    // Record dear imgui primitives into command buffer
    ImGui_ImplVulkan_RenderDrawData(draw_data, gui_CommandBuffers[Frame_Index].command_buffer);
  }

  // Submit command buffer
  vkCmdEndRenderPass(gui_CommandBuffers[Frame_Index].command_buffer);

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
  VkPipelineInputAssemblyStateCreateInfo input_assembly{Core::Data::GetInputAssembly_Info()};
  VkPipelineViewportStateCreateInfo viewport_state{
      Core::Data::GetViewportState_Info(gui_SwapChain.extent)};
  std::vector<VkDynamicState> dynamic_states{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dynamic_state{Core::Data::GetDynamicState_Info(dynamic_states)};
  VkPipelineRasterizationStateCreateInfo raterizer{Core::Data::DEFAULT_RASTERIZER_INFO()};
  VkPipelineMultisampleStateCreateInfo multisampling{Core::Data::DEFAULT_MULTISAMPLING_INFO()};
  VkPipelineDepthStencilStateCreateInfo depth_stencil{Core::Data::DEFAULT_DEPTHSTENCIL_INFO()};

  auto color_blend_attachment_info{Core::Data::GetDefaultColorBlend_Attachment()};
  VkPipelineColorBlendStateCreateInfo color_blend{
      Core::Data::DEFAULT_COLORBLEND_INFO(color_blend_attachment_info)};

  // -------------------- pipeline layout ---------------------

  std::vector<VkPushConstantRange> ranges{};
  model_Pipelinelayout(
      &gui_Device,
      std::vector<VkDescriptorSetLayout>{uni_setlayout.layout, old_school.layouts.texture.layout},
      ranges);

  skybox_Pipelinelayout(
      &gui_Device,
      std::vector<VkDescriptorSetLayout>{uni_setlayout.layout, skybox_setlayout.layout},
      ranges);

  // -------------------- shader code ---------------------
  std::string vert_code{Core::Utils::read_file(MODEL_VertexShader_code).data()};
  auto vertex_shader_module{
      Core::Utils::Glsl_ShaderCompiler(gui_Device.logical_device, EShLangVertex, vert_code)};
  std::string frag_code{Core::Utils::read_file(MODEL_FragmentShader_code).data()};
  auto fragment_shader_module{
      Core::Utils::Glsl_ShaderCompiler(gui_Device.logical_device, EShLangFragment, frag_code)};
  VkPipelineShaderStageCreateInfo vertex_stage{
      Core::Source::Pipeline::Get_VertexShader_CreateInfo("main", vertex_shader_module)};
  VkPipelineShaderStageCreateInfo frag_stage{
      Core::Source::Pipeline::Get_FragmentShader_CreateInfo("main", fragment_shader_module)};

  std::string skybox_vert_code{Core::Utils::read_file(SKYBOX_VertexShader_code).data()};
  auto skybox_vertex_shader_module{
      Core::Utils::Glsl_ShaderCompiler(gui_Device.logical_device, EShLangVertex, skybox_vert_code)};
  std::string skybox_frag_code{Core::Utils::read_file(SKYBOX_FragmentShader_code).data()};
  auto skybox_fragment_shader_module{Core::Utils::Glsl_ShaderCompiler(
      gui_Device.logical_device, EShLangFragment, skybox_frag_code)};
  VkPipelineShaderStageCreateInfo skybox_vertex_stage{
      Core::Source::Pipeline::Get_VertexShader_CreateInfo("main", skybox_vertex_shader_module)};
  VkPipelineShaderStageCreateInfo skybox_frag_stage{
      Core::Source::Pipeline::Get_FragmentShader_CreateInfo("main", skybox_fragment_shader_module)};

  std::vector<VkPipelineShaderStageCreateInfo> shader_stages{vertex_stage, frag_stage};
  std::vector<VkPipelineShaderStageCreateInfo> skybox_shader_stages{skybox_vertex_stage,
                                                                    skybox_frag_stage};

  // -------------------- pipeline initialization ---------------------

  Core::Data::PipelinePreparation_Info pipeline_info{vertex_input,
                                                     input_assembly,
                                                     viewport_state,
                                                     dynamic_state,
                                                     raterizer,
                                                     multisampling,
                                                     depth_stencil,
                                                     color_blend};

  model_GraphicPipeline(
      &gui_Device, &model_Pipelinelayout, &main_RenderPass, shader_stages, &pipeline_info, 0);

  auto sky_box_attribute_descriptions =
      Core::Source::Model::GLTF_EngineModelVertexData::getAttributeDescriptions(
          Core::Source::Model::GLTF_EngineModelVertexData::POS, 0);
  VkPipelineVertexInputStateCreateInfo sky_box_vertex_input{
      Core::Data::GetVertexInput_Info(binding_description, sky_box_attribute_descriptions)};

  pipeline_info.vertex_input = sky_box_vertex_input;
  pipeline_info.depth_stencil = Core::Data::DEFAULT_DEPTHSTENCIL_DISABLED_INFO();
  pipeline_info.rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;

  skybox_GraphicPipeline(&gui_Device,
                         &skybox_Pipelinelayout,
                         &main_RenderPass,
                         skybox_shader_stages,
                         &pipeline_info,
                         0);
}

void SngoEngine::Imgui::ImguiApplication::load_model()
{
  old_school(MAIN_OLD_SCHOOL, &gui_Device, &gui_CommandPool);
  sky_box(CUBEMAP_FILE, CUBEMAP_TEXTURE, &gui_Device, &gui_CommandPool);

  sky_box.generate_descriptor(uni_pool, skybox_setlayout, skybox_set, 1);
}

void SngoEngine::Imgui::ImguiApplication::update_uniform_buffer(uint32_t current_frame)
{
  Core::Source::Buffer::UniformBuffer_Trans ubo{};
  ubo.projection = main_Camera.matrices.perspective;
  ubo.modelView = main_Camera.matrices.view;
  ubo.inverseModelview = glm::inverse(main_Camera.matrices.view);
  ubo.lodBias = 0.0f;

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
