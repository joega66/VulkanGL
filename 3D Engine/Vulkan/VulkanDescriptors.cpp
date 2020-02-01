#include "VulkanDescriptors.h"
#include "VulkanDevice.h"

VulkanDescriptorPool::VulkanDescriptorPool(VulkanDevice& Device)
	: Device(Device)
{
	PendingFreeDescriptorSets.resize(MaxDescriptorSetCount);

	std::array<VkDescriptorPoolSize, VK_DESCRIPTOR_TYPE_RANGE_SIZE> PoolSizes;

	for (VkDescriptorType DescriptorType = VK_DESCRIPTOR_TYPE_BEGIN_RANGE; DescriptorType < VK_DESCRIPTOR_TYPE_RANGE_SIZE;)
	{
		VkDescriptorPoolSize& PoolSize = PoolSizes[DescriptorType];
		PoolSize.descriptorCount = MaxDescriptorSetCount;
		PoolSize.type = DescriptorType;
		DescriptorType = VkDescriptorType(DescriptorType + 1);
	}

	VkDescriptorPoolCreateInfo PoolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	PoolInfo.pPoolSizes = PoolSizes.data();
	PoolInfo.poolSizeCount = PoolSizes.size();
	PoolInfo.maxSets = MaxDescriptorSetCount;
	PoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

	vulkan(vkCreateDescriptorPool(Device, &PoolInfo, nullptr, &DescriptorPool));
}

VkDescriptorSet VulkanDescriptorPool::Spawn(const VkDescriptorSetLayout& DescriptorSetLayout)
{
	check(DescriptorSetCount < MaxDescriptorSetCount, "Descriptor pool is full.");

	VkDescriptorSetAllocateInfo DescriptorSetInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	DescriptorSetInfo.descriptorPool = DescriptorPool;
	DescriptorSetInfo.descriptorSetCount = 1;
	DescriptorSetInfo.pSetLayouts = &DescriptorSetLayout;

	VkDescriptorSet DescriptorSet;
	vulkan(vkAllocateDescriptorSets(Device, &DescriptorSetInfo, &DescriptorSet));
	DescriptorSetCount++;

	return DescriptorSet;
}

void VulkanDescriptorPool::Free(VkDescriptorSet DescriptorSet)
{
	PendingFreeDescriptorSets[PendingFreeCount++] = DescriptorSet;
}

void VulkanDescriptorPool::EndFrame()
{
	vkFreeDescriptorSets(Device, DescriptorPool, PendingFreeCount, PendingFreeDescriptorSets.data());

	DescriptorSetCount -= PendingFreeCount;

	PendingFreeCount = 0;
}

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

VulkanDescriptorSet::VulkanDescriptorSet(VulkanDevice& Device, VulkanDescriptorPool& DescriptorPool)
	: Device(Device), DescriptorPool(DescriptorPool)
{
}

VulkanDescriptorSet::~VulkanDescriptorSet()
{
	std::for_each(PendingWrites.begin(), PendingWrites.end(),
		[&](auto& Write)
	{
		if (Write.pBufferInfo)
		{
			delete Write.pBufferInfo;
		}
		else
		{
			delete Write.pImageInfo;
		}
	});

	DescriptorPool.Free(DescriptorSet);
}

void VulkanDescriptorSet::Write(drm::ImageRef Image, const SamplerState& Sampler, uint32 Binding)
{
	const VkDescriptorType DescriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	const VkSampler VulkanSampler = Device.GetCache().GetSampler(Sampler);
	const VulkanImageRef& VulkanImage = ResourceCast(Image);
	const VkImageLayout ImageLayout = Image->IsDepth() ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	const VkDescriptorImageInfo* ImageInfo = new VkDescriptorImageInfo{ VulkanSampler, VulkanImage->ImageView, ImageLayout };

	MaybeAddBinding(Binding, DescriptorType);

	VkWriteDescriptorSet Write = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	Write.dstBinding = Binding;
	Write.dstArrayElement = 0;
	Write.descriptorType = DescriptorType;
	Write.descriptorCount = 1;
	Write.pImageInfo = ImageInfo;

	PendingWrites.push_back(Write);
}

void VulkanDescriptorSet::Write(drm::ImageRef Image, uint32 Binding)
{
	const VkDescriptorType DescriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	const VulkanImageRef& VulkanImage = ResourceCast(Image);
	const VkDescriptorImageInfo* ImageInfo = new VkDescriptorImageInfo{ VK_NULL_HANDLE, VulkanImage->ImageView, VK_IMAGE_LAYOUT_GENERAL };

	MaybeAddBinding(Binding, DescriptorType);

	VkWriteDescriptorSet Write = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	Write.dstBinding = Binding;
	Write.dstArrayElement = 0;
	Write.descriptorType = DescriptorType;
	Write.descriptorCount = 1;
	Write.pImageInfo = ImageInfo;

	PendingWrites.push_back(Write);
}

void VulkanDescriptorSet::Write(drm::BufferRef Buffer, uint32 Binding)
{
	const VkDescriptorType DescriptorType = Any(Buffer->Usage & EBufferUsage::Uniform) ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	const VulkanBufferRef& VulkanBuffer = ResourceCast(Buffer);
	const VkDescriptorBufferInfo* BufferInfo = new VkDescriptorBufferInfo{ VulkanBuffer->GetVulkanHandle(), VulkanBuffer->GetOffset(), VulkanBuffer->GetSize() };

	MaybeAddBinding(Binding, DescriptorType);

	VkWriteDescriptorSet Write = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	Write.dstBinding = Binding;
	Write.dstArrayElement = 0;
	Write.descriptorType = DescriptorType;
	Write.descriptorCount = 1;
	Write.pBufferInfo = BufferInfo;

	PendingWrites.push_back(Write);
}

void VulkanDescriptorSet::Update()
{
	std::call_once(SpawnDescriptorSetOnceFlag, [&] ()
	{
		DescriptorSetLayout = Device.GetCache().GetDescriptorSetLayout(VulkanBindings);
		DescriptorSet = DescriptorPool.Spawn(DescriptorSetLayout);
		VulkanBindings.clear();
	});

	std::for_each(PendingWrites.begin(), PendingWrites.end(), 
		[&] (auto& Write)
	{
		Write.dstSet = DescriptorSet;
	});

	vkUpdateDescriptorSets(Device, PendingWrites.size(), PendingWrites.data(), 0, nullptr);

	std::for_each(PendingWrites.begin(), PendingWrites.end(),
		[&] (auto& Write)
	{
		if (Write.pBufferInfo)
		{
			delete Write.pBufferInfo;
		}
		else
		{
			delete Write.pImageInfo;
		}
	});
	
	PendingWrites.clear();
}

void VulkanDescriptorSet::MaybeAddBinding(uint32 Binding, VkDescriptorType DescriptorType)
{
	if (DescriptorSetLayout == VK_NULL_HANDLE)
	{
		check(std::none_of(VulkanBindings.begin(), VulkanBindings.end(),
			[&] (const auto& VulkanBinding)
		{
			return VulkanBinding.binding == Binding;
		}), "This binding has already been seen before.");

		VkDescriptorSetLayoutBinding VulkanBinding;
		VulkanBinding.binding = Binding;
		VulkanBinding.descriptorCount = 1;
		VulkanBinding.descriptorType = DescriptorType;
		VulkanBinding.pImmutableSamplers = nullptr;
		VulkanBinding.stageFlags = VK_SHADER_STAGE_ALL;

		VulkanBindings.push_back(VulkanBinding);
	}
}