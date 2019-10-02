#include "VulkanDescriptors.h"
#include "VulkanDevice.h"
#include "VulkanMemory.h"

VulkanDescriptorPool::VulkanDescriptorPool(VulkanDevice& Device)
	: Device(Device)
	, MaxDescriptorSetCount(4096)
	, DescriptorSetCount(0)
{
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

void VulkanDescriptorPool::Reset()
{
	vulkan(vkResetDescriptorPool(Device, DescriptorPool, 0));
	DescriptorSetCount = 0;
}

VkDescriptorSetLayout VulkanDevice::GetDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& Bindings)
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

static CAST(drm::Image, VulkanImage);
static CAST(drm::UniformBuffer, VulkanUniformBuffer);
static CAST(drm::StorageBuffer, VulkanStorageBuffer);

VulkanDescriptorSet::VulkanDescriptorSet(VulkanDevice& Device, VulkanDescriptorPool& DescriptorPool, VulkanAllocator& Allocator)
	: Device(Device), DescriptorPool(DescriptorPool), Allocator(Allocator)
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
}

void VulkanDescriptorSet::Write(drm::ImageRef Image, const SamplerState& Sampler, const ShaderBinding& Binding)
{
	const VkDescriptorType DescriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	const VkSampler VulkanSampler = Device.GetSampler(Sampler);
	VulkanImageRef VulkanImage = ResourceCast(Image);
	const VkDescriptorImageInfo* ImageInfo = new VkDescriptorImageInfo{ VulkanSampler, VulkanImage->ImageView, VulkanImage->GetVulkanLayout() };

	MaybeAddBinding(Binding, DescriptorType);

	VkWriteDescriptorSet Write = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	Write.dstBinding = Binding.GetBinding();
	Write.dstArrayElement = 0;
	Write.descriptorType = DescriptorType;
	Write.descriptorCount = 1;
	Write.pImageInfo = ImageInfo;

	PendingWrites.push_back(Write);
}

void VulkanDescriptorSet::Write(drm::UniformBufferRef UniformBuffer, const ShaderBinding& Binding)
{
	const VkDescriptorType DescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	VulkanUniformBufferRef VulkanUniformBuffer = ResourceCast(UniformBuffer);
	const SharedVulkanBufferRef& SharedBuffer = VulkanUniformBuffer->Buffer;
	const VkDescriptorBufferInfo* BufferInfo = new VkDescriptorBufferInfo{ SharedBuffer->GetVulkanHandle(), SharedBuffer->Offset, SharedBuffer->Size };

	MaybeAddBinding(Binding, DescriptorType);

	if (VulkanUniformBuffer->bDirty)
	{
		Allocator.UploadBufferData(*VulkanUniformBuffer->Buffer, VulkanUniformBuffer->GetData());
		VulkanUniformBuffer->bDirty = false;
	}

	VkWriteDescriptorSet Write = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	Write.dstBinding = Binding.GetBinding();
	Write.dstArrayElement = 0;
	Write.descriptorType = DescriptorType;
	Write.descriptorCount = 1;
	Write.pBufferInfo = BufferInfo;

	PendingWrites.push_back(Write);
}

void VulkanDescriptorSet::Write(drm::StorageBufferRef StorageBuffer, const ShaderBinding& Binding)
{
	const VkDescriptorType DescriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	VulkanStorageBufferRef VulkanStorageBuffer = ResourceCast(StorageBuffer);
	const SharedVulkanBufferRef& SharedBuffer = VulkanStorageBuffer->Buffer;
	const VkDescriptorBufferInfo* BufferInfo = new VkDescriptorBufferInfo{ SharedBuffer->GetVulkanHandle(), SharedBuffer->Offset, SharedBuffer->Size };

	MaybeAddBinding(Binding, DescriptorType);

	VkWriteDescriptorSet Write = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	Write.dstBinding = Binding.GetBinding();
	Write.dstArrayElement = 0;
	Write.descriptorType = DescriptorType;
	Write.descriptorCount = 1;
	Write.pBufferInfo = BufferInfo;

	PendingWrites.push_back(Write);
}

void VulkanDescriptorSet::Update()
{
	if (DescriptorSet == VK_NULL_HANDLE)
	{
		DescriptorSetLayout = Device.GetDescriptorSetLayout(VulkanBindings);
		DescriptorSet = DescriptorPool.Spawn(DescriptorSetLayout);
	}

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

void VulkanDescriptorSet::MaybeAddBinding(const ShaderBinding& Binding, VkDescriptorType DescriptorType)
{
	const bool NoneOf = std::none_of(VulkanBindings.begin(), VulkanBindings.end(),
		[&] (const auto& VulkanBinding)
	{
		return VulkanBinding.binding == Binding.GetBinding();
	});

	if (NoneOf)
	{
		// This is a binding we haven't seen yet. Add it to the list of bindings for this descriptor set.
		VkDescriptorSetLayoutBinding VulkanBinding;
		VulkanBinding.binding = Binding.GetBinding();
		VulkanBinding.descriptorCount = 1;
		VulkanBinding.descriptorType = DescriptorType;
		VulkanBinding.pImmutableSamplers = nullptr;
		VulkanBinding.stageFlags = VK_SHADER_STAGE_ALL;

		VulkanBindings.push_back(VulkanBinding);
	}
}
