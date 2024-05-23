#ifndef __SNGO_DESCRIPTOR_H
#define __SNGO_DESCRIPTOR_H

#include <vulkan/vulkan_core.h>

#include <vector>

#include "src/Core/Device/LogicalDevice.hpp"
#include "src/Core/Source/Buffer/UniformBuffer.hpp"
#include "src/Core/Source/Image/ImageVIew.hpp"
#include "src/Core/Source/Image/Sampler.hpp"

namespace SngoEngine::Core::Source::Descriptor
{

VkDescriptorSetLayoutBinding GetLayoutBinding_UBO();
VkDescriptorSetLayoutBinding GetLayoutBinding_Sampler(int starter_binding = 1);
VkDescriptorSetLayoutBinding GetLayoutBinding(VkDescriptorType type,
                                              VkShaderStageFlags stageFlags,
                                              uint32_t binding,
                                              uint32_t descriptorCount = 1);

VkDescriptorPoolSize Get_DescriptorPoolSize(VkDescriptorType _type, uint32_t _count);

//===========================================================================================================================
// EngineDescriptorSetLayout
//===========================================================================================================================

struct EngineDescriptorSetLayout
{
  EngineDescriptorSetLayout() = default;
  EngineDescriptorSetLayout(EngineDescriptorSetLayout&&) noexcept = default;
  EngineDescriptorSetLayout& operator=(EngineDescriptorSetLayout&&) noexcept = default;
  template <class... Args>
  explicit EngineDescriptorSetLayout(const Device::LogicalDevice::EngineDevice* _device,
                                     Args... args)
  {
    creator(_device, args...);
  }
  template <class... Args>
  void init(const Device::LogicalDevice::EngineDevice* _device, Args... args)
  {
    creator(_device, args...);
  }
  ~EngineDescriptorSetLayout()
  {
    destroyer();
  }
  void destroyer();

  VkDescriptorSetLayout layout{};
  size_t binding_size{};
  const Device::LogicalDevice::EngineDevice* device{};

 private:
  void creator(const Device::LogicalDevice::EngineDevice* _device,
               const std::vector<VkDescriptorSetLayoutBinding>& _bingdings,
               const VkAllocationCallbacks* alloc = nullptr);
  const VkAllocationCallbacks* Alloc{};
};

//===========================================================================================================================
// EngineDescriptorPool
//===========================================================================================================================

struct EngineDescriptorPool
{
  EngineDescriptorPool() = default;
  EngineDescriptorPool(EngineDescriptorPool&&) noexcept = default;
  EngineDescriptorPool& operator=(EngineDescriptorPool&&) noexcept = default;
  template <class... Args>
  explicit EngineDescriptorPool(const Device::LogicalDevice::EngineDevice* _device, Args... args)
  {
    creator(_device, args...);
  }
  template <class... Args>
  void init(const Device::LogicalDevice::EngineDevice* _device, Args... args)
  {
    creator(_device, args...);
  }
  ~EngineDescriptorPool()
  {
    destroyer();
  }
  void destroyer();

  VkDescriptorPool descriptor_pool{};
  const Device::LogicalDevice::EngineDevice* device{};

 private:
  void creator(const Device::LogicalDevice::EngineDevice* _device,
               uint32_t _maxSets,
               const std::vector<VkDescriptorPoolSize>& pool_sizes,
               const VkAllocationCallbacks* alloc = nullptr);

  const VkAllocationCallbacks* Alloc{};
};

//===========================================================================================================================
// EngineDescriptorSet
//===========================================================================================================================

struct EngineDescriptorSet
{
  EngineDescriptorSet() = default;
  EngineDescriptorSet(EngineDescriptorSet&&) noexcept = default;
  EngineDescriptorSet& operator=(EngineDescriptorSet&&) noexcept = default;
  template <class... Args>
  explicit EngineDescriptorSet(const Device::LogicalDevice::EngineDevice* _device, Args... args)
  {
    creator(_device, args...);
  }
  template <class... Args>
  void init(const Device::LogicalDevice::EngineDevice* _device, Args... args)
  {
    creator(_device, args...);
  }
  explicit operator VkDescriptorSet() const
  {
    return descriptor_set;
  }
  ~EngineDescriptorSet() = default;

  void updateWrite(VkWriteDescriptorSet* descriptor_write,
                   uint32_t descriptorCopyCount = 0,
                   VkCopyDescriptorSet* pDescriptorCopies = nullptr);
  void updateWrite(const std::vector<VkWriteDescriptorSet>& descriptor_write,
                   uint32_t descriptorCopyCount = 0,
                   VkCopyDescriptorSet* pDescriptorCopies = nullptr);

  VkDescriptorSet descriptor_set{};
  const Device::LogicalDevice::EngineDevice* device{};

 private:
  void creator(const Device::LogicalDevice::EngineDevice* _device,
               const EngineDescriptorSetLayout* _set_layout,
               const EngineDescriptorPool* descriptor_pool);
  void creator(const Device::LogicalDevice::EngineDevice* _device,
               VkDescriptorSetLayout _layout,
               VkDescriptorPool _pool);
  void creator(const Device::LogicalDevice::EngineDevice* _device,
               const EngineDescriptorSetLayout* _set_layout,
               const EngineDescriptorPool* _pool,
               VkWriteDescriptorSet* descriptor_write,
               uint32_t descriptorCopyCount = 0,
               VkCopyDescriptorSet* pDescriptorCopies = nullptr);
};

//===========================================================================================================================
// EngineDescriptorSets
//===========================================================================================================================

VkDescriptorBufferInfo GetDescriptor_BufferInfo(VkBuffer _buffer, VkDeviceSize _range);
VkDescriptorImageInfo GetDescriptor_ImageInfo(VkImageView _view, VkSampler _sampler);
std::vector<VkWriteDescriptorSet> GetDescriptSet_Write(VkDescriptorSet dstSet,
                                                       VkDescriptorType type,
                                                       uint32_t binding,
                                                       const VkDescriptorImageInfo* imageInfo,
                                                       uint32_t descriptorCount = 1);
std::vector<VkWriteDescriptorSet> GetDescriptSet_Write(VkDescriptorSet dstSet,
                                                       VkDescriptorType type,
                                                       uint32_t binding,
                                                       const VkDescriptorBufferInfo* bufferInfo,
                                                       uint32_t descriptorCount = 1);
std::vector<VkWriteDescriptorSet> GetDescriptSet_Write(
    const std::vector<VkDescriptorImageInfo>& image_info,
    VkDescriptorSet descriptor_set);
std::vector<VkWriteDescriptorSet> GetDescriptSet_Write(
    VkDescriptorBufferInfo buffer_info,
    const std::vector<VkDescriptorImageInfo>& image_info,
    VkDescriptorSet descriptor_set);
std::vector<VkWriteDescriptorSet> GetDescriptSet_Write(
    VkDescriptorBufferInfo buffer_info,
    const ImageView::EngineImageViews* views,
    const std::vector<Image::EngineSampler>& samplers,
    VkDescriptorSet descriptor_set);

template <typename T>
std::vector<VkDescriptorBufferInfo> GetDescriptor_BufferInfo_s(
    std::vector<Buffer::EngineUniformBuffer<T>> buffers)
{
  std::vector<VkDescriptorBufferInfo> infos;

  for (auto& buffer : buffers)
    {
      VkDescriptorBufferInfo buffer_info{};
      buffer_info.buffer = buffer.buffer;
      buffer_info.offset = 0;
      buffer_info.range = buffer.range;

      infos.push_back(buffer_info);
    }
  return infos;
}

struct EngineDescriptorSets
{
  EngineDescriptorSets() = default;
  EngineDescriptorSets(EngineDescriptorSets&&) noexcept = default;
  EngineDescriptorSets& operator=(EngineDescriptorSets&&) noexcept = default;
  template <class... Args>
  explicit EngineDescriptorSets(const Device::LogicalDevice::EngineDevice* _device, Args... args)
  {
    creator(_device, args...);
  }
  template <class... Args>
  void init(const Device::LogicalDevice::EngineDevice* _device, Args... args)
  {
    creator(_device, args...);
  }
  ~EngineDescriptorSets() = default;
  VkDescriptorSet& operator[](size_t t);
  VkDescriptorSet* data();
  void resize(size_t t);
  [[nodiscard]] size_t size() const;
  std::vector<VkDescriptorSet>::iterator begin();
  std::vector<VkDescriptorSet>::iterator end();
  void destroyer();
  void updateWrite(const std::vector<VkWriteDescriptorSet>& writes,
                   uint32_t descriptorCopyCount = 0,
                   VkCopyDescriptorSet* pDescriptorCopies = nullptr);

  std::vector<VkDescriptorSet> descriptor_sets{};
  const Device::LogicalDevice::EngineDevice* device{};

 private:
  void creator(const Device::LogicalDevice::EngineDevice* _device,
               const EngineDescriptorSetLayout* _set_layout,
               const EngineDescriptorPool* _pool,
               uint32_t SIZE);
  void creator(const Device::LogicalDevice::EngineDevice* _device,
               const EngineDescriptorSetLayout* _set_layout,
               const EngineDescriptorPool* _pool,
               uint32_t SIZE,
               std::vector<VkWriteDescriptorSet>& descriptor_writes,
               uint32_t descriptorCopyCount = 0,
               VkCopyDescriptorSet* pDescriptorCopies = nullptr);
};

}  // namespace SngoEngine::Core::Source::Descriptor

#endif