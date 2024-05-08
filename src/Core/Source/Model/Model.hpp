#ifndef __SNGO_MODEL_H
#define __SNGO_MODEL_H

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#ifdef VK_USE_PLATFORM_ANDROID_KHR
#define TINYGLTF_ANDROID_LOAD_FROM_ASSETS
#endif

#include "fmt/core.h"
#include "src/Core/Device/LogicalDevice.hpp"
#include "src/Core/Macro.h"
#include "src/Core/Source/Buffer/CommandBuffer.hpp"
#include "src/Core/Source/Buffer/Descriptor.hpp"
#include "src/Core/Source/Buffer/IndexBuffer.hpp"
#include "src/Core/Source/Buffer/UniformBuffer.hpp"
#include "src/Core/Source/Buffer/VertexBuffer.hpp"
#include "src/Core/Source/Image/Image.hpp"
#include "src/Core/Source/Image/ImageVIew.hpp"
#include "src/Core/Source/Image/Sampler.hpp"
#include "src/Core/Utils/Utils.hpp"
#include "src/Core/Utils/tiny_gltf.hpp"

template <typename T, typename N>
concept ModelTemplate = std::is_integral_v<decltype(T::attribute_count)> && std::is_integral_v<N>;

namespace SngoEngine::Core::Source::Model
{

//===========================================================================================================================
// EngineObj
//===========================================================================================================================

template <typename V, typename I>
  requires(ModelTemplate<V, I> && VertexStructure<V, V::attribute_count>)
struct EngineObj
{
  EngineObj() = default;
  EngineObj(EngineObj&&) noexcept = default;
  EngineObj& operator=(EngineObj&&) noexcept = default;
  template <class... Args>
  explicit EngineObj(const std::string& obj_file, Args... args)
  {
    creator(obj_file, args...);
  }
  template <class... Args>
  void operator()(const std::string& obj_file, Args... args)
  {
    creator(obj_file, args...);
  }
  ~EngineObj() = default;
  void load_buffer(const Device::LogicalDevice::EngineDevice* _device,
                   VkCommandPool _vk_pool,
                   VkQueue _graphic_queue)
  {
    if (vertices.empty() || indices.empty())
      return;

    vertex_buffer(_device, _vk_pool, _graphic_queue, vertices, Alloc);
    index_buffer(_device, _vk_pool, _graphic_queue, indices, Alloc);
  }

  std::vector<V> vertices;
  std::vector<I> indices;
  Buffer::EngineVertexBuffer<V, V::attribute_count> vertex_buffer;
  Buffer::EngineIndexBuffer<I> index_buffer;

 private:
  void creator(const std::string& obj_file, const VkAllocationCallbacks* alloc = nullptr)
  {
    Alloc = alloc;

    Utils::Load_Vetex_Index<V, I>(obj_file, vertices, indices);
  }

  void creator(const std::string& obj_file,
               const Device::LogicalDevice::EngineDevice* _device,
               const Buffer::EngineCommandPool* _pool,
               VkQueue _graphic_queue,
               const VkAllocationCallbacks* alloc = nullptr)
  {
    creator(obj_file, alloc);
    load_buffer(_device, _pool, _graphic_queue);
  }

  void creator(const std::string& obj_file,
               const Device::LogicalDevice::EngineDevice* _device,
               VkCommandPool _vk_pool,
               VkQueue _graphic_queue,
               const VkAllocationCallbacks* alloc = nullptr)
  {
    creator(obj_file, alloc);
    load_buffer(_device, _vk_pool, _graphic_queue);
  }

  const VkAllocationCallbacks* Alloc{};
};

//===========================================================================================================================
// EngineObjModel
//===========================================================================================================================

template <typename V, typename I, typename U>
struct EngineObjModel
{
  EngineObjModel() = default;
  EngineObjModel(EngineObjModel&&) noexcept = default;
  EngineObjModel& operator=(EngineObjModel&&) noexcept = default;
  template <typename... Args>
  explicit EngineObjModel(const std::string& Obj_file,
                          const std::vector<std::string>& Text_file,
                          const Device::LogicalDevice::EngineDevice* _device,
                          Args... args)
  {
    creator(Obj_file, Text_file, _device, args...);
  }
  template <typename... Args>
  void operator()(const std::string& Obj_file,
                  const std::vector<std::string>& Text_file,
                  const Device::LogicalDevice::EngineDevice* _device,
                  Args... args)
  {
    creator(Obj_file, Text_file, _device, args...);
  }
  ~EngineObjModel() = default;

  EngineObj<V, I> Model;
  std::vector<Image::EngineTextureImage> texture_imgs;
  std::vector<Image::EngineSampler> texture_samplers;
  Descriptor::EngineDescriptorPool descriptor_pool{};
  Descriptor::EngineDescriptorSets descriptor_sets;

  const Device::LogicalDevice::EngineDevice* device{};

 private:
  void creator(const std::string& Obj_file,
               const std::vector<std::string>& Text_file,
               const Device::LogicalDevice::EngineDevice* _device,
               VkCommandPool _pool,
               const Descriptor::EngineDescriptorSetLayout* _set_layout,
               const std::vector<Buffer::EngineUniformBuffer<U>>* _uniform_buffers,
               uint32_t max_sets = Macro::MAX_FRAMES_IN_FLIGHT,
               const VkAllocationCallbacks* alloc = nullptr)
  {
    device = _device;
    Alloc = alloc;

    auto _size(Text_file.size());
    descriptor_pool(device, max_sets, _size, Alloc);

    texture_imgs.resize(Text_file.size());
    for (int i = 0; i < _size; i++)
      {
        texture_imgs[i](device, _pool, Text_file[i], Alloc);
      }

    auto sampler_info{Image::Get_Default_Sampler(device)};
    texture_samplers.resize(_size);
    for (auto& sampler : texture_samplers)
      {
        sampler(device, sampler_info, Alloc);
      }

    std::vector<VkDescriptorImageInfo> _img_infos;
    _img_infos.resize(_size);
    for (int i = 0; i < _size; i++)
      {
        _img_infos[i] = VkDescriptorImageInfo{texture_samplers[i].sampler,
                                              texture_imgs[i].view,
                                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
      }

    std::vector<VkDescriptorBufferInfo> _buffer_infos;
    _buffer_infos.resize(Macro::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < Macro::MAX_FRAMES_IN_FLIGHT; i++)
      {
        _buffer_infos[i] = {
            _uniform_buffers->operator[](i).buffer, 0, _uniform_buffers->operator[](i).range};
      }

    descriptor_sets(device, _set_layout, &descriptor_pool, Macro::MAX_FRAMES_IN_FLIGHT);

    std::vector<VkWriteDescriptorSet> writes(Macro::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < Macro::MAX_FRAMES_IN_FLIGHT; i++)
      {
        writes[i] =
            Descriptor::GetDescriptSet_Write(_buffer_infos[i], _img_infos, descriptor_sets[i])[0];
      }
    descriptor_sets.updateWrite(writes);

    Model(Obj_file);
  }
  void creator(const std::string& Obj_file,
               const std::vector<std::string>& Text_file,
               const Device::LogicalDevice::EngineDevice* _device,
               const Buffer::EngineCommandPool* _pool,
               const Descriptor::EngineDescriptorSetLayout* _set_layout,
               const std::vector<Buffer::EngineUniformBuffer<U>>* _uniform_buffers,
               uint32_t max_sets = Macro::MAX_FRAMES_IN_FLIGHT,
               const VkAllocationCallbacks* alloc = nullptr)
  {
    creator(Obj_file,
            Text_file,
            _device,
            _pool->command_pool,
            _set_layout,
            _uniform_buffers,
            max_sets,
            alloc);
  }

  const VkAllocationCallbacks* Alloc{};
};

using DefaultEngineModel =
    EngineObjModel<Buffer::DEFAULT_EngineModelVertexData, uint32_t, Buffer::UniformBuffer_Trans>;

template <typename V, typename I, typename U>
EngineObjModel<V, I, U> Create_EngineModelObj(std::string& Obj_file,
                                              std::vector<std::string>& Text_file);

//===========================================================================================================================
// EngineGltfModel
//===========================================================================================================================

struct GltfMaterial
{
  glm::vec4 baseColorFactor = glm::vec4(1.0f);
  uint32_t baseColor_index;
};

struct Primitive
{
  uint32_t firstIndex;
  uint32_t indexCount;
  int32_t materialIndex;
};

struct Mesh
{
  std::vector<Primitive> primitives;
};

struct GltfNode
{
  GltfNode* parent;
  glm::mat4 matrix;
  std::vector<GltfNode*> children;
  Mesh mesh;

  ~GltfNode()
  {
    for (auto& child : children)
      {
        delete child;
      }
  }
};

void gltf_LoadImage(const SngoEngine::Core::Device::LogicalDevice::EngineDevice* device,
                    VkCommandPool _pool,
                    tinygltf::Model& input,
                    std::vector<Image::EngineTextureImage>& images,
                    std::vector<Image::EngineSampler>& samplers,
                    const VkAllocationCallbacks* alloc = nullptr);

void gltf_LoadTextures(tinygltf::Model& input,
                       std::vector<Image::EngineTextureImage>& _imgs,
                       std::vector<uint32_t>& _textures);
void gltf_LoadMaterial(tinygltf::Model& input,
                       std::vector<Image::EngineTextureImage>& _imgs,
                       std::vector<GltfMaterial>& _materials);
void load_node(const tinygltf::Node& _node,
               const tinygltf::Model& _input,
               Source::Model::GltfNode* _parent,
               std::vector<uint32_t>& _indexbuffer,
               std::vector<Source::Buffer::GLTF_EngineModelVertexData>& _vertexbuffer,
               std::vector<GltfNode*>& nodes);
void gltf_LoadNode(const Device::LogicalDevice::EngineDevice* _device,
                   VkCommandPool _pool,
                   const tinygltf::Model& _input,
                   Buffer::EngineVertexBuffer<Buffer::GLTF_EngineModelVertexData, 4>& vertex,
                   Buffer::EngineIndexBuffer<uint32_t>& index,
                   std::vector<GltfNode*>& nodes,
                   const VkAllocationCallbacks* alloc = nullptr);

struct EngineGltfModel
{
  EngineGltfModel() = default;
  EngineGltfModel(EngineGltfModel&&) noexcept = default;
  EngineGltfModel& operator=(EngineGltfModel&&) noexcept = default;
  template <typename... Args>
  explicit EngineGltfModel(const std::string& gltf_file,
                           const Device::LogicalDevice::EngineDevice* _device,
                           Args... args)
  {
    creator(gltf_file, _device, args...);
  }
  template <typename... Args>
  void operator()(const std::string& gltf_file,
                  const Device::LogicalDevice::EngineDevice* _device,
                  Args... args)
  {
    creator(gltf_file, _device, args...);
  }
  ~EngineGltfModel() = default;

  void draw(VkCommandBuffer command_buffer, VkPipelineLayout pipeline_layout, uint32_t frame_index);

  std::vector<uint32_t> tex_index;
  std::vector<GltfMaterial> materials;
  std::vector<GltfNode*> nodes;
  std::vector<Image::EngineSampler> img_samplers;
  Descriptor::EngineDescriptorPool descriptor_pool{};

  struct
  {
    Descriptor::EngineDescriptorSets matrices_set;
    Descriptor::EngineDescriptorSets img_sets;
  } descript_sets;

  struct
  {
    Descriptor::EngineDescriptorSetLayout matrices_layout;
    Descriptor::EngineDescriptorSetLayout texture_layout;
  } layouts;

  struct
  {
    Buffer::EngineVertexBuffer<Buffer::GLTF_EngineModelVertexData, 4> vertex_buffer{};
    Buffer::EngineIndexBuffer<uint32_t> index_buffer{};
  } model;

  const Device::LogicalDevice::EngineDevice* device{};

 private:
  void drawNode(VkCommandBuffer commandBuffer,
                VkPipelineLayout pipelineLayout,
                GltfNode* node,
                uint32_t frame_index);

  void creator(const std::string& gltf_file,
               const Device::LogicalDevice::EngineDevice* _device,
               const Buffer::EngineCommandPool* _pool,
               const VkDescriptorBufferInfo* buffer_descriptor,
               const VkAllocationCallbacks* alloc = nullptr)
  {
    device = _device;
    Alloc = alloc;

    tinygltf::Model gltf_input;
    tinygltf::TinyGLTF gltf_context;
    std::string error, warning;

#if defined(__ANDROID__)
    tinygltf::asset_manager = androidApp->activity->assetManager;
#endif

    if (!gltf_context.LoadASCIIFromFile(&gltf_input, &error, &warning, gltf_file))
      {
        throw std::runtime_error("[err] load gltf file " + gltf_file + " failed!");
      }

    gltf_LoadImage(device, _pool->command_pool, gltf_input, imgs, img_samplers, Alloc);
    gltf_LoadTextures(gltf_input, imgs, tex_index);
    gltf_LoadMaterial(gltf_input, imgs, materials);
    gltf_LoadNode(device,
                  _pool->command_pool,
                  gltf_input,
                  model.vertex_buffer,
                  model.index_buffer,
                  nodes,
                  Alloc);

    std::vector<VkDescriptorPoolSize> poolSizes{
        Descriptor::Get_DescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
        Descriptor::Get_DescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imgs.size())};
    const uint32_t maxSetCount = static_cast<uint32_t>(imgs.size()) + 1;
    descriptor_pool(device, maxSetCount, poolSizes, Alloc);

    auto binding{Descriptor::GetLayoutBinding(
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0)};
    layouts.matrices_layout(device, std::vector<VkDescriptorSetLayoutBinding>{binding}, Alloc);
    binding = Descriptor::GetLayoutBinding(
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);
    layouts.texture_layout(device, std::vector<VkDescriptorSetLayoutBinding>{binding}, Alloc);

    descript_sets.matrices_set(device, &layouts.matrices_layout, &descriptor_pool, 1);
    std::vector<VkWriteDescriptorSet> wirtes{Descriptor::GetDescriptSet_Write(
        descript_sets.matrices_set[0], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, buffer_descriptor)};
    descript_sets.matrices_set.updateWrite(wirtes);

    descript_sets.img_sets(device, &layouts.texture_layout, &descriptor_pool, imgs.size());
    wirtes.resize(imgs.size());
    std::vector<VkDescriptorImageInfo> _img_infos;
    for (int i = 0; i < imgs.size(); i++)
      {
        auto _img_info = VkDescriptorImageInfo{
            img_samplers[i].sampler, imgs[i].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

        wirtes[i] = Descriptor::GetDescriptSet_Write({_img_info}, descript_sets.img_sets[i])[0];
      }
    descript_sets.img_sets.updateWrite(wirtes);
  }

  std::vector<Image::EngineTextureImage> imgs;
  const VkAllocationCallbacks* Alloc{};
};

}  // namespace SngoEngine::Core::Source::Model

#endif
