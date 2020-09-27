#include "VulkanBindlessDescriptors.h"
#include "VulkanDevice.h"

std::weak_ptr<gpu::BindlessDescriptors> gBindlessTextures;
std::weak_ptr<gpu::BindlessDescriptors> gBindlessSamplers;
std::weak_ptr<gpu::BindlessDescriptors> gBindlessImages;

namespace gpu
{
	BindlessDescriptors::BindlessDescriptors(VkDevice device, VkDescriptorType descriptorType, uint32 descriptorCount)
		: _Device(device)
		, _MaxDescriptorCount(descriptorCount)
	{
		constexpr VkDescriptorBindingFlags bindingFlags = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;

		const VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo = 
		{ 
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
			.bindingCount = 1,
			.pBindingFlags = &bindingFlags
		};

		const VkDescriptorSetLayoutBinding binding =
		{
			.binding = 0,
			.descriptorType = descriptorType,
			.descriptorCount = descriptorCount,
			.stageFlags = VK_SHADER_STAGE_ALL
		};
		
		const VkDescriptorSetLayoutCreateInfo setLayoutInfo = 
		{ 
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.pNext = &bindingFlagsInfo,
			.bindingCount = 1,
			.pBindings = &binding,
		};
		
		VkDescriptorSetVariableDescriptorCountLayoutSupport variableDescriptorCountSupport = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_LAYOUT_SUPPORT };
		VkDescriptorSetLayoutSupport support = 
		{ 
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_SUPPORT,
			.pNext = &variableDescriptorCountSupport
		};

		vkGetDescriptorSetLayoutSupport(device, &setLayoutInfo, &support);

		vulkan(vkCreateDescriptorSetLayout(device, &setLayoutInfo, nullptr, &_DescriptorSetLayout));

		const VkDescriptorPoolSize descriptorPoolSize =
		{
			.type = descriptorType,
			.descriptorCount = descriptorCount,
		};
	
		const VkDescriptorPoolCreateInfo descriptorPoolInfo = 
		{ 
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.maxSets = 1,
			.poolSizeCount = 1,
			.pPoolSizes = &descriptorPoolSize,
		};

		vulkan(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &_DescriptorPool));

		const VkDescriptorSetVariableDescriptorCountAllocateInfo variableDescriptorCountInfo = 
		{ 
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO,
			.descriptorSetCount = 1,
			.pDescriptorCounts = &descriptorCount
		};
		
		const VkDescriptorSetAllocateInfo setAllocateInfo = 
		{ 
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.pNext = &variableDescriptorCountInfo,
			.descriptorPool = _DescriptorPool,
			.descriptorSetCount = 1,
			.pSetLayouts = &_DescriptorSetLayout
		};

		vulkan(vkAllocateDescriptorSets(device, &setAllocateInfo, &_DescriptorSet));
	}

	BindlessDescriptors::~BindlessDescriptors()
	{
		vkDestroyDescriptorSetLayout(_Device, _DescriptorSetLayout, nullptr);
		vkDestroyDescriptorPool(_Device, _DescriptorPool, nullptr);
	}

	TextureID BindlessDescriptors::CreateTextureID(const gpu::ImageView& imageView)
	{
		static auto chooseImageLayout = [] (EFormat format)
		{
			if (gpu::ImagePrivate::IsColor(format))
			{
				return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			}
			else if (gpu::ImagePrivate::IsDepthStencil(format))
			{
				return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			}
			else if (gpu::ImagePrivate::IsDepth(format))
			{
				return VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
			}
			else // Image.IsStencil()
			{
				return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
			}
		};

		const VkDescriptorImageInfo imageInfo = { nullptr, imageView.GetHandle(), chooseImageLayout(imageView.GetFormat()) };

		const VkWriteDescriptorSet writeDescriptorSet = 
		{ 
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = _DescriptorSet,
			.dstBinding = 0,
			.dstArrayElement = Allocate(),
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
			.pImageInfo = &imageInfo,
		};

		vkUpdateDescriptorSets(_Device, 1, &writeDescriptorSet, 0, nullptr);

		return TextureID(writeDescriptorSet.dstArrayElement);
	}

	ImageID BindlessDescriptors::CreateImageID(const gpu::ImageView& imageView)
	{
		const VkDescriptorImageInfo imageInfo = { nullptr, imageView.GetHandle(), VK_IMAGE_LAYOUT_GENERAL };

		const VkWriteDescriptorSet writeDescriptorSet =
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = _DescriptorSet,
			.dstBinding = 0,
			.dstArrayElement = Allocate(),
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.pImageInfo = &imageInfo,
		};

		vkUpdateDescriptorSets(_Device, 1, &writeDescriptorSet, 0, nullptr);

		return ImageID(writeDescriptorSet.dstArrayElement);
	}

	SamplerID BindlessDescriptors::CreateSamplerID(const gpu::Sampler& sampler)
	{
		const VkDescriptorImageInfo imageInfo = { sampler.GetHandle(), nullptr, {} };

		const VkWriteDescriptorSet writeDescriptorSet =
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = _DescriptorSet,
			.dstBinding = 0,
			.dstArrayElement = Allocate(),
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
			.pImageInfo = &imageInfo,
		};

		vkUpdateDescriptorSets(_Device, 1, &writeDescriptorSet, 0, nullptr);

		return SamplerID(writeDescriptorSet.dstArrayElement);
	}

	uint32 BindlessDescriptors::Allocate()
	{
		uint32 dstArrayElement;

		if (_Available.empty())
		{
			check(_NumDescriptors < _MaxDescriptorCount, "Out of descriptor indexes!");
			dstArrayElement = _NumDescriptors++;
		}
		else
		{
			dstArrayElement = _Available.front();
			_Available.pop_front();
		}

		return dstArrayElement;
	}

	void BindlessDescriptors::Release(uint32 descriptorIndex)
	{
		_Released.push_back(descriptorIndex);
	}

	void BindlessDescriptors::EndFrame()
	{
		for (auto id : _Released)
		{
			_Available.push_back(id);
		}

		_Released.clear();
	}
	static constexpr uint32 NULL_DESCRIPTOR_INDEX = -1;

#define DEFINE_DESCRIPTOR_INDEX_TYPE(ResourceType) \
	ResourceType::ResourceType(uint32 descriptorIndex) \
		: _DescriptorIndex(descriptorIndex) \
	{ \
	} \

	DEFINE_DESCRIPTOR_INDEX_TYPE(TextureID);

	void TextureID::Release()
	{
		if (auto textures = gBindlessTextures.lock(); _DescriptorIndex != NULL_DESCRIPTOR_INDEX && textures)
		{
			textures->Release(_DescriptorIndex);
			_DescriptorIndex = NULL_DESCRIPTOR_INDEX;
		}
	}

	DEFINE_DESCRIPTOR_INDEX_TYPE(SamplerID);

	void SamplerID::Release()
	{
		if (auto samplers = gBindlessSamplers.lock(); _DescriptorIndex != NULL_DESCRIPTOR_INDEX && samplers)
		{
			samplers->Release(_DescriptorIndex);
			_DescriptorIndex = NULL_DESCRIPTOR_INDEX;
		}
	}

	DEFINE_DESCRIPTOR_INDEX_TYPE(ImageID);

	void ImageID::Release()
	{
		if (auto images = gBindlessImages.lock(); _DescriptorIndex != NULL_DESCRIPTOR_INDEX && images)
		{
			images->Release(_DescriptorIndex);
			_DescriptorIndex = NULL_DESCRIPTOR_INDEX;
		}
	}
};