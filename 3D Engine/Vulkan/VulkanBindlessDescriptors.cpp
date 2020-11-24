#include "VulkanBindlessDescriptors.h"
#include "VulkanDevice.h"

VulkanBindlessDescriptors::VulkanBindlessDescriptors(VkDevice device, VkDescriptorType descriptorType, uint32 descriptorCount)
	: _Device(device)
	, _MaxDescriptorCount(descriptorCount)
{
	constexpr VkDescriptorBindingFlags bindingFlags = 
		VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
		VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;

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

VulkanBindlessDescriptors::~VulkanBindlessDescriptors()
{
	vkDestroyDescriptorSetLayout(_Device, _DescriptorSetLayout, nullptr);
	vkDestroyDescriptorPool(_Device, _DescriptorPool, nullptr);
}

gpu::TextureID VulkanBindlessDescriptors::CreateTextureID(const gpu::ImageView& imageView, const gpu::Sampler& sampler)
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

	const VkDescriptorImageInfo imageInfo = { sampler, imageView, chooseImageLayout(imageView.GetFormat()) };

	const VkWriteDescriptorSet writeDescriptorSet = 
	{ 
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = _DescriptorSet,
		.dstBinding = 0,
		.dstArrayElement = Allocate(),
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.pImageInfo = &imageInfo,
	};

	vkUpdateDescriptorSets(_Device, 1, &writeDescriptorSet, 0, nullptr);

	return gpu::TextureID(writeDescriptorSet.dstArrayElement);
}

gpu::ImageID VulkanBindlessDescriptors::CreateImageID(const gpu::ImageView& imageView)
{
	const VkDescriptorImageInfo imageInfo = { nullptr, imageView, VK_IMAGE_LAYOUT_GENERAL };

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

	return gpu::ImageID(writeDescriptorSet.dstArrayElement);
}

uint32 VulkanBindlessDescriptors::Allocate()
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

void VulkanBindlessDescriptors::Release(uint32 descriptorIndex)
{
	_Released.push_back(descriptorIndex);
}

void VulkanBindlessDescriptors::EndFrame()
{
	for (auto id : _Released)
	{
		_Available.push_back(id);
	}

	_Released.clear();
}