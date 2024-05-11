#ifndef __SNGO_UTILS_H
#define __SNGO_UTILS_H

#include <glslang/Include/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <io.h>

#include <cassert>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "vulkan/vulkan_core.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.hpp"

namespace SngoEngine::Core::Utils
{

std::vector<char> read_file(const std::string& file_name);

void glob_file(const std::string& direcctory,
               const std::string& format,
               std::vector<std::string>& files);
FILE* FOpenRead(const std::string& filename);
std::vector<float> Read_FloatFile(const std::string& filename);
std::string GetFile_Extension(const std::string& filename);
bool isFile_Exists(const std::string& filename);

void Vk_Exception(VkResult res);
bool Atof(std::string_view str, float* ptr);
bool Atof(std::string_view str, double* ptr);

VkShaderModule Glsl_ShaderCompiler(VkDevice device,
                                   EShLanguage stage,
                                   const std::string& shader_source);

template <typename V, typename I>
void Load_Vetex_Index(const std::string& obj_file,
                      std::vector<V>& vertices,
                      std::vector<I>& indices)
{
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string warn, err;

  if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, obj_file.c_str()))
    {
      throw std::runtime_error(err);
    }

  std::unordered_map<V, I> uniqueVertices{};

  for (const auto& shape : shapes)
    {
      for (const auto& index : shape.mesh.indices)
        {
          V vertex{};

          vertex.pos = {attrib.vertices[3 * index.vertex_index + 0],
                        attrib.vertices[3 * index.vertex_index + 1],
                        attrib.vertices[3 * index.vertex_index + 2]};

          vertex.tex_coord = {attrib.texcoords[2 * index.texcoord_index + 0],
                              1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};

          vertex.color = {1.0f, 1.0f, 1.0f};

          if (uniqueVertices.count(vertex) == 0)
            {
              uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
              vertices.push_back(vertex);
            }

          indices.push_back(uniqueVertices[vertex]);
        }
    }
}

//===========================================================================================================================
// TypePack Operations
//===========================================================================================================================

template <typename... Ts>
struct TypePack
{
  static constexpr size_t count = sizeof...(Ts);
};

template <typename T, typename... Ts>
struct IndexOf
{
  static constexpr int count = 0;
  static_assert(!std::is_same_v<T, T>, "Type not present in TypePack");
};

template <typename T, typename... Ts>
struct IndexOf<T, TypePack<T, Ts...>>
{
  static constexpr int count = 0;
};

template <typename T, typename U, typename... Ts>
struct IndexOf<T, TypePack<U, Ts...>>
{
  static constexpr int count = 1 + IndexOf<T, TypePack<Ts...>>::count;
};

//===========================================================================================================================
// IsSameType
//===========================================================================================================================

template <typename... Ts>
struct IsSameType;
template <>
struct IsSameType<>
{
  static constexpr bool value = true;
};
template <typename T>
struct IsSameType<T>
{
  static constexpr bool value = true;
};

template <typename T, typename U, typename... Ts>
struct IsSameType<T, U, Ts...>
{
  static constexpr bool value = (std::is_same_v<T, U> && IsSameType<U, Ts...>::value);
};

template <typename... Ts>
struct SameType;

template <typename T, typename... Ts>
struct SameType<T, Ts...>
{
  using type = T;
  static_assert(IsSameType<T, Ts...>::value, "Not all types in pack are the same");
};

//===========================================================================================================================
// ReturnType
//===========================================================================================================================

template <typename F, typename... Ts>
struct ReturnType
{
  using type = typename SameType<typename std::invoke_result_t<F, Ts*>...>::type;
};

template <typename F, typename... Ts>
struct ReturnTypeConst
{
  using type = typename SameType<typename std::invoke_result_t<F, const Ts*>...>::type;
};

//===========================================================================================================================
// Dispatch
//===========================================================================================================================

template <typename F, typename R, typename T>
R Dispatch(F&& func, void* ptr, int index)
{
  assert(0 == index);
  return func((T*)ptr);
}

template <typename F, typename R, typename T0, typename T1>
R Dispatch(F&& func, void* ptr, int index)
{
  assert(index >= 0);
  assert(index >= 2);

  if (index == 0)
    return func((T0*)ptr);
  else
    return func((T1*)ptr);
}

template <typename F, typename R, typename T0, typename T1, typename T2>
R Dispatch(F&& func, void* ptr, int index)
{
  assert(index >= 0);
  assert(index >= 3);

  switch (index)
    {
      case 0:
        return func((T0*)ptr);
      case 1:
        return func((T1*)ptr);
      default:
        return func((T2*)ptr);
    }
}

template <typename F, typename R, typename T0, typename T1, typename T2, typename T3>
R Dispatch(F&& func, void* ptr, int index)
{
  assert(index >= 0);
  assert(index >= 4);

  switch (index)
    {
      case 0:
        return func((T0*)ptr);
      case 1:
        return func((T1*)ptr);
      case 2:
        return func((T2*)ptr);
      default:
        return func((T3*)ptr);
    }
}

template <typename F, typename R, typename T0, typename T1, typename T2, typename T3, typename T4>
R Dispatch(F&& func, void* ptr, int index)
{
  assert(index >= 0);
  assert(index <= 5);

  switch (index)
    {
      case 0:
        return func((T0*)ptr);
      case 1:
        return func((T1*)ptr);
      case 2:
        return func((T2*)ptr);
      case 3:
        return func((T3*)ptr);
      default:
        return func((T4*)ptr);
    }
}

template <typename F,
          typename R,
          typename T0,
          typename T1,
          typename T2,
          typename T3,
          typename T4,
          typename... Ts,
          typename = typename std::enable_if_t<(sizeof...(Ts) > 0)>>
R Dispatch(F&& func, void* ptr, int index)
{
  assert(index >= 0);

  switch (index)
    {
      case 0:
        return func((T0*)ptr);
      case 1:
        return func((T1*)ptr);
      case 2:
        return func((T2*)ptr);
      case 3:
        return func((T3*)ptr);
      case 4:
        return func((T4*)ptr);
      default:
        return Dispatch<F, R, Ts...>(func, ptr, index - 5);
    }
}

template <typename F, typename R, typename T>
R Dispatch(F&& func, const void* ptr, int index)
{
  assert(0 == index);
  return func((const T*)ptr);
}

template <typename F, typename R, typename T0, typename T1>
R Dispatch(F&& func, const void* ptr, int index)
{
  assert(index >= 0);
  assert(index >= 2);

  if (index == 0)
    return func((const T0*)ptr);
  else
    return func((const T1*)ptr);
}

template <typename F, typename R, typename T0, typename T1, typename T2>
R Dispatch(F&& func, const void* ptr, int index)
{
  assert(index >= 0);
  assert(index >= 3);

  switch (index)
    {
      case 0:
        return func((const T0*)ptr);
      case 1:
        return func((const T1*)ptr);
      default:
        return func((const T2*)ptr);
    }
}

template <typename F, typename R, typename T0, typename T1, typename T2, typename T3>
R Dispatch(F&& func, const void* ptr, int index)
{
  assert(index >= 0);
  assert(index >= 4);

  switch (index)
    {
      case 0:
        return func((const T0*)ptr);
      case 1:
        return func((const T1*)ptr);
      case 2:
        return func((const T2*)ptr);
      default:
        return func((const T3*)ptr);
    }
}

template <typename F, typename R, typename T0, typename T1, typename T2, typename T3, typename T4>
R Dispatch(F&& func, const void* ptr, int index)
{
  assert(index >= 0);
  assert(index <= 5);

  switch (index)
    {
      case 0:
        return func((const T0*)ptr);
      case 1:
        return func((const T1*)ptr);
      case 2:
        return func((const T2*)ptr);
      case 3:
        return func((const T3*)ptr);
      default:
        return func((const T4*)ptr);
    }
}

template <typename F,
          typename R,
          typename T0,
          typename T1,
          typename T2,
          typename T3,
          typename T4,
          typename... Ts,
          typename = typename std::enable_if_t<(sizeof...(Ts) > 0)>>
R Dispatch(F&& func, const void* ptr, int index)
{
  assert(index >= 0);

  switch (index)
    {
      case 0:
        return func((const T0*)ptr);
      case 1:
        return func((const T1*)ptr);
      case 2:
        return func((const T2*)ptr);
      case 3:
        return func((const T3*)ptr);
      case 4:
        return func((const T4*)ptr);
      default:
        return Dispatch<F, R, Ts...>(func, ptr, index - 5);
    }
}

const TBuiltInResource DefaultTBuiltInResource = {
    /* .MaxLights = */ 32,
    /* .MaxClipPlanes = */ 6,
    /* .MaxTextureUnits = */ 32,
    /* .MaxTextureCoords = */ 32,
    /* .MaxVertexAttribs = */ 64,
    /* .MaxVertexUniformComponents = */ 4096,
    /* .MaxVaryingFloats = */ 64,
    /* .MaxVertexTextureImageUnits = */ 32,
    /* .MaxCombinedTextureImageUnits = */ 80,
    /* .MaxTextureImageUnits = */ 32,
    /* .MaxFragmentUniformComponents = */ 4096,
    /* .MaxDrawBuffers = */ 32,
    /* .MaxVertexUniformVectors = */ 128,
    /* .MaxVaryingVectors = */ 8,
    /* .MaxFragmentUniformVectors = */ 16,
    /* .MaxVertexOutputVectors = */ 16,
    /* .MaxFragmentInputVectors = */ 15,
    /* .MinProgramTexelOffset = */ -8,
    /* .MaxProgramTexelOffset = */ 7,
    /* .MaxClipDistances = */ 8,
    /* .MaxComputeWorkGroupCountX = */ 65535,
    /* .MaxComputeWorkGroupCountY = */ 65535,
    /* .MaxComputeWorkGroupCountZ = */ 65535,
    /* .MaxComputeWorkGroupSizeX = */ 1024,
    /* .MaxComputeWorkGroupSizeY = */ 1024,
    /* .MaxComputeWorkGroupSizeZ = */ 64,
    /* .MaxComputeUniformComponents = */ 1024,
    /* .MaxComputeTextureImageUnits = */ 16,
    /* .MaxComputeImageUniforms = */ 8,
    /* .MaxComputeAtomicCounters = */ 8,
    /* .MaxComputeAtomicCounterBuffers = */ 1,
    /* .MaxVaryingComponents = */ 60,
    /* .MaxVertexOutputComponents = */ 64,
    /* .MaxGeometryInputComponents = */ 64,
    /* .MaxGeometryOutputComponents = */ 128,
    /* .MaxFragmentInputComponents = */ 128,
    /* .MaxImageUnits = */ 8,
    /* .MaxCombinedImageUnitsAndFragmentOutputs = */ 8,
    /* .MaxCombinedShaderOutputResources = */ 8,
    /* .MaxImageSamples = */ 0,
    /* .MaxVertexImageUniforms = */ 0,
    /* .MaxTessControlImageUniforms = */ 0,
    /* .MaxTessEvaluationImageUniforms = */ 0,
    /* .MaxGeometryImageUniforms = */ 0,
    /* .MaxFragmentImageUniforms = */ 8,
    /* .MaxCombinedImageUniforms = */ 8,
    /* .MaxGeometryTextureImageUnits = */ 16,
    /* .MaxGeometryOutputVertices = */ 256,
    /* .MaxGeometryTotalOutputComponents = */ 1024,
    /* .MaxGeometryUniformComponents = */ 1024,
    /* .MaxGeometryVaryingComponents = */ 64,
    /* .MaxTessControlInputComponents = */ 128,
    /* .MaxTessControlOutputComponents = */ 128,
    /* .MaxTessControlTextureImageUnits = */ 16,
    /* .MaxTessControlUniformComponents = */ 1024,
    /* .MaxTessControlTotalOutputComponents = */ 4096,
    /* .MaxTessEvaluationInputComponents = */ 128,
    /* .MaxTessEvaluationOutputComponents = */ 128,
    /* .MaxTessEvaluationTextureImageUnits = */ 16,
    /* .MaxTessEvaluationUniformComponents = */ 1024,
    /* .MaxTessPatchComponents = */ 120,
    /* .MaxPatchVertices = */ 32,
    /* .MaxTessGenLevel = */ 64,
    /* .MaxViewports = */ 16,
    /* .MaxVertexAtomicCounters = */ 0,
    /* .MaxTessControlAtomicCounters = */ 0,
    /* .MaxTessEvaluationAtomicCounters = */ 0,
    /* .MaxGeometryAtomicCounters = */ 0,
    /* .MaxFragmentAtomicCounters = */ 8,
    /* .MaxCombinedAtomicCounters = */ 8,
    /* .MaxAtomicCounterBindings = */ 1,
    /* .MaxVertexAtomicCounterBuffers = */ 0,
    /* .MaxTessControlAtomicCounterBuffers = */ 0,
    /* .MaxTessEvaluationAtomicCounterBuffers = */ 0,
    /* .MaxGeometryAtomicCounterBuffers = */ 0,
    /* .MaxFragmentAtomicCounterBuffers = */ 1,
    /* .MaxCombinedAtomicCounterBuffers = */ 1,
    /* .MaxAtomicCounterBufferSize = */ 16384,
    /* .MaxTransformFeedbackBuffers = */ 4,
    /* .MaxTransformFeedbackInterleavedComponents = */ 64,
    /* .MaxCullDistances = */ 8,
    /* .MaxCombinedClipAndCullDistances = */ 8,
    /* .MaxSamples = */ 4,
    /* .maxMeshOutputVerticesNV = */ 256,
    /* .maxMeshOutputPrimitivesNV = */ 512,
    /* .maxMeshWorkGroupSizeX_NV = */ 32,
    /* .maxMeshWorkGroupSizeY_NV = */ 1,
    /* .maxMeshWorkGroupSizeZ_NV = */ 1,
    /* .maxTaskWorkGroupSizeX_NV = */ 32,
    /* .maxTaskWorkGroupSizeY_NV = */ 1,
    /* .maxTaskWorkGroupSizeZ_NV = */ 1,
    /* .maxMeshViewCountNV = */ 4,
    /* .maxMeshOutputVerticesEXT = */ 256,
    /* .maxMeshOutputPrimitivesEXT = */ 512,
    /* .maxMeshWorkGroupSizeX_EXT = */ 32,
    /* .maxMeshWorkGroupSizeY_EXT = */ 1,
    /* .maxMeshWorkGroupSizeZ_EXT = */ 1,
    /* .maxTaskWorkGroupSizeX_EXT = */ 32,
    /* .maxTaskWorkGroupSizeY_EXT = */ 1,
    /* .maxTaskWorkGroupSizeZ_EXT = */ 1,
    /* .maxMeshViewCountEXT = */ 4,
    /* .maxDualSourceDrawBuffersEXT = */ 1,

    /* .limits = */
    {
        /* .nonInductiveForLoops = */ true,
        /* .whileLoops = */ true,
        /* .doWhileLoops = */ true,
        /* .generalUniformIndexing = */ true,
        /* .generalAttributeMatrixVectorIndexing = */ true,
        /* .generalVaryingIndexing = */ true,
        /* .generalSamplerIndexing = */ true,
        /* .generalVariableIndexing = */ true,
        /* .generalConstantMatrixVectorIndexing = */ true,
    }};
}  // namespace SngoEngine::Core::Utils

#endif