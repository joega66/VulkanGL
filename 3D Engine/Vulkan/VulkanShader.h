#pragma once
#include "../GLShader.h"
#include <vulkan/vulkan.h>

class VulkanDevice;

class VulkanShaderCompiler : public GLShaderCompiler
{
public:
	virtual GLShaderRef CompileShader(ShaderCompilerWorker& Worker, const ShaderMetadata& Meta) const final;
};

class VulkanShader : public GLShader
{
public:
	std::vector<VkVertexInputAttributeDescription> Attributes;
	std::vector<VkDescriptorSetLayoutBinding> Bindings;
	VkShaderModule ShaderModule;

	VulkanShader(
		VulkanDevice& Device
		, VkShaderModule ShaderModule
		, const ShaderMetadata& Meta
		, const std::vector<VkVertexInputAttributeDescription>& Attributes
		, const std::vector<VkDescriptorSetLayoutBinding>& Bindings
		, const Map<std::string, uint32>& AttributeLocations
		, const Map<std::string, uint32>& UniformLocations
	);

	virtual void ReleaseGL() final;

	VkShaderStageFlagBits GetVulkanStage() const;

private:
	VulkanDevice& Device;
};

CLASS(VulkanShader);

const Map<std::string, VkFormat> GLSLTypeToVulkanFormat =
{
	ENTRY("vec4", VK_FORMAT_R32G32B32A32_SFLOAT)
	ENTRY("vec3", VK_FORMAT_R32G32B32_SFLOAT)
	ENTRY("vec2", VK_FORMAT_R32G32_SFLOAT)
	ENTRY("float", VK_FORMAT_R32_SFLOAT)
	ENTRY("int", VK_FORMAT_R32_SINT)
	ENTRY("uint", VK_FORMAT_R32_UINT)
};

const Map<std::string, uint32> GLSLTypeSizes =
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