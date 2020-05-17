#include "VulkanDescriptors.h"
#include "VulkanDevice.h"

std::pair<VkDescriptorSetLayout, VkDescriptorUpdateTemplate> VulkanCache::GetDescriptorSetLayout(
	const std::vector<VkDescriptorSetLayoutBinding>& Bindings,
	const std::vector<VkDescriptorUpdateTemplateEntry>& Entries
)
{
	const Crc Crc = Platform::CalculateCrc(Bindings.data(), Bindings.size() * sizeof(Bindings.front()));

	if (auto Iter = SetLayoutCache.find(Crc); Iter != SetLayoutCache.end())
	{
		return Iter->second;
	}

	VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	DescriptorSetLayoutInfo.bindingCount = static_cast<uint32>(Bindings.size());
	DescriptorSetLayoutInfo.pBindings = Bindings.data();

	VkDescriptorSetLayout DescriptorSetLayout;
	vulkan(vkCreateDescriptorSetLayout(Device, &DescriptorSetLayoutInfo, nullptr, &DescriptorSetLayout));

	VkDescriptorUpdateTemplateCreateInfo CreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_CREATE_INFO };
	CreateInfo.descriptorUpdateEntryCount = static_cast<uint32>(Entries.size());
	CreateInfo.pDescriptorUpdateEntries = Entries.data();
	CreateInfo.templateType = VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_DESCRIPTOR_SET;
	CreateInfo.descriptorSetLayout = DescriptorSetLayout;

	PFN_vkCreateDescriptorUpdateTemplateKHR p_vkCreateDescriptorUpdateTemplateKHR = 
		(PFN_vkCreateDescriptorUpdateTemplateKHR)vkGetInstanceProcAddr(Device.GetInstance(), "vkCreateDescriptorUpdateTemplateKHR");

	VkDescriptorUpdateTemplate DescriptorUpdateTemplate;
	p_vkCreateDescriptorUpdateTemplateKHR(Device, &CreateInfo, nullptr, &DescriptorUpdateTemplate);

	SetLayoutCache[Crc] = { DescriptorSetLayout, DescriptorUpdateTemplate };

	return SetLayoutCache[Crc];
}

void VulkanCache::UpdateDescriptorSetWithTemplate(VkDescriptorSet DescriptorSet, VkDescriptorUpdateTemplate DescriptorUpdateTemplate, const void* Data)
{
	p_vkUpdateDescriptorSetWithTemplateKHR(Device, DescriptorSet, DescriptorUpdateTemplate, Data);
}

VulkanDescriptorPool::VulkanDescriptorPool(VulkanDevice& Device, const VkDescriptorPoolCreateInfo& CreateInfo)
{
	vulkan(vkCreateDescriptorPool(Device, &CreateInfo, nullptr, &DescriptorPool));
	DescriptorSetsWaitingToBeFreed.reserve(VulkanDescriptorPoolManager::SetsPerPool);
}

VkDescriptorSet VulkanDescriptorPool::Allocate(VulkanDevice& Device, VkDescriptorSetLayout Layout)
{
	if (NumDescriptorSets < VulkanDescriptorPoolManager::SetsPerPool)
	{
		VkDescriptorSetAllocateInfo AllocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		AllocInfo.descriptorPool = DescriptorPool;
		AllocInfo.descriptorSetCount = 1;
		AllocInfo.pSetLayouts = &Layout;

		VkDescriptorSet DescriptorSet;
		vulkan(vkAllocateDescriptorSets(Device, &AllocInfo, &DescriptorSet));
		NumDescriptorSets++;
		return DescriptorSet;
	}
	else
	{
		return VK_NULL_HANDLE;
	}
}

void VulkanDescriptorPool::Free(VkDescriptorSet DescriptorSet)
{
	DescriptorSetsWaitingToBeFreed.push_back(DescriptorSet);
}

void VulkanDescriptorPool::EndFrame(VulkanDevice& Device)
{
	if (!DescriptorSetsWaitingToBeFreed.empty())
	{
		vkFreeDescriptorSets(Device, DescriptorPool, static_cast<uint32>(DescriptorSetsWaitingToBeFreed.size()), DescriptorSetsWaitingToBeFreed.data());
		NumDescriptorSets -= static_cast<uint32>(DescriptorSetsWaitingToBeFreed.size());
		DescriptorSetsWaitingToBeFreed.clear();
	}
}

VulkanDescriptorPoolManager::VulkanDescriptorPoolManager()
{
	for (VkDescriptorType DescriptorType = VK_DESCRIPTOR_TYPE_BEGIN_RANGE; DescriptorType < VK_DESCRIPTOR_TYPE_RANGE_SIZE;)
	{
		VkDescriptorPoolSize& PoolSize = PoolSizes[DescriptorType];
		PoolSize.descriptorCount = SetsPerPool;
		PoolSize.type = DescriptorType;
		DescriptorType = VkDescriptorType(DescriptorType + 1);
	}

	PoolInfo.pPoolSizes = PoolSizes.data();
	PoolInfo.poolSizeCount = static_cast<uint32>(PoolSizes.size());
	PoolInfo.maxSets = SetsPerPool;
	PoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
}

VulkanDescriptorSet VulkanDescriptorPoolManager::Allocate(VulkanDevice& Device, VkDescriptorSetLayout Layout)
{
	for (auto& DescriptorPool : DescriptorPools)
	{
		if (VkDescriptorSet DescriptorSet = DescriptorPool->Allocate(Device, Layout); DescriptorSet != VK_NULL_HANDLE)
		{
			return VulkanDescriptorSet(*DescriptorPool, Layout, DescriptorSet);
		}
	}

	DescriptorPools.push_back(std::make_unique<VulkanDescriptorPool>(Device, PoolInfo));
	VulkanDescriptorPool& DescriptorPool = *DescriptorPools.back();
	return VulkanDescriptorSet(DescriptorPool, Layout, DescriptorPool.Allocate(Device, Layout));
}

void VulkanDescriptorPoolManager::EndFrame(VulkanDevice& Device)
{
	for (auto& DescriptorPool : DescriptorPools)
	{
		DescriptorPool->EndFrame(Device);
	}
}

VulkanDescriptorSet::VulkanDescriptorSet(
	VulkanDescriptorPool& DescriptorPool,
	VkDescriptorSetLayout Layout,
	VkDescriptorSet DescriptorSet)
	: DescriptorPool(&DescriptorPool)
	, Layout(Layout)
	, DescriptorSet(DescriptorSet)
{
}

VulkanDescriptorSet::VulkanDescriptorSet(VulkanDescriptorSet&& Other)
	: DescriptorPool(Other.DescriptorPool)
	, Layout(Other.Layout)
	, DescriptorSet(std::exchange(Other.DescriptorSet, nullptr))
{
}

VulkanDescriptorSet& VulkanDescriptorSet::operator=(VulkanDescriptorSet&& Other)
{
	DescriptorPool = Other.DescriptorPool;
	Layout = std::exchange(Other.Layout, nullptr);
	DescriptorSet = std::exchange(Other.DescriptorSet, nullptr);
	return *this;
}

VulkanDescriptorSet::~VulkanDescriptorSet()
{
	if (DescriptorSet != nullptr)
	{
		DescriptorPool->Free(DescriptorSet);
	}
}

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(
	VulkanDevice& Device,
	std::size_t NumBindings,
	const DescriptorBinding* Bindings)
{
	static const VkDescriptorType DescriptorTypes[] =
	{
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	};

	std::vector<VkDescriptorUpdateTemplateEntry> DescriptorUpdateTemplateEntries;
	DescriptorUpdateTemplateEntries.reserve(NumBindings);

	std::vector<VkDescriptorSetLayoutBinding> DescriptorSetLayoutBindings;
	DescriptorSetLayoutBindings.reserve(NumBindings);
	
	uint32 StructSize = 0;

	for (std::size_t BindingIndex = 0; BindingIndex < NumBindings; BindingIndex++)
	{
		const DescriptorBinding& Binding = Bindings[BindingIndex];
		VkDescriptorUpdateTemplateEntry DescriptorUpdateTemplateEntry = {};
		DescriptorUpdateTemplateEntry.dstBinding = Binding.Binding;
		DescriptorUpdateTemplateEntry.descriptorCount = Binding.DescriptorCount;
		DescriptorUpdateTemplateEntry.descriptorType = DescriptorTypes[static_cast<uint32>(Binding.DescriptorType)];
		DescriptorUpdateTemplateEntry.offset = StructSize;

		StructSize += (DescriptorUpdateTemplateEntry.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER || DescriptorUpdateTemplateEntry.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ?
			sizeof(VkDescriptorBufferInfo) : sizeof(VkDescriptorImageInfo));

		DescriptorUpdateTemplateEntries.push_back(DescriptorUpdateTemplateEntry);

		VkDescriptorSetLayoutBinding DescriptorSetLayoutBinding;
		DescriptorSetLayoutBinding.binding = Binding.Binding;
		DescriptorSetLayoutBinding.descriptorCount = Binding.DescriptorCount;
		DescriptorSetLayoutBinding.descriptorType = DescriptorTypes[static_cast<uint32>(Binding.DescriptorType)];
		DescriptorSetLayoutBinding.pImmutableSamplers = nullptr;
		DescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_ALL;

		DescriptorSetLayoutBindings.push_back(DescriptorSetLayoutBinding);
	}

	std::tie(DescriptorSetLayout, DescriptorUpdateTemplate) = Device.GetCache().GetDescriptorSetLayout(DescriptorSetLayoutBindings, DescriptorUpdateTemplateEntries);
}

VulkanDescriptorSet VulkanDescriptorSetLayout::CreateDescriptorSet(drm::Device& Device)
{
	return static_cast<VulkanDevice&>(Device).GetDescriptorPoolManager().Allocate(static_cast<VulkanDevice&>(Device), DescriptorSetLayout);
}

void VulkanDescriptorSetLayout::UpdateDescriptorSet(drm::Device& Device, const VulkanDescriptorSet& DescriptorSet, void* Struct)
{
	static_cast<VulkanDevice&>(Device).GetCache().UpdateDescriptorSetWithTemplate(DescriptorSet.GetHandle(), DescriptorUpdateTemplate, Struct);
}