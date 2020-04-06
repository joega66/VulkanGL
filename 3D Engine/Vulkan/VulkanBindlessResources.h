#pragma once
#include <Engine/Types.h>
#include <vulkan/vulkan.h>

class VulkanImageView;
class VulkanSampler;

class VulkanBindlessResources
{
public:
	VulkanBindlessResources(const VulkanBindlessResources&) = delete;
	VulkanBindlessResources& operator=(const VulkanBindlessResources&) = delete;

	VulkanBindlessResources(VkDevice Device, VkDescriptorType BindlessResourceType);
	~VulkanBindlessResources();

	inline VkDescriptorSetLayout GetLayout() const { return BindlessResourceSetLayout; }
	inline VkDescriptorSet GetResources() const { return BindlessResources; }

	uint32 Add(const VulkanImageView& ImageView, const VulkanSampler& Sampler);

private:
	VkDevice Device;
	static constexpr uint32 BindlessResourceCount = 65556;
	uint32 CurrNumBindlessResources = 0;
	VkDescriptorSetLayout BindlessResourceSetLayout;
	VkDescriptorPool BindlessResourceDescriptorPool;
	VkDescriptorSet BindlessResources;
};