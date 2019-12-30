#pragma once
#include <DRMShader.h>
#include <vulkan/vulkan.h>

class VulkanDevice;

class VulkanShader
{
public:
	std::vector<VkVertexInputAttributeDescription> Attributes;
	VkShaderModule ShaderModule = VK_NULL_HANDLE;

	VulkanShader() = default;

	VulkanShader(const VulkanDevice* Device, VkShaderModule ShaderModule, const std::vector<VkVertexInputAttributeDescription>& Attributes);

	~VulkanShader();

	static VkShaderStageFlagBits GetVulkanStage(EShaderStage Stage);

private:
	const VulkanDevice* Device = nullptr;

};