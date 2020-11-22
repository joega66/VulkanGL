#pragma once
#include <GPU/GPUShader.h>

class MeshShader : public gpu::Shader
{
public:
	MeshShader() = default;
	MeshShader(const ShaderCompilationInfo& compilationInfo)
		: gpu::Shader(compilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& worker)
	{
		worker.SetDefine("STATIC_MESH");
	}
};