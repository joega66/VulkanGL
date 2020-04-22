#include "VulkanBindlessResources.h"
#include "VulkanDevice.h"

VulkanBindlessResources::VulkanBindlessResources(VkDevice Device, VkDescriptorType ResourceType, uint32 ResourceCount)
	: Device(Device)
{
	constexpr VkDescriptorBindingFlags BindingFlags = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;

	VkDescriptorSetLayoutBindingFlagsCreateInfo BindingFlagsInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO };
	BindingFlagsInfo.bindingCount = 1;
	BindingFlagsInfo.pBindingFlags = &BindingFlags;

	VkDescriptorSetLayoutBinding Binding = {};
	Binding.binding = 0;
	Binding.descriptorType = ResourceType;
	Binding.descriptorCount = ResourceCount;
	Binding.stageFlags = VK_SHADER_STAGE_ALL;

	VkDescriptorSetLayoutCreateInfo SetLayoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	SetLayoutInfo.bindingCount = 1;
	SetLayoutInfo.pBindings = &Binding;
	SetLayoutInfo.pNext = &BindingFlagsInfo;

	VkDescriptorSetVariableDescriptorCountLayoutSupport VariableDescriptorCountSupport = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_LAYOUT_SUPPORT };
	VkDescriptorSetLayoutSupport Support = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_SUPPORT };
	Support.pNext = &VariableDescriptorCountSupport;
	vkGetDescriptorSetLayoutSupport(Device, &SetLayoutInfo, &Support);

	vulkan(vkCreateDescriptorSetLayout(Device, &SetLayoutInfo, nullptr, &BindlessResourceSetLayout));

	VkDescriptorPoolSize DescriptorPoolSize;
	DescriptorPoolSize.descriptorCount = ResourceCount;
	DescriptorPoolSize.type = ResourceType;

	VkDescriptorPoolCreateInfo DescriptorPoolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	DescriptorPoolInfo.maxSets = 1;
	DescriptorPoolInfo.poolSizeCount = 1;
	DescriptorPoolInfo.pPoolSizes = &DescriptorPoolSize;

	vulkan(vkCreateDescriptorPool(Device, &DescriptorPoolInfo, nullptr, &BindlessResourceDescriptorPool));

	VkDescriptorSetVariableDescriptorCountAllocateInfo VariableDescriptorCountInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO };
	VariableDescriptorCountInfo.descriptorSetCount = 1;
	VariableDescriptorCountInfo.pDescriptorCounts = &ResourceCount;

	VkDescriptorSetAllocateInfo SetAllocateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	SetAllocateInfo.pNext = &VariableDescriptorCountInfo;
	SetAllocateInfo.descriptorPool = BindlessResourceDescriptorPool;
	SetAllocateInfo.descriptorSetCount = 1;
	SetAllocateInfo.pSetLayouts = &BindlessResourceSetLayout;

	vulkan(vkAllocateDescriptorSets(Device, &SetAllocateInfo, &BindlessResources));
}

VulkanBindlessResources::~VulkanBindlessResources()
{
	vkDestroyDescriptorSetLayout(Device, BindlessResourceSetLayout, nullptr);
	vkDestroyDescriptorPool(Device, BindlessResourceDescriptorPool, nullptr);
}

static VkImageLayout ChooseImageLayout(EFormat Format)
{
	if (drm::ImagePrivate::IsColor(Format))
	{
		return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}
	else if (drm::ImagePrivate::IsDepthStencil(Format))
	{
		return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	}
	else if (drm::ImagePrivate::IsDepth(Format))
	{
		return VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
	}
	else // Image.IsStencil()
	{
		return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
	}
}

VulkanTextureID VulkanBindlessResources::CreateTextureID(const VulkanImageView& ImageView, const VulkanSampler& Sampler)
{
	const VkDescriptorImageInfo ImageInfo = { Sampler.GetHandle(), ImageView.GetHandle(), ChooseImageLayout(ImageView.GetFormat()) };

	uint32 DstArrayElement;

	if (Available.empty())
	{
		DstArrayElement = CurrNumBindlessResources++;
	}
	else
	{
		DstArrayElement = Available.front();
		Available.pop_front();
	}

	VkWriteDescriptorSet WriteDescriptorSet = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	WriteDescriptorSet.dstSet = BindlessResources;
	WriteDescriptorSet.dstBinding = 0;
	WriteDescriptorSet.dstArrayElement = DstArrayElement;
	WriteDescriptorSet.descriptorCount = 1;
	WriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	WriteDescriptorSet.pImageInfo = &ImageInfo;
	
	vkUpdateDescriptorSets(Device, 1, &WriteDescriptorSet, 0, nullptr);

	return VulkanTextureID(DstArrayElement);
}

void VulkanBindlessResources::FreeResourceID(uint32 ResourceID)
{
	FreedThisFrame.push_back(ResourceID);
}

void VulkanBindlessResources::EndFrame()
{
	for (auto ID : FreedThisFrame)
	{
		Available.push_back(ID);
	}

	FreedThisFrame.clear();
}

static constexpr uint32 NULL_ID = -1;

VulkanBindlessResources* gBindlessTextures = nullptr;

VulkanTextureID::VulkanTextureID(uint32 ID)
	: ID(ID)
{
}

VulkanTextureID::VulkanTextureID(VulkanTextureID&& Other)
	: ID(std::exchange(Other.ID, NULL_ID))
{
}

VulkanTextureID& VulkanTextureID::operator=(VulkanTextureID&& Other)
{
	ID = std::exchange(Other.ID, NULL_ID);
	return *this;
}

VulkanTextureID::~VulkanTextureID()
{
	if (ID != NULL_ID)
	{
		gBindlessTextures->FreeResourceID(ID);
	}
}