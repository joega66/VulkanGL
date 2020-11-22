#pragma once
#include <GPU/GPUShader.h>

class MeshShader : public gpu::Shader
{
public:
	MeshShader() = default;

	static void SetEnvironmentVariables(ShaderCompilerWorker& worker)
	{
		worker.SetDefine("STATIC_MESH");
	}
};