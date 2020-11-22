#pragma once
#include <GPU/GPUShader.h>

class VulkanDevice;

class VulkanShaderLibrary final : public gpu::ShaderLibrary
{
public:
	VulkanShaderLibrary(VulkanDevice& device);

	void RecompileShaders() override;

private:
	ShaderCompilationInfo CompileShader(
		const ShaderCompilerWorker& worker,
		const std::filesystem::path& path,
		const std::string& entryPoint,
		EShaderStage stage,
		std::type_index type
	) override;

	VulkanDevice& _Device;
};