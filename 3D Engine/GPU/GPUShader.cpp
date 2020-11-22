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
std::string ShaderTypeSerializer::Serialize<glm::vec2>()
{
	return "vec2";
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

std::string& gpu::GetRegisteredShaderStructs()
{
	static std::string registrar = "";
	return registrar;
}

std::vector<RegisteredDescriptorSetType>& gpu::GetRegisteredDescriptorSetTypes()
{
	static std::vector<RegisteredDescriptorSetType> registrar;
	return registrar;
}

std::vector<ShaderCompilationTask>& gpu::GetShaderCompilationTasks()
{
	static std::vector<ShaderCompilationTask> tasks;
	return tasks;
}
