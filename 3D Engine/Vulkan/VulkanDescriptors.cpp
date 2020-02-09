#include "VulkanDescriptors.h"
#include "VulkanDevice.h"

VkDescriptorSetLayout VulkanCache::GetDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& Bindings)
{
	for (const auto& [CachedBindings, CachedDescriptorSetLayout] : DescriptorSetLayoutCache)
	{
		if (std::equal(Bindings.begin(), Bindings.end(), CachedBindings.begin(), CachedBindings.end(), 
			[](const VkDescriptorSetLayoutBinding& LHS, const VkDescriptorSetLayoutBinding& RHS)
			{
				return LHS.binding == RHS.binding &&
					LHS.descriptorCount == RHS.descriptorCount &&
					LHS.descriptorType == RHS.descriptorType &&
					LHS.stageFlags == RHS.stageFlags;
			}))
		{
			return CachedDescriptorSetLayout;
		}
	}

	VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	DescriptorSetLayoutInfo.bindingCount = static_cast<uint32>(Bindings.size());
	DescriptorSetLayoutInfo.pBindings = Bindings.data();

	VkDescriptorSetLayout DescriptorSetLayout;
	vulkan(vkCreateDescriptorSetLayout(Device, &DescriptorSetLayoutInfo, nullptr, &DescriptorSetLayout));

	DescriptorSetLayoutCache.push_back({ Bindings, DescriptorSetLayout });

	return DescriptorSetLayout;
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

void VulkanDescriptorPool::DeferredFree(VulkanDevice& Device)
{
	if (!DescriptorSetsWaitingToBeFreed.empty())
	{
		vkFreeDescriptorSets(Device, DescriptorPool, DescriptorSetsWaitingToBeFreed.size(), DescriptorSetsWaitingToBeFreed.data());
		NumDescriptorSets -= DescriptorSetsWaitingToBeFreed.size();
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
	PoolInfo.poolSizeCount = PoolSizes.size();
	PoolInfo.maxSets = SetsPerPool;
	PoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
}

VulkanDescriptorSetRef VulkanDescriptorPoolManager::Allocate(VulkanDevice& Device, VkDescriptorSetLayout Layout)
{
	for (auto& DescriptorPool : DescriptorPools)
	{
		if (VkDescriptorSet DescriptorSet = DescriptorPool->Allocate(Device, Layout); DescriptorSet != VK_NULL_HANDLE)
		{
			return MakeRef<VulkanDescriptorSet>(*DescriptorPool, Layout, DescriptorSet);
		}
	}

	DescriptorPools.push_back(std::make_unique<VulkanDescriptorPool>(Device, PoolInfo));
	VulkanDescriptorPool& DescriptorPool = *DescriptorPools.back();
	return MakeRef<VulkanDescriptorSet>(DescriptorPool, Layout, DescriptorPool.Allocate(Device, Layout));
}

void VulkanDescriptorPoolManager::DeferredFree(VulkanDevice& Device)
{
	for (auto& DescriptorPool : DescriptorPools)
	{
		DescriptorPool->DeferredFree(Device);
	}
}

VulkanDescriptorSet::VulkanDescriptorSet(
	VulkanDescriptorPool& DescriptorPool,
	VkDescriptorSetLayout Layout,
	VkDescriptorSet DescriptorSet)
	: DescriptorPool(DescriptorPool)
	, Layout(Layout)
	, DescriptorSet(DescriptorSet)
{
}

VulkanDescriptorSet::~VulkanDescriptorSet()
{
	DescriptorPool.Free(DescriptorSet);
}

VulkanDescriptorTemplate::VulkanDescriptorTemplate(
	VulkanDevice& Device, 
	uint32 NumEntries,
	const DescriptorTemplateEntry* Entries)
	: Device(Device)
{
	static const VkDescriptorType DescriptorTypes[] =
	{
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	};

	DescriptorUpdateTemplateEntries.reserve(NumEntries);

	std::vector<VkDescriptorSetLayoutBinding> DescriptorSetLayoutBindings;
	DescriptorSetLayoutBindings.reserve(NumEntries);
	
	uint32 StructSize = 0;

	for (uint32 EntryIndex = 0; EntryIndex < NumEntries; EntryIndex++)
	{
		const DescriptorTemplateEntry& Entry = Entries[EntryIndex];
		VkDescriptorUpdateTemplateEntry DescriptorUpdateTemplateEntry = {};
		DescriptorUpdateTemplateEntry.dstBinding = Entry.Binding;
		DescriptorUpdateTemplateEntry.descriptorCount = Entry.DescriptorCount;
		DescriptorUpdateTemplateEntry.descriptorType = DescriptorTypes[Entry.DescriptorType];
		DescriptorUpdateTemplateEntry.offset = StructSize;

		StructSize += (DescriptorUpdateTemplateEntry.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER || DescriptorUpdateTemplateEntry.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ?
			sizeof(VkDescriptorBufferInfo) : sizeof(VkDescriptorImageInfo));

		DescriptorUpdateTemplateEntries.push_back(DescriptorUpdateTemplateEntry);

		VkDescriptorSetLayoutBinding DescriptorSetLayoutBinding;
		DescriptorSetLayoutBinding.binding = Entry.Binding;
		DescriptorSetLayoutBinding.descriptorCount = Entry.DescriptorCount;
		DescriptorSetLayoutBinding.descriptorType = DescriptorTypes[Entry.DescriptorType];
		DescriptorSetLayoutBinding.pImmutableSamplers = nullptr;
		DescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_ALL;

		DescriptorSetLayoutBindings.push_back(DescriptorSetLayoutBinding);
	}

	DescriptorSetLayout = Device.GetCache().GetDescriptorSetLayout(DescriptorSetLayoutBindings);
	
	VkDescriptorUpdateTemplateCreateInfo CreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_CREATE_INFO };
	CreateInfo.descriptorUpdateEntryCount = DescriptorUpdateTemplateEntries.size();
	CreateInfo.pDescriptorUpdateEntries = DescriptorUpdateTemplateEntries.data();
	CreateInfo.templateType = VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_DESCRIPTOR_SET;
	CreateInfo.descriptorSetLayout = DescriptorSetLayout;

	p_vkCreateDescriptorUpdateTemplateKHR = (PFN_vkCreateDescriptorUpdateTemplateKHR)vkGetInstanceProcAddr(Device.GetInstance(), "vkCreateDescriptorUpdateTemplateKHR");
	p_vkCreateDescriptorUpdateTemplateKHR(Device, &CreateInfo, nullptr, &DescriptorTemplate);
	p_vkUpdateDescriptorSetWithTemplateKHR = (PFN_vkUpdateDescriptorSetWithTemplateKHR)vkGetInstanceProcAddr(Device.GetInstance(), "vkUpdateDescriptorSetWithTemplateKHR");
	p_vkDestroyDescriptorUpdateTemplateKHR = (PFN_vkDestroyDescriptorUpdateTemplateKHR)vkGetInstanceProcAddr(Device.GetInstance(), "vkDestroyDescriptorUpdateTemplateKHR");

	Data = new uint8[StructSize];
}

drm::DescriptorSetRef VulkanDescriptorTemplate::CreateDescriptorSet()
{
	return Device.GetDescriptorPoolManager().Allocate(Device, DescriptorSetLayout);
}

void VulkanDescriptorTemplate::UpdateDescriptorSet(drm::DescriptorSetRef DescriptorSet, void* Struct)
{
	uint32 DataOffset = 0;
	uint32 StructOffset = 0;

	for (auto& DescriptorUpdateTemplateEntry : DescriptorUpdateTemplateEntries)
	{
		if (DescriptorUpdateTemplateEntry.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
			DescriptorUpdateTemplateEntry.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		{
			// Get the buffer from the struct ptr.
			drm::BufferRef* Buffer = reinterpret_cast<drm::BufferRef*>(static_cast<uint8*>(Struct) + StructOffset);
			VulkanBufferRef& VulkanBuffer = *reinterpret_cast<VulkanBufferRef*>(Buffer);

			// Increment the offset into the struct.
			StructOffset += sizeof(drm::BufferRef);

			VkDescriptorBufferInfo* BufferInfoPtr = reinterpret_cast<VkDescriptorBufferInfo*>(static_cast<uint8*>(Data) + DataOffset);
			*BufferInfoPtr = { VulkanBuffer->GetVulkanHandle(), VulkanBuffer->GetOffset(), VulkanBuffer->GetSize() };

			// Increment the offset into the data ptr.
			DataOffset += sizeof(VkDescriptorBufferInfo);
		}
		else
		{
			// Get the image from the struct ptr.
			drm::ImageRef* Image = reinterpret_cast<drm::ImageRef*>(static_cast<uint8*>(Struct) + StructOffset);
			VulkanImageRef& VulkanImage = *reinterpret_cast<VulkanImageRef*>(Image);

			// Increment the offset into the struct;
			StructOffset += sizeof(drm::ImageRef);

			// Find what the image layout should be.
			const VkImageLayout ImageLayout = 
				DescriptorUpdateTemplateEntry.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ?
				(VulkanImage->IsDepth() ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) 
				: VK_IMAGE_LAYOUT_GENERAL;

			// Get the image info from the data ptr.
			VkDescriptorImageInfo* ImageInfoPtr = reinterpret_cast<VkDescriptorImageInfo*>(static_cast<uint8*>(Data) + DataOffset);
			*ImageInfoPtr = VkDescriptorImageInfo{ VK_NULL_HANDLE, VulkanImage->ImageView, ImageLayout };

			// Increment the offset into the data ptr.
			DataOffset += sizeof(VkDescriptorImageInfo);

			if (DescriptorUpdateTemplateEntry.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			{
				// Get the sampler from the struct and increment the offset into the struct.
				const SamplerState& SamplerDesc = *reinterpret_cast<const SamplerState*>(static_cast<uint8*>(Struct) + StructOffset);
				StructOffset += sizeof(SamplerState);
				const VkSampler VulkanSampler = Device.GetCache().GetSampler(SamplerDesc);
				ImageInfoPtr->sampler = VulkanSampler;
			}
		}
	}

	VulkanDescriptorSetRef VulkanDescriptorSet = ResourceCast(DescriptorSet);
	
	p_vkUpdateDescriptorSetWithTemplateKHR(Device, VulkanDescriptorSet->GetVulkanHandle(), DescriptorTemplate, Data);
}

VulkanDescriptorTemplate::~VulkanDescriptorTemplate()
{
	p_vkDestroyDescriptorUpdateTemplateKHR(Device, DescriptorTemplate, nullptr);
	delete[] Data;
}