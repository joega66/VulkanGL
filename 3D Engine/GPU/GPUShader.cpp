#include "GPU/GPUShader.h"
#include "GPU/GPUDefinitions.h"

template<>
std::string ShaderTypeReflector::Reflect<uint32>()
{
	return "uint";
}

template<>
std::string ShaderTypeReflector::Reflect<float>()
{
	return "float";
}

template<>
std::string ShaderTypeReflector::Reflect<glm::vec2>()
{
	return "vec2";
}

template<>
std::string ShaderTypeReflector::Reflect<glm::vec3>()
{
	return "vec3";
}

template<>
std::string ShaderTypeReflector::Reflect<glm::vec4>()
{
	return "vec4";
}

template<>
std::string ShaderTypeReflector::Reflect<glm::mat4>()
{
	return "mat4";
}

template<>
std::string ShaderTypeReflector::Reflect<gpu::TextureID>()
{
	return "uint";
}

template<>
std::string ShaderTypeReflector::Reflect<gpu::ImageID>()
{
	return "uint";
}

namespace gpu
{
	std::string& GetShaderTypeReflectionTasks()
	{
		static std::string tasks = "";
		return tasks;
	}

	std::vector<DescriptorSetReflectionTask>& GetDescriptorSetReflectionTasks()
	{
		static std::vector<DescriptorSetReflectionTask> tasks;
		return tasks;
	}

	std::vector<ShaderCompilationTask>& GetShaderCompilationTasks()
	{
		static std::vector<ShaderCompilationTask> tasks;
		return tasks;
	}
}