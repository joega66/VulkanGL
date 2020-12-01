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