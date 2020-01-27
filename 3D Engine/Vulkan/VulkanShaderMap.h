#pragma once
#include <DRMShader.h>

class VulkanDRM;

class VulkanShaderMap final : public DRMShaderMap
{
public:
	VulkanShaderMap(VulkanDRM& Device);

	virtual void RecompileShaders() override;

private:
	virtual ShaderCompilationInfo CompileShader(
		const ShaderCompilerWorker& Worker,
		const std::string& Filename,
		const std::string& EntryPoint,
		EShaderStage Stage,
		std::type_index Type
	) override;

	VulkanDRM& Device;
};