#include "GPU/GPUShader.h"
#include "GPU/GPUDefinitions.h"

template<>
std::string ShaderTypeSerializer::Serialize<uint32>()
{
	return "uint";
}

template<>
std::string ShaderTypeSerializer::Serialize<float>()
{
	return "float";
}

template<>
std::string ShaderTypeSerializer::Serialize<glm::vec3>()
{
	return "vec3";
}

template<>
std::string ShaderTypeSerializer::Serialize<glm::vec4>()
{
	return "vec4";
}

template<>
std::string ShaderTypeSerializer::Serialize<glm::mat4>()
{
	return "mat4";
}

template<>
std::string ShaderTypeSerializer::Serialize<gpu::TextureID>()
{
	return "uint";
}

template<>
std::string ShaderTypeSerializer::Serialize<gpu::SamplerID>()
{
	return "uint";
}

template<>
std::string ShaderTypeSerializer::Serialize<gpu::ImageID>()
{
	return "uint";
}

template<>
EDescriptorType ShaderTypeSerializer::GetDescriptorType<gpu::SampledImage>()
{
	return EDescriptorType::SampledImage;
}

template<>
EDescriptorType ShaderTypeSerializer::GetDescriptorType<gpu::StorageImage>()
{
	return EDescriptorType::StorageImage;
}

template<>
EDescriptorType ShaderTypeSerializer::GetDescriptorType<gpu::UniformBuffer>()
{
	return EDescriptorType::UniformBuffer;
}

template<>
EDescriptorType ShaderTypeSerializer::GetDescriptorType<gpu::StorageBuffer>()
{
	return EDescriptorType::StorageBuffer;
}