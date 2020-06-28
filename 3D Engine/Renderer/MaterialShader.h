#pragma once
#include <GPU/GPUShader.h>

enum class EMeshType
{
	StaticMesh
};

template<EMeshType MeshType>
class MeshShader : public gpu::Shader
{
public:
	MeshShader(const ShaderCompilationInfo& CompilationInfo)
		: gpu::Shader(CompilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		if constexpr (MeshType == EMeshType::StaticMesh)
		{
			Worker.SetDefine("STATIC_MESH");
		}
	}
};