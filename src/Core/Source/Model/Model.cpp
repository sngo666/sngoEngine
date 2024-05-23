#include "Model.hpp"

#include <vulkan/vulkan_core.h>

#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_float4.hpp>
#include <stdexcept>
#include <utility>
#include <vector>

#include "Model.hpp"
#include "fmt/core.h"
#include "src/Core/Data.h"
#include "src/Core/Device/LogicalDevice.hpp"
#include "src/Core/Source/Buffer/CommandBuffer.hpp"
#include "src/Core/Source/Buffer/Descriptor.hpp"
#include "src/Core/Source/Buffer/IndexBuffer.hpp"
#include "src/Core/Source/Buffer/VertexBuffer.hpp"
#include "src/Core/Source/Image/Image.hpp"
#include "src/Core/Source/Image/Sampler.hpp"
#include "src/Core/Utils/tiny_gltf.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

void SngoEngine::Core::Source::Model::gltf_LoadImage(
    const SngoEngine::Core::Device::LogicalDevice::EngineDevice* device,
    VkCommandPool _pool,
    tinygltf::Model& input,
    std::vector<Image::EngineTextureImage>& images,
    const VkAllocationCallbacks* alloc)
{
  for (size_t i = 0; i < input.images.size(); i++)
    {
      tinygltf::Image& glTFImage = input.images[i];
      // Get the image data from the glTF loader
      unsigned char* buffer = nullptr;
      VkDeviceSize bufferSize = 0;
      bool deleteBuffer = false;
      // We convert RGB-only images to RGBA, as most devices don't support RGB-formats in Vulkan
      if (glTFImage.component == 3)
        {
          bufferSize = static_cast<long long>(glTFImage.width * glTFImage.height * 4);
          buffer = new unsigned char[bufferSize];
          unsigned char* rgba = buffer;
          unsigned char* rgb = &glTFImage.image[0];
          for (size_t j = 0; j < static_cast<long long>(glTFImage.width * glTFImage.height); ++j)
            {
              memcpy(rgba, rgb, sizeof(unsigned char) * 3);
              rgba += 4;
              rgb += 3;
            }
          deleteBuffer = true;
        }
      else
        {
          buffer = &glTFImage.image[0];
          bufferSize = glTFImage.image.size();
        }
      // Load texture from image buffer

      images[i].init(device,
                     _pool,
                     Image::EnginePixelData{buffer,
                                            static_cast<unsigned int>(glTFImage.width),
                                            static_cast<unsigned int>(glTFImage.height),
                                            bufferSize},
                     alloc);

      if (deleteBuffer)
        {
          delete[] buffer;
        }
    }
}

void SngoEngine::Core::Source::Model::gltf_LoadMaterial(
    tinygltf::Model& input,
    std::vector<Image::EngineTextureImage>& _imgs,
    std::vector<GltfMaterial>& _materials)
{
  auto lambda_get_texture{[&](uint32_t index) -> Source::Image::EngineTextureImage* {
    if (index < _imgs.size())
      {
        return &_imgs[index];
      }
    return nullptr;
  }};

  for (size_t i = 0; i < input.materials.size(); i++)
    {
      // We only read the most basic properties required for our sample
      tinygltf::Material glTFMaterial = input.materials[i];
      // Color Factor
      if (glTFMaterial.values.find("baseColorFactor") != glTFMaterial.values.end())
        {
          _materials[i].baseColor_factor =
              glm::make_vec4(glTFMaterial.values["baseColorFactor"].ColorFactor().data());
        }

      // metallic Factor
      if (glTFMaterial.values.find("metallicFactor") != glTFMaterial.values.end())
        {
          _materials[i].metallicFactor =
              static_cast<float>(glTFMaterial.values["metallicFactor"].Factor());
        }

      // Roughness Factor
      if (glTFMaterial.values.find("roughnessFactor") != glTFMaterial.values.end())
        {
          _materials[i].roughnessFactor =
              static_cast<float>(glTFMaterial.values["roughnessFactor"].Factor());
        }

      // Alpha Cutoff Factor
      if (glTFMaterial.additionalValues.find("alphaCutoff") != glTFMaterial.additionalValues.end())
        {
          _materials[i].alphaCutoff =
              static_cast<float>(glTFMaterial.additionalValues["alphaCutoff"].Factor());
        }

      // Color Texture
      if (glTFMaterial.values.find("baseColorTexture") != glTFMaterial.values.end())
        {
          uint32_t _index{
              static_cast<uint32_t>(glTFMaterial.values["baseColorTexture"].TextureIndex())};
          _materials[i].base_color = {_index, lambda_get_texture(_index)};
        }

      // Metallic Roughness Texture
      if (glTFMaterial.values.find("metallicRoughnessTexture") != glTFMaterial.values.end())
        {
          uint32_t _index{static_cast<uint32_t>(
              glTFMaterial.values["metallicRoughnessTexture"].TextureIndex())};
          _materials[i].metallic_roughness = {_index, lambda_get_texture(_index)};
        }

      // Emissive Texture
      if (glTFMaterial.additionalValues.find("emissiveTexture")
          != glTFMaterial.additionalValues.end())
        {
          uint32_t _index{
              static_cast<uint32_t>(glTFMaterial.values["emissiveTexture"].TextureIndex())};
          _materials[i].emissive = {_index, lambda_get_texture(_index)};
        }

      // Occlusion Texture
      if (glTFMaterial.additionalValues.find("occlusionTexture")
          != glTFMaterial.additionalValues.end())
        {
          uint32_t _index{
              static_cast<uint32_t>(glTFMaterial.values["occlusionTexture"].TextureIndex())};
          _materials[i].occlusion = {_index, lambda_get_texture(_index)};
        }

      // Alpha Mode
      if (glTFMaterial.additionalValues.find("alphaMode") != glTFMaterial.additionalValues.end())
        {
          tinygltf::Parameter param = glTFMaterial.additionalValues["alphaMode"];
          if (param.string_value == "BLEND")
            {
              _materials[i].alphaMode = GltfMaterial::ALPHAMODE_BLEND;
            }
          if (param.string_value == "MASK")
            {
              _materials[i].alphaMode = GltfMaterial::ALPHAMODE_MASK;
            }
        }

      // Normal Texture
      auto normal_index{static_cast<uint32_t>(glTFMaterial.values["normalTexture"].TextureIndex())};
      if (glTFMaterial.additionalValues.find("normalTexture") != glTFMaterial.additionalValues.end()
          && normal_index < _imgs.size())
        {
          _materials[i].normal = {normal_index, lambda_get_texture(normal_index)};
        }
      else
        {
          _materials[i].normal = {static_cast<uint32_t>(-1), &_imgs.back()};
        }
    }
}

void SngoEngine::Core::Source::Model::Set_Dimensions(Primitive::Dimensions& dimensions,
                                                     glm::vec3 min,
                                                     glm::vec3 max)
{
  dimensions.min = min;
  dimensions.max = max;
  dimensions.size = max - min;
  dimensions.center = (min + max) / 2.0f;
  dimensions.radius = glm::distance(min, max) / 2.0f;
}

void SngoEngine::Core::Source::Model::Get_NodeDimensions(GltfNode* node,
                                                         glm::vec3& min,
                                                         glm::vec3& max)
{
  if (node->mesh)
    {
      for (Primitive& primitive : node->mesh->primitives)
        {
          glm::vec4 locMin = glm::vec4(primitive.dimensions.min, 1.0f) * node->getMatrix();
          glm::vec4 locMax = glm::vec4(primitive.dimensions.max, 1.0f) * node->getMatrix();
          if (locMin.x < min.x)
            {
              min.x = locMin.x;
            }
          if (locMin.y < min.y)
            {
              min.y = locMin.y;
            }
          if (locMin.z < min.z)
            {
              min.z = locMin.z;
            }
          if (locMax.x > max.x)
            {
              max.x = locMax.x;
            }
          if (locMax.y > max.y)
            {
              max.y = locMax.y;
            }
          if (locMax.z > max.z)
            {
              max.z = locMax.z;
            }
        }
    }
  for (auto child : node->children)
    {
      Get_NodeDimensions(child, min, max);
    }
}

//===========================================================================================================================
// GltfMaterial
//===========================================================================================================================

void SngoEngine::Core::Source::Model::GltfMaterial::create_set(VkDescriptorPool _pool,
                                                               VkDescriptorSetLayout _layout,
                                                               uint32_t bingding_flags)
{
  descriptor_set.init(device, _layout, _pool);
  std::vector<VkWriteDescriptorSet> writes{};
  VkWriteDescriptorSet writeDescriptorSet{};

  if (bingding_flags & DescriptorBindingFlags::ImageBaseColor)
    {
      writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      writeDescriptorSet.descriptorCount = 1;
      writeDescriptorSet.dstSet = descriptor_set.descriptor_set;
      writeDescriptorSet.dstBinding = static_cast<uint32_t>(writes.size());
      writeDescriptorSet.pImageInfo = &base_color.texture->descriptor;
      writes.push_back(writeDescriptorSet);
    }
  if (normal.is_available() && bingding_flags & DescriptorBindingFlags::ImageNormalMap)
    {
      writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      writeDescriptorSet.descriptorCount = 1;
      writeDescriptorSet.dstSet = descriptor_set.descriptor_set;
      writeDescriptorSet.dstBinding = static_cast<uint32_t>(writes.size());
      writeDescriptorSet.pImageInfo = &normal.texture->descriptor;
      writes.push_back(writeDescriptorSet);
    }

  descriptor_set.updateWrite(writes);
}

//===========================================================================================================================
// EngineGltfModel
//===========================================================================================================================

void SngoEngine::Core::Source::Model::EngineGltfModel::bind_buffers(VkCommandBuffer command_buffer)
{
  const VkDeviceSize offsets[1] = {0};
  vkCmdBindVertexBuffers(command_buffer, 0, 1, &model.vertex_buffer.buffer, offsets);
  vkCmdBindIndexBuffer(command_buffer, model.index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);
}

void SngoEngine::Core::Source::Model::EngineGltfModel::load_imgs(tinygltf::Model& input,
                                                                 VkCommandPool _pool)
{
  imgs.resize(input.images.size() + 1);

  gltf_LoadImage(device, _pool, input, imgs, Alloc);
  Image::Get_EmptyTextureImg(device, imgs.back(), _pool, Alloc);
}

void SngoEngine::Core::Source::Model::EngineGltfModel::load_node(
    const tinygltf::Node& _node,
    const tinygltf::Model& _input,
    uint32_t _index,
    GltfNode* _parent,
    std::vector<uint32_t>& _indexbuffer,
    std::vector<GLTF_EngineModelVertexData>& _vertexbuffer)
{
  auto* new_node = new GltfNode{};
  new_node->matrix = glm::mat4(1.0f);
  new_node->parent = _parent;
  new_node->name = _node.name;
  new_node->index = _index;
  new_node->skinIndex = _node.skin;

  // get transformation
  {
    auto translation = glm::vec3(0.0f);
    if (_node.translation.size() == 3)
      {
        translation = glm::make_vec3(_node.translation.data());
        new_node->translation = translation;
      }

    if (_node.rotation.size() == 4)
      {
        glm::quat q = glm::make_quat(_node.rotation.data());
        new_node->rotation = glm::mat4(q);
      }
    auto scale = glm::vec3(1.0f);
    if (_node.scale.size() == 3)
      {
        scale = glm::make_vec3(_node.scale.data());
        new_node->scale = scale;
      }
    if (_node.matrix.size() == 16)
      {
        new_node->matrix = glm::make_mat4x4(_node.matrix.data());
      };
  }

  // Load node's children
  if (!_node.children.empty())
    {
      for (int it : _node.children)
        {
          load_node(_input.nodes[it], _input, it, new_node, _indexbuffer, _vertexbuffer);
        }
    }
  // throw std::runtime_error("you must");

  if (_node.mesh > -1)
    {
      const tinygltf::Mesh gltf_mesh = _input.meshes[_node.mesh];
      Mesh* mesh = new Mesh(device, new_node->matrix);
      mesh->name = gltf_mesh.name;

      // Iterate through all primitives of this node's mesh
      for (const auto& glTF_primitive : gltf_mesh.primitives)
        {
          auto firstIndex = static_cast<uint32_t>(_indexbuffer.size());
          auto vertexStart = static_cast<uint32_t>(_vertexbuffer.size());
          uint32_t indexCount = 0;
          // Vertices
          {
            const float* position_buffer = nullptr;
            const float* normals_buffer = nullptr;
            const float* texCoords_buffer = nullptr;
            const float* colors_buffer = nullptr;
            const float* tangents_buffer = nullptr;
            const uint16_t* joints_buffer = nullptr;
            const float* weights_buffer = nullptr;
            uint32_t num_colorComponents;
            size_t vertexCount = 0;

            // Get buffer data for vertex positions
            if (glTF_primitive.attributes.find("POSITION") != glTF_primitive.attributes.end())
              {
                const tinygltf::Accessor& accessor =
                    _input.accessors[glTF_primitive.attributes.find("POSITION")->second];
                const tinygltf::BufferView& view = _input.bufferViews[accessor.bufferView];

                position_buffer = reinterpret_cast<const float*>(
                    &(_input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                vertexCount = accessor.count;
              }
            // Get buffer data for vertex normals
            if (glTF_primitive.attributes.find("NORMAL") != glTF_primitive.attributes.end())
              {
                const tinygltf::Accessor& accessor =
                    _input.accessors[glTF_primitive.attributes.find("NORMAL")->second];
                const tinygltf::BufferView& view = _input.bufferViews[accessor.bufferView];
                normals_buffer = reinterpret_cast<const float*>(
                    &(_input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
              }
            // Get buffer data for vertex texture coordinates
            // glTF supports multiple sets, we only load the first one
            if (glTF_primitive.attributes.find("TEXCOORD_0") != glTF_primitive.attributes.end())
              {
                const tinygltf::Accessor& accessor =
                    _input.accessors[glTF_primitive.attributes.find("TEXCOORD_0")->second];
                const tinygltf::BufferView& view = _input.bufferViews[accessor.bufferView];
                texCoords_buffer = reinterpret_cast<const float*>(
                    &(_input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
              }

            if (glTF_primitive.attributes.count("COLOR_0"))
              {
                const tinygltf::Accessor& colorAccessor =
                    _input.accessors[glTF_primitive.attributes.find("COLOR_0")->second];
                const tinygltf::BufferView& colorView =
                    _input.bufferViews[colorAccessor.bufferView];
                // Color buffer are either of type vec3 or vec4
                num_colorComponents =
                    colorAccessor.type == TINYGLTF_PARAMETER_TYPE_FLOAT_VEC3 ? 3 : 4;
                colors_buffer = reinterpret_cast<const float*>(
                    &(_input.buffers[colorView.buffer]
                          .data[colorAccessor.byteOffset + colorView.byteOffset]));
              }

            if (glTF_primitive.attributes.find("TANGENT") != glTF_primitive.attributes.end())
              {
                const tinygltf::Accessor& tangentAccessor =
                    _input.accessors[glTF_primitive.attributes.find("TANGENT")->second];
                const tinygltf::BufferView& tangentView =
                    _input.bufferViews[tangentAccessor.bufferView];
                tangents_buffer = reinterpret_cast<const float*>(
                    &(_input.buffers[tangentView.buffer]
                          .data[tangentAccessor.byteOffset + tangentView.byteOffset]));
              }

            // Skinning
            // Joints
            if (glTF_primitive.attributes.find("JOINTS_0") != glTF_primitive.attributes.end())
              {
                const tinygltf::Accessor& jointAccessor =
                    _input.accessors[glTF_primitive.attributes.find("JOINTS_0")->second];
                const tinygltf::BufferView& jointView =
                    _input.bufferViews[jointAccessor.bufferView];
                joints_buffer = reinterpret_cast<const uint16_t*>(
                    &(_input.buffers[jointView.buffer]
                          .data[jointAccessor.byteOffset + jointView.byteOffset]));
              }

            if (glTF_primitive.attributes.find("WEIGHTS_0") != glTF_primitive.attributes.end())
              {
                const tinygltf::Accessor& uvAccessor =
                    _input.accessors[glTF_primitive.attributes.find("WEIGHTS_0")->second];
                const tinygltf::BufferView& uvView = _input.bufferViews[uvAccessor.bufferView];
                weights_buffer = reinterpret_cast<const float*>(&(
                    _input.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
              }
            auto hasSkin{joints_buffer && weights_buffer};

            // Append data to model's vertex buffer
            for (size_t v = 0; v < vertexCount; v++)
              {
                GLTF_EngineModelVertexData vert{};
                vert.pos = glm::vec4(glm::make_vec3(&position_buffer[v * 3]), 1.0f);
                vert.normal = glm::normalize(glm::vec3(
                    normals_buffer ? glm::make_vec3(&normals_buffer[v * 3]) : glm::vec3(0.0f)));
                vert.uv =
                    texCoords_buffer ? glm::make_vec2(&texCoords_buffer[v * 2]) : glm::vec3(0.0f);
                if (colors_buffer)
                  {
                    if (num_colorComponents == 3)
                      {
                        vert.color = glm::vec4(glm::make_vec3(&colors_buffer[v * 3]), 1.0f);
                      }
                    else
                      {
                        vert.color = glm::make_vec4(&colors_buffer[v * 4]);
                      }
                    _vertexbuffer.push_back(vert);
                  }
                vert.tangent = tangents_buffer ? glm::vec4(glm::make_vec4(&tangents_buffer[v * 4]))
                                               : glm::vec4(0.0f);
                vert.joint0 =
                    hasSkin ? glm::vec4(glm::make_vec4(&joints_buffer[v * 4])) : glm::vec4(0.0f);
                vert.weight0 = hasSkin ? glm::make_vec4(&weights_buffer[v * 4]) : glm::vec4(0.0f);
                _vertexbuffer.push_back(vert);
              }
            // Indices
            {
              const tinygltf::Accessor& accessor = _input.accessors[glTF_primitive.indices];
              const tinygltf::BufferView& bufferView = _input.bufferViews[accessor.bufferView];
              const tinygltf::Buffer& buffer = _input.buffers[bufferView.buffer];

              indexCount += static_cast<uint32_t>(accessor.count);

              // glTF supports different component types of indices
              switch (accessor.componentType)
                {
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
                      const auto* buf = reinterpret_cast<const uint32_t*>(
                          &buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                      for (size_t index = 0; index < accessor.count; index++)
                        {
                          _indexbuffer.push_back(buf[index] + vertexStart);
                        }
                      break;
                    }
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                      const auto* buf = reinterpret_cast<const uint16_t*>(
                          &buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                      for (size_t index = 0; index < accessor.count; index++)
                        {
                          _indexbuffer.push_back(buf[index] + vertexStart);
                        }
                      break;
                    }
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                      const auto* buf = reinterpret_cast<const uint8_t*>(
                          &buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                      for (size_t index = 0; index < accessor.count; index++)
                        {
                          _indexbuffer.push_back(buf[index] + vertexStart);
                        }
                      break;
                    }
                  default:
                    std::cerr << "Index component type " << accessor.componentType
                              << " not supported!" << '\n';
                    return;
                }
            }
            Primitive primitive{firstIndex,
                                indexCount,
                                glTF_primitive.material > -1 ? &materials[glTF_primitive.material]
                                                             : &materials.back()};
            primitive.firstVertex = vertexStart;
            primitive.vertexCount = vertexCount;
            primitive.materialIndex = glTF_primitive.material;
            mesh->primitives.push_back(primitive);
          }
          new_node->mesh = mesh;
        }
    }

  if (_parent)
    {
      _parent->children.push_back(new_node);
    }
  else
    {
      nodes.push_back(new_node);
    }
  linear_nodes.push_back(new_node);
}

void SngoEngine::Core::Source::Model::EngineGltfModel::load_nodes(
    const Device::LogicalDevice::EngineDevice* _device,
    VkCommandPool _pool,
    const tinygltf::Model& _input,
    uint32_t flags)
{
  std::vector<uint32_t> index_data;
  std::vector<GLTF_EngineModelVertexData> vertex_data;
  nodes.clear();

  const tinygltf::Scene& scene = _input.scenes[_input.defaultScene > -1 ? _input.defaultScene : 0];
  for (int it : scene.nodes)
    {
      const tinygltf::Node& node = _input.nodes[it];
      load_node(node, _input, it, nullptr, index_data, vertex_data);
    }

  for (auto node : linear_nodes)
    {
      // Assign skins
      if (node->skinIndex > -1)
        {
          node->skin = skins[node->skinIndex];
        }
      // Initial pose
      if (node->mesh)
        {
          node->update();
        }
    }

  // Pre-Calculations for requested features
  if ((flags & FileLoadingFlags::PreTransformVertices)
      || (flags & FileLoadingFlags::PreMultiplyVertexColors) || (flags & FileLoadingFlags::FlipY))
    {
      const bool preTransform = flags & FileLoadingFlags::PreTransformVertices;
      const bool preMultiplyColor = flags & FileLoadingFlags::PreMultiplyVertexColors;
      const bool flipY = flags & FileLoadingFlags::FlipY;
      for (GltfNode* node : linear_nodes)
        {
          if (node->mesh)
            {
              const glm::mat4 localMatrix = node->getMatrix();
              for (Primitive& primitive : node->mesh->primitives)
                {
                  for (uint32_t i = 0; i < primitive.vertexCount; i++)
                    {
                      GLTF_EngineModelVertexData& vertex = vertex_data[primitive.firstVertex + i];
                      // Pre-transform vertex positions by node-hierarchy
                      if (preTransform)
                        {
                          vertex.pos = glm::vec3(localMatrix * glm::vec4(vertex.pos, 1.0f));
                          vertex.normal = glm::normalize(glm::mat3(localMatrix) * vertex.normal);
                        }
                      // Flip Y-Axis of vertex positions
                      if (flipY)
                        {
                          vertex.pos.y *= -1.0f;
                          vertex.normal.y *= -1.0f;
                        }
                      // Pre-Multiply vertex colors with material base color
                      if (preMultiplyColor)
                        {
                          vertex.color = primitive.material->baseColor_factor * vertex.color;
                        }
                    }
                }
            }
        }
    }

  model.vertex_buffer.init(_device, _pool, _device->graphics_queue, vertex_data, Alloc);
  model.index_buffer.init(_device, _pool, _device->graphics_queue, index_data, Alloc);
}

void SngoEngine::Core::Source::Model::EngineGltfModel::load_skins(tinygltf::Model& input)
{
  // TODO: load skins
}

void SngoEngine::Core::Source::Model::EngineGltfModel::load_animations(tinygltf::Model& input)
{
  // TODO: load animations
}

void SngoEngine::Core::Source::Model::EngineGltfModel::load_materials(tinygltf::Model& input)
{
  materials.clear();
  for (int i = 0; i < input.materials.size(); i++)
    {
      materials.emplace_back(device);
    }

  gltf_LoadMaterial(input, imgs, materials);
  materials.emplace_back(device);
}

void SngoEngine::Core::Source::Model::EngineGltfModel::get_sceneDimensions(
    Primitive::Dimensions& dimensions)
{
  dimensions.min = glm::vec3(FLT_MAX);
  dimensions.max = glm::vec3(-FLT_MAX);
  for (auto node : nodes)
    {
      Get_NodeDimensions(node, dimensions.min, dimensions.max);
    }
  dimensions.size = dimensions.max - dimensions.min;
  dimensions.center = (dimensions.min + dimensions.max) / 2.0f;
  dimensions.radius = glm::distance(dimensions.min, dimensions.max) / 2.0f;
}

void SngoEngine::Core::Source::Model::EngineGltfModel::drawNode(VkCommandBuffer commandBuffer,
                                                                VkPipelineLayout pipelineLayout,
                                                                GltfNode* node,
                                                                uint32_t bindImage_set,
                                                                uint32_t renderFlags,
                                                                uint32_t frame_index)
{
  if (node->mesh)
    {
      // Pass the node's matrix via push constants
      // Traverse the node hierarchy to the top-most parent to get the final matrix of the current
      // node
      for (Primitive& primitive : node->mesh->primitives)
        {
          bool skip = false;
          const GltfMaterial* material = primitive.material;
          if (renderFlags & RenderFlags::RenderOpaqueNodes)
            {
              skip = (material->alphaMode != GltfMaterial::ALPHAMODE_OPAQUE);
            }
          if (renderFlags & RenderFlags::RenderAlphaMaskedNodes)
            {
              skip = (material->alphaMode != GltfMaterial::ALPHAMODE_MASK);
            }
          if (renderFlags & RenderFlags::RenderAlphaBlendedNodes)
            {
              skip = (material->alphaMode != GltfMaterial::ALPHAMODE_BLEND);
            }
          // Get the texture index for this primitive
          if (!skip)
            {
              if (renderFlags & RenderFlags::BindImages && material->base_color.is_available())
                {
                  // fmt::println("draw img");
                  vkCmdBindDescriptorSets(commandBuffer,
                                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                                          pipelineLayout,
                                          bindImage_set,
                                          1,
                                          &material->descriptor_set.descriptor_set,
                                          0,
                                          nullptr);
                }
              vkCmdDrawIndexed(commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, 0);
            }
        }
    }
  for (auto& child : node->children)
    {
      drawNode(commandBuffer, pipelineLayout, child, bindImage_set, renderFlags, frame_index);
    }
}

void SngoEngine::Core::Source::Model::EngineGltfModel::draw(VkCommandBuffer command_buffer,
                                                            VkPipelineLayout pipeline_layout,
                                                            uint32_t bindImage_set,
                                                            uint32_t renderFlags,
                                                            uint32_t frame_index)
{
  // must bind all buffer in this struct first
  // Render all nodes at top-level
  for (auto& node : nodes)
    {
      drawNode(command_buffer, pipeline_layout, node, bindImage_set, renderFlags, frame_index);
    }
}

void SngoEngine::Core::Source::Model::EngineGltfModel::destroyer()
{
  for (auto& node : nodes)
    delete node;
}

//===========================================================================================================================
// EngineCubeMap
//===========================================================================================================================

void SngoEngine::Core::Source::Model::EngineCubeMap::creator(
    const std::string& gltf_file,
    const std::string& boxTexture_file,
    const Device::LogicalDevice::EngineDevice* _device,
    Buffer::EngineCommandPool* _pool,
    const VkAllocationCallbacks* alloc)
{
  device = _device;
  Alloc = alloc;

  cube_img.init(device, _pool->command_pool, boxTexture_file, Alloc);
  model.init(gltf_file, device, _pool);
}

void SngoEngine::Core::Source::Model::EngineCubeMap::generate_descriptor(
    const Descriptor::EngineDescriptorPool& _pool,
    Descriptor::EngineDescriptorSetLayout& _layout,
    Descriptor::EngineDescriptorSet& _set,
    uint32_t binding)
{
  std::vector<VkDescriptorSetLayoutBinding> bindings{};
  bindings.push_back(Descriptor::GetLayoutBinding(
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, binding));

  _layout.init(device, bindings, Alloc);
  _set.init(device, _layout.layout, _pool.descriptor_pool);

  std::vector<VkWriteDescriptorSet> writes{};
  VkWriteDescriptorSet writeDescriptorSet{};

  writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writeDescriptorSet.descriptorCount = 1;
  writeDescriptorSet.dstSet = _set.descriptor_set;
  writeDescriptorSet.dstBinding = binding;
  writeDescriptorSet.pImageInfo = &cube_img.descriptor;
  writes.push_back(writeDescriptorSet);

  _set.updateWrite(writes);
}

void SngoEngine::Core::Source::Model::EngineCubeMap::destroyer()
{
  model.destroyer();
  cube_img.destroyer();
}
