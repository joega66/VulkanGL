#include "VulkanDescriptors.h"
#include "VulkanInstance.h"
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

	auto p_vkCreateDescriptorUpdateTemplateKHR = (PFN_vkCreateDescriptorUpdateTemplateKHR)vkGetInstanceProcAddr(Device.GetInstance(), "vkCreateDescriptorUpdateTemplateKHR");

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
	DescriptorSet::DescriptorSet(VkDescriptorSet descriptorSet)
		: _DescriptorSet(descriptorSet)
	{
	}

	DescriptorSet::DescriptorSet(DescriptorSet&& other)
		: _DescriptorSet(std::exchange(other._DescriptorSet, nullptr))
	{
	}

	DescriptorSet& DescriptorSet::operator=(DescriptorSet&& other)
	{
		_DescriptorSet = std::exchange(other._DescriptorSet, nullptr);
		return *this;
	}

	DescriptorSetLayout::DescriptorSetLayout(
		VulkanDevice& device,
		std::size_t numBindings,
		const DescriptorBinding* bindings) 
		: _Device(&device)
	{
		static const VkDescriptorType descriptorTypes[] =
		{
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
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

			structSize += (descriptorUpdateTemplateEntry.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC || descriptorUpdateTemplateEntry.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ?
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

	DescriptorSet DescriptorSetLayout::CreateDescriptorSet()
	{
		const VkDescriptorSetAllocateInfo descriptorSetAllocateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = _Device->GetDescriptorPool(),
			.descriptorSetCount = 1,
			.pSetLayouts = &_DescriptorSetLayout
		};

		VkDescriptorSet descriptorSet;
		vulkan(vkAllocateDescriptorSets(*_Device, &descriptorSetAllocateInfo, &descriptorSet));

		return DescriptorSet(descriptorSet);
	}

	void DescriptorSetLayout::UpdateDescriptorSet(const DescriptorSet& descriptorSet, const void* data)
	{
		_Device->GetCache().UpdateDescriptorSetWithTemplate(descriptorSet.GetHandle(), _DescriptorUpdateTemplate, data);
	}
};