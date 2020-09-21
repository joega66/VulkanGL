#pragma once
#include <Engine/Types.h>
#include <vulkan/vulkan.h>

#define DECLARE_DESCRIPTOR_INDEX_TYPE(DescriptorIndexType) \
class DescriptorIndexType \
{ \
public: \
	DescriptorIndexType() = default; \
	DescriptorIndexType(uint32 descriptorIndex); \
	void Release(); \
	inline const uint32& Get() const { return _DescriptorIndex; } \
	inline operator uint32() const { return _DescriptorIndex; } \
protected: \
	uint32 _DescriptorIndex; \
}; \

namespace gpu
{
	DECLARE_DESCRIPTOR_INDEX_TYPE(TextureID);
	DECLARE_DESCRIPTOR_INDEX_TYPE(SamplerID);
	DECLARE_DESCRIPTOR_INDEX_TYPE(ImageID);

	class ImageView;
	class Sampler;

	class BindlessDescriptors
	{
		friend class TextureID;
		friend class SamplerID;
		friend class Sampler;
		friend class ImageID;

	public:
		BindlessDescriptors(const BindlessDescriptors&) = delete;
		BindlessDescriptors& operator=(const BindlessDescriptors&) = delete;

		BindlessDescriptors(VkDevice device, VkDescriptorType descriptorType, uint32 descriptorCount);
		~BindlessDescriptors();

		/** Create a texture ID for indexing into the texture2D array. */
		TextureID CreateTextureID(const ImageView& imageView);

		/** Create an image ID for indexing into the storage image array. */
		ImageID CreateImageID(const ImageView& imageView);

		/** Called in VulkanDevice::EndFrame(). */
		void EndFrame();

		inline VkDescriptorSetLayout GetLayout() const { return _DescriptorSetLayout; }
		inline operator VkDescriptorSet() const { return _DescriptorSet; }

	private:
		VkDevice _Device;
		const uint32 _MaxDescriptorCount;
		VkDescriptorSetLayout _DescriptorSetLayout;
		VkDescriptorPool _DescriptorPool;
		VkDescriptorSet _DescriptorSet;
		uint32 _NumDescriptors = 0;
		std::list<uint32> _Available;
		std::list<uint32> _Released;

		/** Creates a SamplerID. Samplers call this automatically. */
		SamplerID CreateSamplerID(const Sampler& sampler);

		/** Allocates an index into a bindless descriptor table. */
		uint32 AllocateDescriptorIndex();

		/** Release a descriptor index. */
		void Release(uint32 descriptorIndex);
	};
};

extern std::weak_ptr<gpu::BindlessDescriptors> gBindlessTextures;
extern std::weak_ptr<gpu::BindlessDescriptors> gBindlessSamplers;
extern std::weak_ptr<gpu::BindlessDescriptors> gBindlessImages;