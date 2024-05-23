#include "Descriptor.hpp"

#include <vulkan/vulkan_core.h>

#include <cassert>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "fmt/core.h"
#include "src/Core/Source/Image/ImageVIew.hpp"
#include "src/Core/Source/Image/Sampler.hpp"

VkDescriptorSetLayoutBinding SngoEngine::Core::Source::Descriptor::GetLayoutBinding_UBO()
{
  VkDescriptorSetLayoutBinding ubo_layout_binding{};
  ubo_layout_binding.binding = 0;
  ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  ubo_layout_binding.descriptorCount = 1;
  ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  ubo_layout_binding.pImmutableSamplers = nullptr;

  return ubo_layout_binding;
}

VkDescriptorSetLayoutBinding SngoEngine::Core::Source::Descriptor::GetLayoutBinding_Sampler(
    int starter_binding)
{
  VkDescriptorSetLayoutBinding sampler_layout_binding{};
  sampler_layout_binding.binding = starter_binding;
  sampler_layout_binding.descriptorCount = 1;
  sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  sampler_layout_binding.pImmutableSamplers = nullptr;
  sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  return sampler_layout_binding;
}

VkDescriptorSetLayoutBinding SngoEngine::Core::Source::Descriptor::GetLayoutBinding(
    VkDescriptorType type,
    VkShaderStageFlags stageFlags,
    uint32_t binding,
    uint32_t descriptorCount)
{
  VkDescriptorSetLayoutBinding sampler_layout_binding{};
  sampler_layout_binding.binding = binding;
  sampler_layout_binding.descriptorCount = descriptorCount;
  sampler_layout_binding.descriptorType = type;
  sampler_layout_binding.pImmutableSamplers = nullptr;
  sampler_layout_binding.stageFlags = stageFlags;
  return sampler_layout_binding;
}

VkDescriptorPoolSize SngoEngine::Core::Source::Descriptor::Get_DescriptorPoolSize(
    VkDescriptorType _type,
    uint32_t _count)
{
  VkDescriptorPoolSize pool_size{};
  pool_size.type = _type;
  pool_size.descriptorCount = _count;
  return pool_size;
}

//===========================================================================================================================
// EngineDescriptorSetLayout
//===========================================================================================================================

void SngoEngine::Core::Source::Descriptor::EngineDescriptorSetLayout::creator(
    const Device::LogicalDevice::EngineDevice* _device,
    const std::vector<VkDescriptorSetLayoutBinding>& _bingdings,
    const VkAllocationCallbacks* alloc)
{
  if (layout != VK_NULL_HANDLE)
    vkDestroyDescriptorSetLayout(device->logical_device, layout, Alloc);
  device = _device;
  Alloc = alloc;
  binding_size = _bingdings.size();

  VkDescriptorSetLayoutCreateInfo layout_info{};
  layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layout_info.bindingCount = _bingdings.size();
  layout_info.pBindings = _bingdings.data();

  if (vkCreateDescriptorSetLayout(device->logical_device, &layout_info, Alloc, &layout)
      != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create descriptor set layout!");
    }
}

void SngoEngine::Core::Source::Descriptor::EngineDescriptorSetLayout::destroyer()
{
  if (device)
    vkDestroyDescriptorSetLayout(device->logical_device, layout, Alloc);
}

//===========================================================================================================================
// EngineDescriptorPool
//===========================================================================================================================

void SngoEngine::Core::Source::Descriptor::EngineDescriptorPool::creator(
    const Device::LogicalDevice::EngineDevice* _device,
    uint32_t _maxSets,
    const std::vector<VkDescriptorPoolSize>& pool_sizes,
    const VkAllocationCallbacks* alloc)
{
  destroyer();
  Alloc = alloc;
  device = _device;

  VkDescriptorPoolCreateInfo pool_info{};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  pool_info.poolSizeCount = pool_sizes.size();
  pool_info.pPoolSizes = pool_sizes.data();
  pool_info.maxSets = _maxSets;

  if (vkCreateDescriptorPool(device->logical_device, &pool_info, Alloc, &descriptor_pool)
      != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create descriptor pool!");
    }
}

void SngoEngine::Core::Source::Descriptor::EngineDescriptorPool::destroyer()
{
  if (device)
    {
      vkDestroyDescriptorPool(device->logical_device, descriptor_pool, Alloc);
      descriptor_pool = VK_NULL_HANDLE;
    }
}

// void SngoEngine::Core::Source::Descriptor::EngineDescriptorPool::creator(
//     const Device::LogicalDevice::EngineDevice* _device,
//     uint32_t _maxSets,
//     uint32_t sampler_num,
//     const VkAllocationCallbacks* alloc)
// {
//   std::vector<VkDescriptorPoolSize> pool_sizes;
//   pool_sizes.resize(sampler_num + 1);

//   pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
//   pool_sizes[0].descriptorCount = static_cast<uint32_t>(_maxSets);

//   for (size_t i = 0; i < sampler_num; i++)
//     {
//       pool_sizes[i + 1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//       pool_sizes[i + 1].descriptorCount = static_cast<uint32_t>(_maxSets);
//     }

//   creator(_device, _maxSets, pool_sizes, alloc);
// }

//===========================================================================================================================
// EngineDescriptorSet
//===========================================================================================================================
void SngoEngine::Core::Source::Descriptor::EngineDescriptorSet::creator(
    const Device::LogicalDevice::EngineDevice* _device,
    const EngineDescriptorSetLayout* _set_layout,
    const EngineDescriptorPool* descriptor_pool)
{
  descriptor_set = VK_NULL_HANDLE;
  device = _device;

  VkDescriptorSetAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  alloc_info.descriptorPool = descriptor_pool->descriptor_pool;
  alloc_info.descriptorSetCount = 1;
  alloc_info.pSetLayouts = &_set_layout->layout;

  if (vkAllocateDescriptorSets(device->logical_device, &alloc_info, &descriptor_set) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to allocate descriptor set!");
    }
}

void SngoEngine::Core::Source::Descriptor::EngineDescriptorSet::creator(
    const Device::LogicalDevice::EngineDevice* _device,
    VkDescriptorSetLayout _layout,
    VkDescriptorPool _pool)
{
  descriptor_set = VK_NULL_HANDLE;
  device = _device;

  VkDescriptorSetAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  alloc_info.descriptorPool = _pool;
  alloc_info.descriptorSetCount = 1;
  alloc_info.pSetLayouts = &_layout;

  if (vkAllocateDescriptorSets(device->logical_device, &alloc_info, &descriptor_set) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to allocate descriptor set!");
    }
}

void SngoEngine::Core::Source::Descriptor::EngineDescriptorSet::creator(
    const Device::LogicalDevice::EngineDevice* _device,
    const EngineDescriptorSetLayout* _set_layout,
    const EngineDescriptorPool* _pool,
    VkWriteDescriptorSet* descriptor_write,
    uint32_t descriptorCopyCount,
    VkCopyDescriptorSet* pDescriptorCopies)
{
  creator(_device, _set_layout, _pool);
  updateWrite(descriptor_write, descriptorCopyCount, pDescriptorCopies);
}

void SngoEngine::Core::Source::Descriptor::EngineDescriptorSet::updateWrite(
    VkWriteDescriptorSet* descriptor_write,
    uint32_t descriptorCopyCount,
    VkCopyDescriptorSet* pDescriptorCopies)
{
  vkUpdateDescriptorSets(
      device->logical_device, 1, descriptor_write, descriptorCopyCount, pDescriptorCopies);
}

void SngoEngine::Core::Source::Descriptor::EngineDescriptorSet::updateWrite(
    const std::vector<VkWriteDescriptorSet>& descriptor_write,
    uint32_t descriptorCopyCount,
    VkCopyDescriptorSet* pDescriptorCopies)
{
  vkUpdateDescriptorSets(device->logical_device,
                         descriptor_write.size(),
                         descriptor_write.data(),
                         descriptorCopyCount,
                         pDescriptorCopies);
}

//===========================================================================================================================
// EngineDescriptorSets
//===========================================================================================================================

VkDescriptorBufferInfo SngoEngine::Core::Source::Descriptor::GetDescriptor_BufferInfo(
    VkBuffer _buffer,
    VkDeviceSize _range)
{
  VkDescriptorBufferInfo buffer_info{};
  buffer_info.buffer = _buffer;
  buffer_info.offset = 0;
  buffer_info.range = _range;

  return buffer_info;
}

VkDescriptorImageInfo SngoEngine::Core::Source::Descriptor::GetDescriptor_ImageInfo(
    VkImageView _view,
    VkSampler _sampler)
{
  VkDescriptorImageInfo image_info{};
  image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  image_info.imageView = _view;
  image_info.sampler = _sampler;

  return image_info;
}

std::vector<VkWriteDescriptorSet> SngoEngine::Core::Source::Descriptor::GetDescriptSet_Write(
    VkDescriptorSet dstSet,
    VkDescriptorType type,
    uint32_t binding,
    const VkDescriptorBufferInfo* bufferInfo,
    uint32_t descriptorCount)
{
  VkWriteDescriptorSet descriptor_writes{};
  descriptor_writes.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptor_writes.dstSet = dstSet;
  descriptor_writes.descriptorType = type;
  descriptor_writes.dstBinding = binding;
  descriptor_writes.pBufferInfo = bufferInfo;
  descriptor_writes.descriptorCount = descriptorCount;
  return {descriptor_writes};
}

std::vector<VkWriteDescriptorSet> SngoEngine::Core::Source::Descriptor::GetDescriptSet_Write(
    VkDescriptorSet dstSet,
    VkDescriptorType type,
    uint32_t binding,
    const VkDescriptorImageInfo* imageInfo,
    uint32_t descriptorCount)
{
  VkWriteDescriptorSet descriptor_writes{};
  descriptor_writes.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptor_writes.dstSet = dstSet;
  descriptor_writes.descriptorType = type;
  descriptor_writes.dstBinding = binding;
  descriptor_writes.pImageInfo = imageInfo;
  descriptor_writes.descriptorCount = descriptorCount;
  return {descriptor_writes};
}

std::vector<VkWriteDescriptorSet> SngoEngine::Core::Source::Descriptor::GetDescriptSet_Write(
    const std::vector<VkDescriptorImageInfo>& image_info,
    VkDescriptorSet descriptor_set)
{
  std::vector<VkWriteDescriptorSet> descriptor_writes(image_info.size());

  for (size_t j = 0; j < image_info.size(); j++)
    {
      descriptor_writes[j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptor_writes[j].dstSet = descriptor_set;
      descriptor_writes[j].dstBinding = j;
      descriptor_writes[j].dstArrayElement = 0;
      descriptor_writes[j].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      descriptor_writes[j].descriptorCount = 1;
      descriptor_writes[j].pImageInfo = &image_info[j];
    }

  return descriptor_writes;
}

inline std::vector<VkWriteDescriptorSet> SngoEngine::Core::Source::Descriptor::GetDescriptSet_Write(
    VkDescriptorBufferInfo buffer_info,
    const std::vector<VkDescriptorImageInfo>& image_info,
    VkDescriptorSet descriptor_set)
{
  std::vector<VkWriteDescriptorSet> descriptor_writes(1 + image_info.size());

  descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptor_writes[0].dstSet = descriptor_set;
  descriptor_writes[0].dstBinding = 0;
  descriptor_writes[0].dstArrayElement = 0;
  descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  descriptor_writes[0].descriptorCount = 1;
  descriptor_writes[0].pBufferInfo = &buffer_info;

  for (size_t j = 1; j <= image_info.size(); j++)
    {
      descriptor_writes[j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptor_writes[j].dstSet = descriptor_set;
      descriptor_writes[j].dstBinding = j;
      descriptor_writes[j].dstArrayElement = 0;
      descriptor_writes[j].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      descriptor_writes[j].descriptorCount = 1;
      descriptor_writes[j].pImageInfo = &image_info[j - 1];
    }

  return descriptor_writes;
}

inline std::vector<VkWriteDescriptorSet> SngoEngine::Core::Source::Descriptor::GetDescriptSet_Write(
    VkDescriptorBufferInfo buffer_info,
    const ImageView::EngineImageViews* views,
    const std::vector<Image::EngineSampler>& samplers,
    VkDescriptorSet descriptor_set)
{
  std::vector<VkWriteDescriptorSet> descriptor_writes(1 + views->size());

  descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptor_writes[0].dstSet = descriptor_set;
  descriptor_writes[0].dstBinding = 0;
  descriptor_writes[0].dstArrayElement = 0;
  descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  descriptor_writes[0].descriptorCount = 1;
  descriptor_writes[0].pBufferInfo = &buffer_info;

  std::vector<VkDescriptorImageInfo> image_infos;
  image_infos.resize(views->size());

  for (size_t j = 1; j <= image_infos.size(); j++)
    {
      VkDescriptorImageInfo imageInfo{};
      imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      imageInfo.imageView = views->operator[](j - 1);
      imageInfo.sampler = samplers[j - 1].sampler;
      image_infos[j - 1] = imageInfo;

      descriptor_writes[j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptor_writes[j].dstSet = descriptor_set;
      descriptor_writes[j].dstBinding = static_cast<uint32_t>(j);
      descriptor_writes[j].dstArrayElement = 0;
      descriptor_writes[j].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      descriptor_writes[j].descriptorCount = 1;
      descriptor_writes[j].pImageInfo = &image_infos[j - 1];
    }

  return descriptor_writes;
}

void SngoEngine::Core::Source::Descriptor::EngineDescriptorSets::creator(
    const Device::LogicalDevice::EngineDevice* _device,
    const EngineDescriptorSetLayout* _set_layout,
    const EngineDescriptorPool* _pool,
    uint32_t SIZE)
{
  destroyer();
  device = _device;

  VkDescriptorSetAllocateInfo alloc_info{};
  std::vector<VkDescriptorSetLayout> layout(SIZE, _set_layout->layout);

  alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  alloc_info.descriptorPool = _pool->descriptor_pool;
  alloc_info.descriptorSetCount = static_cast<uint32_t>(SIZE);
  alloc_info.pSetLayouts = layout.data();

  descriptor_sets.resize(SIZE);

  if (vkAllocateDescriptorSets(device->logical_device, &alloc_info, descriptor_sets.data())
      != VK_SUCCESS)
    {
      throw std::runtime_error("failed to allocate descriptor sets!");
    }
}

void SngoEngine::Core::Source::Descriptor::EngineDescriptorSets::creator(
    const Device::LogicalDevice::EngineDevice* _device,
    const EngineDescriptorSetLayout* _set_layout,
    const EngineDescriptorPool* _pool,
    uint32_t SIZE,
    std::vector<VkWriteDescriptorSet>& descriptor_writes,
    uint32_t descriptorCopyCount,
    VkCopyDescriptorSet* pDescriptorCopies)
{
  creator(_device, _set_layout, _pool, SIZE);
  updateWrite(descriptor_writes, descriptorCopyCount, pDescriptorCopies);
}

void SngoEngine::Core::Source::Descriptor::EngineDescriptorSets::updateWrite(
    const std::vector<VkWriteDescriptorSet>& writes,
    uint32_t descriptorCopyCount,
    VkCopyDescriptorSet* pDescriptorCopies)
{
  assert(writes.size() == descriptor_sets.size());
  vkUpdateDescriptorSets(
      device->logical_device, writes.size(), writes.data(), descriptorCopyCount, pDescriptorCopies);
}

void SngoEngine::Core::Source::Descriptor::EngineDescriptorSets::destroyer()
{
  descriptor_sets.clear();
}

VkDescriptorSet& SngoEngine::Core::Source::Descriptor::EngineDescriptorSets::operator[](size_t t)
{
  return descriptor_sets[t];
}

VkDescriptorSet* SngoEngine::Core::Source::Descriptor::EngineDescriptorSets::data()
{
  return descriptor_sets.data();
}

void SngoEngine::Core::Source::Descriptor::EngineDescriptorSets::resize(size_t t)
{
  descriptor_sets.resize(t);
}

size_t SngoEngine::Core::Source::Descriptor::EngineDescriptorSets::size() const
{
  return descriptor_sets.size();
}

std::vector<VkDescriptorSet>::iterator
SngoEngine::Core::Source::Descriptor::EngineDescriptorSets::begin()
{
  return descriptor_sets.begin();
}

std::vector<VkDescriptorSet>::iterator
SngoEngine::Core::Source::Descriptor::EngineDescriptorSets::end()
{
  return descriptor_sets.end();
}
