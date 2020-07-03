#pragma once
#include <GPU/GPUShader.h>

enum class EMeshType
{
	StaticMesh
};

template<EMeshType meshType>
class MeshShader : public gpu::Shader
{
public:
	MeshShader(const ShaderCompilationInfo& compilationInfo)
		: gpu::Shader(compilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& worker)
	{
		if constexpr (meshType == EMeshType::StaticMesh)
		{
			worker.SetDefine("STATIC_MESH");
		}
	}
};