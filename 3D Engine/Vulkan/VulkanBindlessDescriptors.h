#pragma once
#include <Engine/Types.h>
#include <vulkan/vulkan.h>

#define DECLARE_DESCRIPTOR_INDEX_TYPE(DescriptorIndexType)				\
	class DescriptorIndexType											\
	{																	\
	public:																\
		DescriptorIndexType() = default;								\
		DescriptorIndexType(uint32 descriptorIndex) : _DescriptorIndex(descriptorIndex) {}	\
		inline operator uint32() const { return _DescriptorIndex; }		\
		inline bool IsValid() const { return _DescriptorIndex != -1; }	\
	protected:															\
		uint32 _DescriptorIndex = -1;									\
	};																	\

namespace gpu
{
	DECLARE_DESCRIPTOR_INDEX_TYPE(TextureID);
	DECLARE_DESCRIPTOR_INDEX_TYPE(ImageID);
	class ImageView;
	class Sampler;
}

class VulkanBindlessDescriptors
{
public:
	VulkanBindlessDescriptors(const VulkanBindlessDescriptors&) = delete;
	VulkanBindlessDescriptors& operator=(const VulkanBindlessDescriptors&) = delete;
	VulkanBindlessDescriptors() = default;
	VulkanBindlessDescriptors(VkDevice device, VkDescriptorType descriptorType, uint32 descriptorCount);
	~VulkanBindlessDescriptors();

	/** Create a texture ID for indexing into the texture2D array. */
	gpu::TextureID CreateTextureID(const gpu::ImageView& imageView, const gpu::Sampler& sampler);

	/** Create an image ID for indexing into the storage image array. */
	gpu::ImageID CreateImageID(const gpu::ImageView& imageView);

	/** Called in VulkanDevice::EndFrame(). */
	void EndFrame();

	/** Release a descriptor index. */
	void Release(uint32 descriptorIndex);

	inline VkDescriptorSetLayout GetLayout() const { return _DescriptorSetLayout; }
	inline operator VkDescriptorSet&() { return _DescriptorSet; }

private:
	VkDevice				_Device;
	uint32					_MaxDescriptorCount;
	VkDescriptorSetLayout	_DescriptorSetLayout;
	VkDescriptorPool		_DescriptorPool;
	VkDescriptorSet			_DescriptorSet;
	uint32					_NumDescriptors = 0;
	std::list<uint32>		_Available;
	std::list<uint32>		_Released;

	/** Allocate an index into the bindless descriptor table. */
	uint32 Allocate();
};