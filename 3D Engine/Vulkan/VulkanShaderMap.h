#pragma once
#include <DRMShader.h>

class VulkanDevice;

class VulkanShaderMap final : public DRMShaderMap
{
public:
	VulkanShaderMap(VulkanDevice& Device);

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