#pragma once
#include <GPU/GPUShader.h>

class VulkanDevice;

class VulkanShaderLibrary final : public gpu::ShaderLibrary
{
public:
	VulkanShaderLibrary(VulkanDevice& Device);

	virtual void RecompileShaders() override;

private:
	virtual ShaderCompilationInfo CompileShader(
		const ShaderCompilerWorker& Worker,
		const std::string& Filename,
		const std::string& EntryPoint,
		EShaderStage Stage,
		std::type_index Type
	) override;

	VulkanDevice& Device;
};