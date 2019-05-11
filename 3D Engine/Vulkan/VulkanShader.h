#pragma once
#include <DRMShader.h>
#include <vulkan/vulkan.h>

class VulkanDevice;

class VulkanShader
{
public:
	std::vector<VkVertexInputAttributeDescription> Attributes;
	std::vector<VkDescriptorSetLayoutBinding> Bindings;
	VkShaderModule ShaderModule;

	VulkanShader() = default;
	VulkanShader(
		VkShaderModule ShaderModule
		, const std::vector<VkVertexInputAttributeDescription>& Attributes
		, const std::vector<VkDescriptorSetLayoutBinding>& Bindings);

	static VkShaderStageFlagBits GetVulkanStage(EShaderStage Stage);
};

const HashTable<std::string, VkFormat> GLSLTypeToVulkanFormat =
{
	ENTRY("vec4", VK_FORMAT_R32G32B32A32_SFLOAT)
	ENTRY("vec3", VK_FORMAT_R32G32B32_SFLOAT)
	ENTRY("vec2", VK_FORMAT_R32G32_SFLOAT)
	ENTRY("float", VK_FORMAT_R32_SFLOAT)
	ENTRY("int", VK_FORMAT_R32_SINT)
	ENTRY("uint", VK_FORMAT_R32_UINT)
};

const HashTable<std::string, uint32> GLSLTypeSizes =
{
	ENTRY("bool", 1)
	ENTRY("int", 4)
	ENTRY("uint", 4)
	ENTRY("float", 4)
	ENTRY("vec2", 8)
	ENTRY("vec3", 12)
	ENTRY("vec4", 16)
	ENTRY("ivec2", 8)
	ENTRY("ivec3", 12)
	ENTRY("ivec4", 16)
	ENTRY("uvec2", 8)
	ENTRY("uvec3", 12)
	ENTRY("uvec4", 16)
	ENTRY("mat2", 16)
	ENTRY("mat3", 36)
	ENTRY("mat4", 64)
};