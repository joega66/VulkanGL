#include "VulkanDescriptors.h"
#include "VulkanDevice.h"

std::pair<VkDescriptorSetLayout, VkDescriptorUpdateTemplate> VulkanCache::GetDescriptorSetLayout(
	const std::vector<VkDescriptorSetLayoutBinding>& bindings,
	const std::vector<VkDescriptorUpdateTemplateEntry>& entries
)
{
	const Crc crc = Platform::CalculateCrc(bindings.data(), bindings.size() * sizeof(bindings.front()));

	if (auto iter = SetLayoutCache.find(crc); iter != SetLayoutCache.end())
	{
		return iter->second;
	}

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	descriptorSetLayoutInfo.bindingCount = static_cast<uint32>(bindings.size());
	descriptorSetLayoutInfo.pBindings = bindings.data();

	VkDescriptorSetLayout descriptorSetLayout;
	vulkan(vkCreateDescriptorSetLayout(Device, &descriptorSetLayoutInfo, nullptr, &descriptorSetLayout));

	VkDescriptorUpdateTemplateCreateInfo descriptorUpdateTemplateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_CREATE_INFO };
	descriptorUpdateTemplateInfo.descriptorUpdateEntryCount = static_cast<uint32>(entries.size());
	descriptorUpdateTemplateInfo.pDescriptorUpdateEntries = entries.data();
	descriptorUpdateTemplateInfo.templateType = VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_DESCRIPTOR_SET;
	descriptorUpdateTemplateInfo.descriptorSetLayout = descriptorSetLayout;

	PFN_vkCreateDescriptorUpdateTemplateKHR p_vkCreateDescriptorUpdateTemplateKHR = 
		(PFN_vkCreateDescriptorUpdateTemplateKHR)vkGetInstanceProcAddr(Device.GetInstance(), "vkCreateDescriptorUpdateTemplateKHR");

	VkDescriptorUpdateTemplate descriptorUpdateTemplate;
	p_vkCreateDescriptorUpdateTemplateKHR(Device, &descriptorUpdateTemplateInfo, nullptr, &descriptorUpdateTemplate);

	SetLayoutCache[crc] = { descriptorSetLayout, descriptorUpdateTemplate };

	return SetLayoutCache[crc];
}

void VulkanCache::UpdateDescriptorSetWithTemplate(VkDescriptorSet descriptorSet, VkDescriptorUpdateTemplate descriptorUpdateTemplate, const void* data)
{
	p_vkUpdateDescriptorSetWithTemplateKHR(Device, descriptorSet, descriptorUpdateTemplate, data);
}

namespace gpu
{
	DescriptorPool::DescriptorPool(VulkanDevice& device, const VkDescriptorPoolCreateInfo& descriptorPoolInfo)
	{
		vulkan(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &_DescriptorPool));
		_DescriptorSetsWaitingToBeFreed.reserve(DescriptorPoolManager::SETS_PER_POOL);
	}

	VkDescriptorSet DescriptorPool::Allocate(VulkanDevice& device, VkDescriptorSetLayout layout)
	{
		if (_NumDescriptorSets < DescriptorPoolManager::SETS_PER_POOL)
		{
			const VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = 
			{ 
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
				.descriptorPool = _DescriptorPool,
				.descriptorSetCount = 1,
				.pSetLayouts = &layout,
			};

			VkDescriptorSet descriptorSet;
			vulkan(vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, &descriptorSet));

			_NumDescriptorSets++;

			return descriptorSet;
		}
		else
		{
			return VK_NULL_HANDLE;
		}
	}

	void DescriptorPool::Free(VkDescriptorSet descriptorSet)
	{
		_DescriptorSetsWaitingToBeFreed.push_back(descriptorSet);
	}

	void DescriptorPool::EndFrame(VulkanDevice& device)
	{
		if (!_DescriptorSetsWaitingToBeFreed.empty())
		{
			vkFreeDescriptorSets(device, _DescriptorPool, static_cast<uint32>(_DescriptorSetsWaitingToBeFreed.size()), _DescriptorSetsWaitingToBeFreed.data());
			_NumDescriptorSets -= static_cast<uint32>(_DescriptorSetsWaitingToBeFreed.size());
			_DescriptorSetsWaitingToBeFreed.clear();
		}
	}

	DescriptorPoolManager::DescriptorPoolManager()
	{
		for (VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_BEGIN_RANGE; descriptorType < VK_DESCRIPTOR_TYPE_RANGE_SIZE;)
		{
			VkDescriptorPoolSize& poolSize = _PoolSizes[descriptorType];
			poolSize.descriptorCount = SETS_PER_POOL;
			poolSize.type = descriptorType;
			descriptorType = VkDescriptorType(descriptorType + 1);
		}

		_PoolInfo.pPoolSizes = _PoolSizes.data();
		_PoolInfo.poolSizeCount = static_cast<uint32>(_PoolSizes.size());
		_PoolInfo.maxSets = SETS_PER_POOL;
		_PoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	}

	DescriptorSet DescriptorPoolManager::Allocate(VulkanDevice& device, VkDescriptorSetLayout layout)
	{
		for (auto& descriptorPool : _DescriptorPools)
		{
			if (VkDescriptorSet descriptorSet = descriptorPool->Allocate(device, layout); descriptorSet != VK_NULL_HANDLE)
			{
				return DescriptorSet(*descriptorPool, descriptorSet);
			}
		}

		_DescriptorPools.push_back(std::make_unique<DescriptorPool>(device, _PoolInfo));
		DescriptorPool& descriptorPool = *_DescriptorPools.back();
		return DescriptorSet(descriptorPool, descriptorPool.Allocate(device, layout));
	}

	void DescriptorPoolManager::EndFrame(VulkanDevice& device)
	{
		for (auto& descriptorPool : _DescriptorPools)
		{
			descriptorPool->EndFrame(device);
		}
	}

	DescriptorSet::DescriptorSet(
		class DescriptorPool& descriptorPool,
		VkDescriptorSet descriptorSet)
		: _DescriptorPool(&descriptorPool)
		, _DescriptorSet(descriptorSet)
	{
	}

	DescriptorSet::DescriptorSet(DescriptorSet&& other)
		: _DescriptorPool(other._DescriptorPool)
		, _DescriptorSet(std::exchange(other._DescriptorSet, nullptr))
	{
	}

	DescriptorSet& DescriptorSet::operator=(DescriptorSet&& other)
	{
		_DescriptorPool = other._DescriptorPool;
		_DescriptorSet = std::exchange(other._DescriptorSet, nullptr);
		return *this;
	}

	DescriptorSet::~DescriptorSet()
	{
		if (_DescriptorSet != nullptr)
		{
			_DescriptorPool->Free(_DescriptorSet);
		}
	}

	DescriptorSetLayout::DescriptorSetLayout(
		VulkanDevice& device,
		std::size_t numBindings,
		const DescriptorBinding* bindings)
	{
		static const VkDescriptorType descriptorTypes[] =
		{
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		};

		std::vector<VkDescriptorUpdateTemplateEntry> descriptorUpdateTemplateEntries;
		descriptorUpdateTemplateEntries.reserve(numBindings);

		std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings;
		descriptorSetLayoutBindings.reserve(numBindings);

		uint32 structSize = 0;

		for (std::size_t bindingIndex = 0; bindingIndex < numBindings; bindingIndex++)
		{
			const DescriptorBinding& binding = bindings[bindingIndex];
			VkDescriptorUpdateTemplateEntry descriptorUpdateTemplateEntry = {};
			descriptorUpdateTemplateEntry.dstBinding = binding.binding;
			descriptorUpdateTemplateEntry.descriptorCount = binding.descriptorCount;
			descriptorUpdateTemplateEntry.descriptorType = descriptorTypes[static_cast<uint32>(binding.descriptorType)];
			descriptorUpdateTemplateEntry.offset = structSize;

			structSize += (descriptorUpdateTemplateEntry.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER || descriptorUpdateTemplateEntry.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ?
				sizeof(VkDescriptorBufferInfo) : sizeof(VkDescriptorImageInfo));

			descriptorUpdateTemplateEntries.push_back(descriptorUpdateTemplateEntry);

			VkDescriptorSetLayoutBinding descriptorSetLayoutBinding;
			descriptorSetLayoutBinding.binding = binding.binding;
			descriptorSetLayoutBinding.descriptorCount = binding.descriptorCount;
			descriptorSetLayoutBinding.descriptorType = descriptorTypes[static_cast<uint32>(binding.descriptorType)];
			descriptorSetLayoutBinding.pImmutableSamplers = nullptr;
			descriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_ALL;

			descriptorSetLayoutBindings.push_back(descriptorSetLayoutBinding);
		}

		std::tie(_DescriptorSetLayout, _DescriptorUpdateTemplate) = device.GetCache().GetDescriptorSetLayout(descriptorSetLayoutBindings, descriptorUpdateTemplateEntries);
	}

	DescriptorSet DescriptorSetLayout::CreateDescriptorSet(gpu::Device& device)
	{
		return static_cast<VulkanDevice&>(device).GetDescriptorPoolManager().Allocate(static_cast<VulkanDevice&>(device), _DescriptorSetLayout);
	}

	void DescriptorSetLayout::UpdateDescriptorSet(gpu::Device& device, const DescriptorSet& descriptorSet, const void* data)
	{
		static_cast<VulkanDevice&>(device).GetCache().UpdateDescriptorSetWithTemplate(descriptorSet.GetHandle(), _DescriptorUpdateTemplate, data);
	}
};