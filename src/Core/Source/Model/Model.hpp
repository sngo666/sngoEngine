#ifndef __SNGO_MODEL_H
#define __SNGO_MODEL_H

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <functional>
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

static bool loadImageDataFunc(tinygltf::Image* image,
                              const int imageIndex,
                              std::string* error,
                              std::string* warning,
                              int req_width,
                              int req_height,
                              const unsigned char* bytes,
                              int size,
                              void* userData)
{
  // KTX files will be handled by our own code
  if (image->uri.find_last_of(".") != std::string::npos)
    {
      if (image->uri.substr(image->uri.find_last_of(".") + 1) == "ktx")
        {
          return true;
        }
    }

  return tinygltf::LoadImageData(
      image, imageIndex, error, warning, req_width, req_height, bytes, size, userData);
}

static bool loadImageDataFuncEmpty(tinygltf::Image* image,
                                   const int imageIndex,
                                   std::string* error,
                                   std::string* warning,
                                   int req_width,
                                   int req_height,
                                   const unsigned char* bytes,
                                   int size,
                                   void* userData)
{
  // This function will be used for samples that don't require images to be loaded
  return true;
}

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
                                              texture_imgs[i].view.image_view,
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

enum FileLoadingFlags
{
  None = 0x00000000,
  PreTransformVertices = 0x00000001,
  PreMultiplyVertexColors = 0x00000002,
  FlipY = 0x00000004,
  DontLoadImages = 0x00000008
};

enum DescriptorBindingFlags
{
  ImageBaseColor = 0x00000001,
  ImageNormalMap = 0x00000002
};

enum RenderFlags
{
  BindImages = 0x00000001,
  RenderOpaqueNodes = 0x00000002,
  RenderAlphaMaskedNodes = 0x00000004,
  RenderAlphaBlendedNodes = 0x00000008
};

struct EngineGltfModel;
struct GltfNode;

struct GLTF_EngineModelVertexData
{
  glm::vec3 pos;
  glm::vec3 normal;
  glm::vec2 uv;
  glm::vec4 color;
  glm::vec4 joint0;
  glm::vec4 weight0;
  glm::vec4 tangent;

  static const int attribute_count = 7;

  enum component
  {
    POS = 0x00000001,
    NORMAL = 0x00000002,
    UV = 0x00000004,
    COLOR = 0x00000008,
    JOINT0 = 0x00000010,
    WEIGHT0 = 0x00000020,
    TANGENT = 0x00000040
  };

  static VkVertexInputBindingDescription getBindingDescription()
  {
    VkVertexInputBindingDescription binding_description{};
    binding_description.binding = 0;
    binding_description.stride = sizeof(GLTF_EngineModelVertexData);
    binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return binding_description;
  }

  bool operator==(const GLTF_EngineModelVertexData& data) const
  {
    return (pos == data.pos && normal == data.normal && color == data.color && uv == data.uv
            && joint0 == data.joint0 && weight0 == data.weight0 && tangent == data.tangent);
  }

  static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions()
  {
    std::vector<VkVertexInputAttributeDescription> attribute_descriptions{7};

    attribute_descriptions[0].binding = 0;
    attribute_descriptions[0].location = 0;
    attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attribute_descriptions[0].offset = offsetof(GLTF_EngineModelVertexData, pos);

    attribute_descriptions[1].binding = 0;
    attribute_descriptions[1].location = 1;
    attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attribute_descriptions[1].offset = offsetof(GLTF_EngineModelVertexData, normal);

    attribute_descriptions[2].binding = 0;
    attribute_descriptions[2].location = 2;
    attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attribute_descriptions[2].offset = offsetof(GLTF_EngineModelVertexData, uv);

    attribute_descriptions[3].binding = 0;
    attribute_descriptions[3].location = 3;
    attribute_descriptions[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attribute_descriptions[3].offset = offsetof(GLTF_EngineModelVertexData, color);

    attribute_descriptions[4].binding = 0;
    attribute_descriptions[4].location = 4;
    attribute_descriptions[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attribute_descriptions[4].offset = offsetof(GLTF_EngineModelVertexData, joint0);

    attribute_descriptions[5].binding = 0;
    attribute_descriptions[5].location = 5;
    attribute_descriptions[5].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attribute_descriptions[5].offset = offsetof(GLTF_EngineModelVertexData, weight0);

    attribute_descriptions[6].binding = 0;
    attribute_descriptions[6].location = 6;
    attribute_descriptions[6].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attribute_descriptions[6].offset = offsetof(GLTF_EngineModelVertexData, tangent);

    return attribute_descriptions;
  }

  static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions(int _com,
                                                                                 uint32_t binding)
  {
    std::vector<VkVertexInputAttributeDescription> attributes{};
    VkVertexInputAttributeDescription attri;

    if (_com & POS)
      {
        attri.binding = binding;
        attri.location = attributes.size();
        attri.format = VK_FORMAT_R32G32B32_SFLOAT;
        attri.offset = offsetof(GLTF_EngineModelVertexData, pos);

        attributes.push_back(attri);
      }

    if (_com & NORMAL)
      {
        attri.binding = binding;
        attri.location = attributes.size();
        attri.format = VK_FORMAT_R32G32B32_SFLOAT;
        attri.offset = offsetof(GLTF_EngineModelVertexData, normal);

        attributes.push_back(attri);
      }

    if (_com & UV)
      {
        attri.binding = binding;
        attri.location = attributes.size();
        attri.format = VK_FORMAT_R32G32_SFLOAT;
        attri.offset = offsetof(GLTF_EngineModelVertexData, uv);

        attributes.push_back(attri);
      }

    if (_com & COLOR)
      {
        attri.binding = binding;
        attri.location = attributes.size();
        attri.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attri.offset = offsetof(GLTF_EngineModelVertexData, color);
        attributes.push_back(attri);
      }

    if (_com & JOINT0)
      {
        attri.binding = binding;
        attri.location = attributes.size();
        attri.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attri.offset = offsetof(GLTF_EngineModelVertexData, joint0);
        attributes.push_back(attri);
      }

    if (_com & WEIGHT0)
      {
        attri.binding = binding;
        attri.location = attributes.size();
        attri.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attri.offset = offsetof(GLTF_EngineModelVertexData, weight0);
        attributes.push_back(attri);
      }

    if (_com & TANGENT)
      {
        attri.binding = binding;
        attri.location = attributes.size();
        attri.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attri.offset = offsetof(GLTF_EngineModelVertexData, tangent);
        attributes.push_back(attri);
      }

    return attributes;
  }
};

struct GltfTexture
{
  uint32_t img_index;
  Image::EngineTextureImage* texture;

  [[nodiscard]] bool is_available() const
  {
    return (texture != nullptr);
  }
};

struct GltfMaterial
{
  // const definitions
  enum AlphaMode
  {
    ALPHAMODE_OPAQUE,
    ALPHAMODE_MASK,
    ALPHAMODE_BLEND
  };
  constexpr static const GltfTexture empty_tex{0, nullptr};

  // members
  AlphaMode alphaMode = ALPHAMODE_OPAQUE;
  glm::vec4 baseColor_factor = glm::vec4(1.0f);
  float alphaCutoff = 1.0f;
  float metallicFactor = 1.0f;
  float roughnessFactor = 1.0f;

  GltfTexture base_color = empty_tex;
  GltfTexture metallic_roughness = empty_tex;
  GltfTexture normal = empty_tex;
  GltfTexture occlusion = empty_tex;
  GltfTexture emissive = empty_tex;
  GltfTexture specular_glossiness = empty_tex;
  GltfTexture diffuse = empty_tex;

  Descriptor::EngineDescriptorSet descriptor_set{};
  const Device::LogicalDevice::EngineDevice* device{};

  // functions
  explicit GltfMaterial(const Device::LogicalDevice::EngineDevice* _device) : device(_device){};
  GltfMaterial() = default;
  void create_set(VkDescriptorPool _pool, VkDescriptorSetLayout _layout, uint32_t bingding_flags);
};

struct Primitive
{
  // members
  uint32_t firstIndex{};
  uint32_t indexCount{};
  uint32_t firstVertex{};
  uint32_t vertexCount{};

  int32_t materialIndex{};
  GltfMaterial* material{};

  // functions
  struct Dimensions
  {
    glm::vec3 min = glm::vec3(FLT_MAX);
    glm::vec3 max = glm::vec3(-FLT_MAX);
    glm::vec3 size{};
    glm::vec3 center{};
    float radius{};
  } dimensions;

  Primitive(uint32_t firstIndex, uint32_t indexCount, GltfMaterial* material)
      : firstIndex(firstIndex), indexCount(indexCount), material(material){};
};

struct UniformBlock
{
  glm::mat4 matrix{};
  glm::mat4 jointMatrix[64]{};
  float jointcount{0};
};

struct Mesh
{
  std::vector<Primitive> primitives;
  std::string name;

  Buffer::EngineUniformBuffer<UniformBlock> unibuffer;
  Descriptor::EngineDescriptorSet set;
  const Device::LogicalDevice::EngineDevice* device{};

  UniformBlock uniformBlock;

  Mesh(const Device::LogicalDevice::EngineDevice* _device,
       glm::mat4 matrix,
       const VkAllocationCallbacks* alloc = nullptr)
  {
    device = _device;
    uniformBlock.matrix = matrix;
    unibuffer(device, device->graphics_queue, alloc);
  }
};

struct Skin
{
  std::string name;
  GltfNode* skeletonRoot = nullptr;
  std::vector<glm::mat4> inverseBindMatrices;
  std::vector<GltfNode*> joints;
};

struct GltfNode
{
  // members
  GltfNode* parent;
  glm::mat4 matrix;
  std::vector<GltfNode*> children;
  std::string name;
  uint32_t index;

  Skin* skin;
  int32_t skinIndex = -1;

  Mesh* mesh;
  glm::vec3 translation{};
  glm::vec3 scale{1.0f};
  glm::quat rotation{};

  // functions
  [[nodiscard]] glm::mat4 localMatrix() const
  {
    return glm::translate(glm::mat4(1.0f), translation) * glm::mat4(rotation)
           * glm::scale(glm::mat4(1.0f), scale) * matrix;
  }
  [[nodiscard]] glm::mat4 getMatrix() const
  {
    glm::mat4 m = localMatrix();
    GltfNode* p = parent;
    while (p)
      {
        m = p->localMatrix() * m;
        p = p->parent;
      }
    return m;
  }
  void update()
  {
    if (mesh)
      {
        glm::mat4 m = getMatrix();
        if (skin)
          {
            mesh->uniformBlock.matrix = m;
            // Update join matrices
            glm::mat4 inverseTransform = glm::inverse(m);
            for (size_t i = 0; i < skin->joints.size(); i++)
              {
                GltfNode* jointNode = skin->joints[i];
                glm::mat4 jointMat = jointNode->getMatrix() * skin->inverseBindMatrices[i];
                jointMat = inverseTransform * jointMat;
                mesh->uniformBlock.jointMatrix[i] = jointMat;
              }
            mesh->uniformBlock.jointcount = (float)skin->joints.size();
            memcpy(mesh->unibuffer.mapped, &mesh->uniformBlock, sizeof(mesh->uniformBlock));
          }
        else
          {
            memcpy(mesh->unibuffer.mapped, &m, sizeof(glm::mat4));
          }
      }

    for (auto& child : children)
      {
        child->update();
      }
  }

  ~GltfNode()
  {
    for (auto& child : children)
      {
        delete child;
      }

    delete mesh;
  }
};

void gltf_LoadImage(const SngoEngine::Core::Device::LogicalDevice::EngineDevice* device,
                    VkCommandPool _pool,
                    tinygltf::Model& input,
                    std::vector<Image::EngineTextureImage>& images,
                    const VkAllocationCallbacks* alloc = nullptr);

void gltf_LoadTextures(tinygltf::Model& input,
                       std::vector<Image::EngineTextureImage>& _imgs,
                       std::vector<GltfTexture>& _textures);
void gltf_LoadMaterial(tinygltf::Model& input,
                       std::vector<Image::EngineTextureImage>& _imgs,
                       std::vector<GltfMaterial>& _materials);

void Set_Dimensions(Primitive::Dimensions& dimensions, glm::vec3 min, glm::vec3 max);
void Get_NodeDimensions(GltfNode* node, glm::vec3& min, glm::vec3& max);

struct EngineGltfModel
{
  // ----------------------    functions     -----------------------
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
  ~EngineGltfModel()
  {
    destroyer();
  }
  void bind_buffers(VkCommandBuffer command_buffer);
  void draw(VkCommandBuffer command_buffer,
            VkPipelineLayout pipeline_layout,
            uint32_t bindImage_set = 1,
            uint32_t renderFlags = BindImages,
            uint32_t frame_index = 0);

  // ----------------------    members     -----------------------
  std::vector<GltfMaterial> materials;
  std::vector<GltfNode*> nodes;
  std::vector<GltfNode*> linear_nodes;
  std::vector<Skin*> skins;
  Primitive::Dimensions dimensions;

  Descriptor::EngineDescriptorPool descriptor_pool{};

  struct
  {
    Descriptor::EngineDescriptorSetLayout matrices;
    Descriptor::EngineDescriptorSetLayout texture;
    uint32_t descript_bindingflags =
        DescriptorBindingFlags::ImageBaseColor | DescriptorBindingFlags::ImageNormalMap;
  } layouts;

  struct
  {
    Buffer::EngineVertexBuffer<GLTF_EngineModelVertexData,
                               GLTF_EngineModelVertexData::attribute_count>
        vertex_buffer{};
    Buffer::EngineIndexBuffer<uint32_t> index_buffer{};
  } model;
  const Device::LogicalDevice::EngineDevice* device{};

  // ----------------------    private     -----------------------
 private:
  void load_imgs(tinygltf::Model& input, VkCommandPool _pool);
  void load_node(const tinygltf::Node& _node,
                 const tinygltf::Model& _input,
                 uint32_t _index,
                 Source::Model::GltfNode* _parent,
                 std::vector<uint32_t>& _indexbuffer,
                 std::vector<GLTF_EngineModelVertexData>& _vertexbuffer);
  void load_nodes(const Device::LogicalDevice::EngineDevice* _device,
                  VkCommandPool _pool,
                  const tinygltf::Model& _input,
                  uint32_t flags);
  void load_skins(tinygltf::Model& input);
  void load_animations(tinygltf::Model& input);
  void load_materials(tinygltf::Model& input);
  void get_sceneDimensions(Primitive::Dimensions& dimensions);

  void drawNode(VkCommandBuffer commandBuffer,
                VkPipelineLayout pipelineLayout,
                GltfNode* node,
                uint32_t bindImage_set,
                uint32_t renderFlags,
                uint32_t frame_index);
  void destroyer();

  // ------------------------------ creator  --------------------------------------

  void creator(const std::string& gltf_file,
               const Device::LogicalDevice::EngineDevice* _device,
               const Buffer::EngineCommandPool* _pool,
               const VkAllocationCallbacks* alloc = nullptr,
               const uint32_t loading_flag = PreTransformVertices | FlipY)
  {
    device = _device;
    Alloc = alloc;

    tinygltf::Model gltf_input;
    tinygltf::TinyGLTF gltf_context;
    std::string error, warning;

    // load gltf with tinygltf
    if (loading_flag & FileLoadingFlags::DontLoadImages)
      {
        gltf_context.SetImageLoader(loadImageDataFuncEmpty, nullptr);
      }
    else
      {
        gltf_context.SetImageLoader(loadImageDataFunc, nullptr);
      }

    if (!gltf_context.LoadASCIIFromFile(&gltf_input, &error, &warning, gltf_file))
      {
        throw std::runtime_error("[err] load gltf file " + gltf_file + " failed!");
      }

    // load elements
    if (!(loading_flag & FileLoadingFlags::DontLoadImages))
      {
        load_imgs(gltf_input, _pool->command_pool);
      }

    load_materials(gltf_input);
    load_nodes(device, _pool->command_pool, gltf_input, loading_flag);
    if (!gltf_input.animations.empty())
      {
        load_animations(gltf_input);
      }
    load_skins(gltf_input);

    get_sceneDimensions(dimensions);

    // descriptor pool for all
    {
      uint32_t ubo_count{0};
      uint32_t img_count{0};
      for (auto node : linear_nodes)
        {
          if (node->mesh)
            {
              ubo_count++;
            }
        }
      for (auto& material : materials)
        {
          if (material.base_color.texture)
            {
              img_count++;
            }
        }
      std::vector<VkDescriptorPoolSize> poolSizes = {
          {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, ubo_count},
      };
      if (img_count)
        {
          if (layouts.descript_bindingflags & DescriptorBindingFlags::ImageBaseColor)
            {
              poolSizes.push_back(Descriptor::Get_DescriptorPoolSize(
                  VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, img_count));
            }
          if (layouts.descript_bindingflags & DescriptorBindingFlags::ImageNormalMap)
            {
              poolSizes.push_back(Descriptor::Get_DescriptorPoolSize(
                  VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, img_count));
            }
        }
      const uint32_t maxSetCount = img_count + ubo_count;
      descriptor_pool(device, maxSetCount, poolSizes, Alloc);
    }

    // uniform buffer descriptor layout & sets
    {
      auto binding{Descriptor::GetLayoutBinding(
          VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0)};
      layouts.matrices(device, std::vector<VkDescriptorSetLayoutBinding>{binding}, Alloc);

      std::function<void(GltfNode * node, VkDescriptorSetLayout layout)> lambda_node_descript =
          [&](GltfNode* node, VkDescriptorSetLayout layout) {
            if (node->mesh)
              {
                node->mesh->set(device, &layouts.matrices, &descriptor_pool);

                auto write{Descriptor::GetDescriptSet_Write(node->mesh->set.descriptor_set,
                                                            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                            0,
                                                            &node->mesh->unibuffer.descriptor)};
                node->mesh->set.updateWrite(&write[0]);
              }
            for (auto& child : node->children)
              {
                lambda_node_descript(child, layouts.matrices.layout);
              }
          };

      for (auto node : nodes)
        {
          lambda_node_descript(node, layouts.matrices.layout);
        }
    }

    // texture imgs descriptor layout & sets
    {
      std::vector<VkDescriptorSetLayoutBinding> binding{};
      if (layouts.descript_bindingflags & DescriptorBindingFlags::ImageBaseColor)
        {
          binding.push_back(Descriptor::GetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                         VK_SHADER_STAGE_FRAGMENT_BIT,
                                                         static_cast<uint32_t>(binding.size())));
        }
      if (layouts.descript_bindingflags & DescriptorBindingFlags::ImageNormalMap)
        {
          binding.push_back(Descriptor::GetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                         VK_SHADER_STAGE_FRAGMENT_BIT,
                                                         static_cast<uint32_t>(binding.size())));
        }
      layouts.texture(device, binding, Alloc);
      for (auto& material : materials)
        {
          if (material.base_color.is_available())
            {
              material.create_set(descriptor_pool.descriptor_pool,
                                  layouts.texture.layout,
                                  layouts.descript_bindingflags);
            }
        }
    }
  }

  std::vector<Image::EngineTextureImage> imgs;
  const VkAllocationCallbacks* Alloc{};
};

//===========================================================================================================================
// EngineCubeMap
//===========================================================================================================================

struct EngineCubeMap
{
  EngineCubeMap() = default;
  EngineCubeMap(EngineCubeMap&&) noexcept = default;
  EngineCubeMap& operator=(EngineCubeMap&&) noexcept = default;
  template <typename... Args>
  explicit EngineCubeMap(const std::string& gltf_file,
                         const std::string& boxTexture_file,
                         const Device::LogicalDevice::EngineDevice* _device,
                         const Args... args)
  {
    creator(gltf_file, boxTexture_file, _device, args...);
  }
  template <typename... Args>
  void operator()(const std::string& gltf_file,
                  const std::string& boxTexture_file,
                  const Device::LogicalDevice::EngineDevice* _device,
                  Args... args)
  {
    creator(gltf_file, boxTexture_file, _device, args...);
  }
  ~EngineCubeMap() = default;

  void generate_descriptor(const Descriptor::EngineDescriptorPool& _pool,
                           Descriptor::EngineDescriptorSetLayout& _layout,
                           Descriptor::EngineDescriptorSet& _set,
                           uint32_t binding);

  EngineGltfModel model;
  Image::EngineCubeTexture cube_img;

  const Device::LogicalDevice::EngineDevice* device{};

 private:
  void creator(const std::string& gltf_file,
               const std::string& boxTexture_file,
               const Device::LogicalDevice::EngineDevice* _device,
               Buffer::EngineCommandPool* _pool,
               const VkAllocationCallbacks* alloc = nullptr);
  const VkAllocationCallbacks* Alloc{};
};

}  // namespace SngoEngine::Core::Source::Model

#endif
