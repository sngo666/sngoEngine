#include "Model.hpp"

#include <vulkan/vulkan_core.h>

#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_float4.hpp>
#include <stdexcept>
#include <vector>

#include "src/Core/Data.h"
#include "src/Core/Device/LogicalDevice.hpp"
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
    std::vector<Image::EngineSampler>& samplers,
    const VkAllocationCallbacks* alloc)
{
  images.resize(input.images.size());
  samplers.resize(input.images.size());
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

      images[i](device,
                _pool,
                Image::EnginePixelData{buffer,
                                       static_cast<unsigned int>(glTFImage.width),
                                       static_cast<unsigned int>(glTFImage.height)},
                alloc);

      samplers[i](device, Image::Get_Default_Sampler(device), alloc);
      if (deleteBuffer)
        {
          delete[] buffer;
        }
    }
}

void SngoEngine::Core::Source::Model::gltf_LoadTextures(
    tinygltf::Model& input,
    std::vector<Image::EngineTextureImage>& _imgs,
    std::vector<uint32_t>& _textures)
{
  _textures.resize(input.textures.size());
  for (size_t i = 0; i < input.textures.size(); i++)
    {
      _textures[i] = input.textures[i].source;
    }
}

void SngoEngine::Core::Source::Model::gltf_LoadMaterial(
    tinygltf::Model& input,
    std::vector<Image::EngineTextureImage>& _imgs,
    std::vector<GltfMaterial>& _materials)
{
  _materials.resize(input.materials.size());
  for (size_t i = 0; i < input.materials.size(); i++)
    {
      // We only read the most basic properties required for our sample
      tinygltf::Material glTFMaterial = input.materials[i];
      // Get the base color factor
      if (glTFMaterial.values.find("baseColorFactor") != glTFMaterial.values.end())
        {
          _materials[i].baseColorFactor =
              glm::make_vec4(glTFMaterial.values["baseColorFactor"].ColorFactor().data());
        }
      // Get base color texture index
      if (glTFMaterial.values.find("baseColorTexture") != glTFMaterial.values.end())
        {
          _materials[i].baseColor_index = glTFMaterial.values["baseColorTexture"].TextureIndex();
        }
    }
}

void SngoEngine::Core::Source::Model::load_node(
    const tinygltf::Node& _node,
    const tinygltf::Model& _input,
    GltfNode* _parent,
    std::vector<uint32_t>& _indexbuffer,
    std::vector<Source::Buffer::GLTF_EngineModelVertexData>& _vertexbuffer,
    std::vector<GltfNode*>& nodes)
{
  auto* node = new GltfNode{};
  node->matrix = glm::mat4(1.0f);
  node->parent = _parent;

  if (_node.translation.size() == 3)
    {
      node->matrix =
          glm::translate(node->matrix, glm::vec3(glm::make_vec3(_node.translation.data())));
    }
  if (_node.rotation.size() == 4)
    {
      glm::quat q = glm::make_quat(_node.rotation.data());
      node->matrix *= glm::mat4(q);
    }
  if (_node.scale.size() == 3)
    {
      node->matrix = glm::scale(node->matrix, glm::vec3(glm::make_vec3(_node.scale.data())));
    }
  if (_node.matrix.size() == 16)
    {
      node->matrix = glm::make_mat4x4(_node.matrix.data());
    };

  // Load node's children
  if (!_node.children.empty())
    {
      for (int it : _node.children)
        {
          load_node(_input.nodes[it], _input, node, _indexbuffer, _vertexbuffer, nodes);
        }
    }

  if (_node.mesh > -1)
    {
      const tinygltf::Mesh mesh = _input.meshes[_node.mesh];
      // Iterate through all primitives of this node's mesh
      for (const auto& glTF_primitive : mesh.primitives)
        {
          auto firstIndex = static_cast<uint32_t>(_indexbuffer.size());
          auto vertexStart = static_cast<uint32_t>(_vertexbuffer.size());
          uint32_t indexCount = 0;
          // Vertices
          {
            const float* positionBuffer = nullptr;
            const float* normalsBuffer = nullptr;
            const float* texCoordsBuffer = nullptr;
            size_t vertexCount = 0;

            // Get buffer data for vertex positions
            if (glTF_primitive.attributes.find("POSITION") != glTF_primitive.attributes.end())
              {
                const tinygltf::Accessor& accessor =
                    _input.accessors[glTF_primitive.attributes.find("POSITION")->second];
                const tinygltf::BufferView& view = _input.bufferViews[accessor.bufferView];

                positionBuffer = reinterpret_cast<const float*>(
                    &(_input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                vertexCount = accessor.count;
              }
            // Get buffer data for vertex normals
            if (glTF_primitive.attributes.find("NORMAL") != glTF_primitive.attributes.end())
              {
                const tinygltf::Accessor& accessor =
                    _input.accessors[glTF_primitive.attributes.find("NORMAL")->second];
                const tinygltf::BufferView& view = _input.bufferViews[accessor.bufferView];
                normalsBuffer = reinterpret_cast<const float*>(
                    &(_input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
              }
            // Get buffer data for vertex texture coordinates
            // glTF supports multiple sets, we only load the first one
            if (glTF_primitive.attributes.find("TEXCOORD_0") != glTF_primitive.attributes.end())
              {
                const tinygltf::Accessor& accessor =
                    _input.accessors[glTF_primitive.attributes.find("TEXCOORD_0")->second];
                const tinygltf::BufferView& view = _input.bufferViews[accessor.bufferView];
                texCoordsBuffer = reinterpret_cast<const float*>(
                    &(_input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
              }

            // Append data to model's vertex buffer
            for (size_t v = 0; v < vertexCount; v++)
              {
                SngoEngine::Core::Source::Buffer::GLTF_EngineModelVertexData vert{};
                vert.pos = glm::vec4(glm::make_vec3(&positionBuffer[v * 3]), 1.0f);
                vert.normal = glm::normalize(glm::vec3(
                    normalsBuffer ? glm::make_vec3(&normalsBuffer[v * 3]) : glm::vec3(0.0f)));
                vert.tex_coord =
                    texCoordsBuffer ? glm::make_vec2(&texCoordsBuffer[v * 2]) : glm::vec3(0.0f);
                vert.color = glm::vec3(1.0f);
                _vertexbuffer.push_back(vert);
              }
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
          Primitive primitive{};
          primitive.firstIndex = firstIndex;
          primitive.indexCount = indexCount;
          primitive.materialIndex = glTF_primitive.material;
          node->mesh.primitives.push_back(primitive);
        }
    }

  if (_parent)
    {
      _parent->children.push_back(node);
    }
  else
    {
      nodes.push_back(node);
    }
}

void SngoEngine::Core::Source::Model::gltf_LoadNode(
    const Device::LogicalDevice::EngineDevice* _device,
    VkCommandPool _pool,
    const tinygltf::Model& _input,
    Buffer::EngineVertexBuffer<Buffer::GLTF_EngineModelVertexData, 4>& vertex,
    Buffer::EngineIndexBuffer<uint32_t>& index,
    std::vector<GltfNode*>& nodes,
    const VkAllocationCallbacks* alloc)
{
  std::vector<uint32_t> index_data;
  std::vector<Buffer::GLTF_EngineModelVertexData> vertex_data;
  nodes.clear();

  const tinygltf::Scene& scene = _input.scenes[0];
  for (int it : scene.nodes)
    {
      const tinygltf::Node node = _input.nodes[it];
      load_node(node, _input, nullptr, index_data, vertex_data, nodes);
    }

  vertex(_device, _pool, _device->graphics_queue, vertex_data, alloc);
  index(_device, _pool, _device->graphics_queue, index_data, alloc);
}

void SngoEngine::Core::Source::Model::EngineGltfModel::drawNode(VkCommandBuffer commandBuffer,
                                                                VkPipelineLayout pipelineLayout,
                                                                GltfNode* node,
                                                                uint32_t frame_index)
{
  if (!node->mesh.primitives.empty())
    {
      // Pass the node's matrix via push constants
      // Traverse the node hierarchy to the top-most parent to get the final matrix of the current
      // node
      glm::mat4 nodeMatrix = node->matrix;
      GltfNode* currentParent = node->parent;
      while (currentParent)
        {
          nodeMatrix = currentParent->matrix * nodeMatrix;
          currentParent = currentParent->parent;
        }
      // Pass the final matrix to the vertex shader using push constants
      vkCmdPushConstants(commandBuffer,
                         pipelineLayout,
                         VK_SHADER_STAGE_VERTEX_BIT,
                         0,
                         sizeof(glm::mat4),
                         &nodeMatrix);
      for (Primitive& primitive : node->mesh.primitives)
        {
          if (primitive.indexCount > 0)
            {
              // Get the texture index for this primitive
              auto tex = tex_index[materials[primitive.materialIndex].baseColor_index];
              // Bind the descriptor for the current primitive's texture
              vkCmdBindDescriptorSets(commandBuffer,
                                      VK_PIPELINE_BIND_POINT_GRAPHICS,
                                      pipelineLayout,
                                      1,
                                      1,
                                      &descript_sets.img_sets[tex],
                                      0,
                                      nullptr);
              vkCmdDrawIndexed(commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, 0);
            }
        }
    }
  for (auto& child : node->children)
    {
      drawNode(commandBuffer, pipelineLayout, child, frame_index);
    }
}

void SngoEngine::Core::Source::Model::EngineGltfModel::draw(VkCommandBuffer command_buffer,
                                                            VkPipelineLayout pipeline_layout,
                                                            uint32_t frame_index)
{
  VkDeviceSize offsets[1] = {0};
  vkCmdBindVertexBuffers(command_buffer, 0, 1, &model.vertex_buffer.buffer, offsets);
  vkCmdBindIndexBuffer(command_buffer, model.index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);
  vkCmdBindDescriptorSets(command_buffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipeline_layout,
                          0,
                          1,
                          &descript_sets.matrices_set[0],
                          0,
                          nullptr);
  // Render all nodes at top-level
  for (auto& node : nodes)
    {
      drawNode(command_buffer, pipeline_layout, node, frame_index);
    }
}
