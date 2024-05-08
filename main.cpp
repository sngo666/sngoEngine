#include <cstdlib>
#include <exception>
#include <iostream>
#include <ostream>

#include "src/IMGUI/Imgui.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <fmt/core.h>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

int main()
{
  SngoEngine::Imgui::ImguiApplication app;
  app.init();
}