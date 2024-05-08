#include "Utils.hpp"

#include <cstdlib>
#include <fstream>
#include <ios>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "src/Core/Device/LogicalDevice.hpp"

std::vector<char> SngoEngine::Core::Utils::read_file(const std::string& file_name)
{
  std::ifstream file(file_name, std::ios::ate | std::ios::binary);

  if (!file.is_open())
    {
      throw std::runtime_error("failed to open file!");
    }
  size_t size{static_cast<size_t>(file.tellg())};
  std::vector<char> buffer(size + 1, '\0');
  file.seekg(0);
  file.read(buffer.data(), size);

  file.close();
  return buffer;
}

void SngoEngine::Core::Utils::glob_file(const std::string& direcctory,
                                        const std::string& format,
                                        std::vector<std::string>& files)
{
  if (!files.empty())
    files.clear();

  intptr_t hFile{0};
  _finddata_t fileInfo;
  std::string p;

  hFile = _findfirst(p.assign(direcctory).append("\\*" + format).c_str(), &fileInfo);

  if (hFile != -1)
    {
      do
        {
          files.push_back(p.assign(direcctory).append("\\").append(fileInfo.name));
      } while (_findnext(hFile, &fileInfo) == 0);

      _findclose(hFile);
    }
}

FILE* SngoEngine::Core::Utils::FOpenRead(const std::string& filename)
{
  return fopen(filename.c_str(), "rb");
}

std::vector<float> SngoEngine::Core::Utils::Read_FloatFile(std::string filename)
{
  FILE* f = FOpenRead(filename);
  if (f == nullptr)
    {
      throw std::runtime_error("Unable to open file " + filename);
      return {};
    }

  int c;
  bool inNumber = false;
  char curNumber[32];
  int curNumberPos = 0;
  int lineNumber = 1;
  std::vector<float> values;
  while ((c = getc(f)) != EOF)
    {
      if (c == '\n')
        ++lineNumber;
      if (inNumber)
        {
          // Note: this is not very robust, and would accept something
          // like 0.0.0.0eeee-+--2 as a valid number.
          if ((isdigit(c) != 0) || c == '.' || c == 'e' || c == 'E' || c == '-' || c == '+')
            {
              assert(curNumberPos <= sizeof(curNumber));
              curNumber[curNumberPos++] = c;
            }
          else
            {
              curNumber[curNumberPos++] = '\0';
              float v{};
              if (Atof(curNumber, &v))
                throw std::runtime_error("UInable to parse float value in" + filename + " in "
                                         + curNumber);
              values.push_back(v);
              inNumber = false;
              curNumberPos = 0;
            }
        }
      else
        {
          if ((isdigit(c) != 0) || c == '.' || c == '-' || c == '+')
            {
              inNumber = true;
              curNumber[curNumberPos++] = c;
            }
          else if (c == '#')
            {
              while ((c = getc(f)) != '\n' && c != EOF)
                ;
              ++lineNumber;
            }
          else if (isspace(c) == 0)
            {
              throw std::runtime_error(filename + ": unexpected character " + std::to_string(c)
                                       + " found at line" + std::to_string(lineNumber));
              return {};
            }
        }
    }
  fclose(f);
  return values;
}

void SngoEngine::Core::Utils::Vk_Exception(VkResult res)
{
  if (!res)
    return;
  fprintf(stderr, "[err] vulkan result: %d\n", res);
}

VkShaderModule SngoEngine::Core::Utils::Glsl_ShaderCompiler(VkDevice device,
                                                            EShLanguage stage,
                                                            const std::string& shader_source)
{
  glslang::InitializeProcess();

  glslang::TShader shader{stage};
  std::vector<uint32_t> SPV_code{};

  auto psource{shader_source.data()};
  shader.setStrings(&psource, 1);
  shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientVulkan, 100);
  shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_2);
  shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_3);

  if (!shader.parse(&DefaultTBuiltInResource, 100, ENoProfile, false, false, EShMsgDefault))
    {
      throw std::runtime_error("failed to parse shader: " + shader_source);
    }

  glslang::TProgram program{};
  program.addShader(&shader);
  if (!program.link(EShMsgDefault))
    {
      throw std::runtime_error(program.getInfoLog());
    }
  const auto intermediate{program.getIntermediate(stage)};
  glslang::GlslangToSpv(*intermediate, SPV_code);

  VkShaderModuleCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  create_info.codeSize = SPV_code.size() * sizeof(uint32_t);
  create_info.pCode = reinterpret_cast<const uint32_t*>(SPV_code.data());

  VkShaderModule shader_module{};
  if (vkCreateShaderModule(device, &create_info, nullptr, &shader_module) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create shader module!");
    }
  glslang::FinalizeProcess();
  return shader_module;
}

bool SngoEngine::Core::Utils::Atof(std::string_view str, float* ptr)
{
  try
    {
      *ptr = std::stof(std::string(str.begin(), str.end()));
    }
  catch (...)
    {
      return false;
    }
  return true;
}

bool SngoEngine::Core::Utils::Atof(std::string_view str, double* ptr)
{
  try
    {
      *ptr = std::stod(std::string(str.begin(), str.end()));
    }
  catch (...)
    {
      return false;
    }
  return true;
}
