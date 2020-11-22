#pragma once
#include <GPU/GPUShader.h>

class VulkanDevice;

class VulkanShaderLibrary final : public gpu::ShaderLibrary
{
public:
	VulkanShaderLibrary(VulkanDevice& device);

	void RecompileShaders() override;

private:
	ShaderCompilationResult CompileShader(
		const ShaderCompilerWorker& worker,
		const std::filesystem::path& path,
		const std::string& entrypoint,
		EShaderStage stage) override;

	VulkanDevice& _Device;
};