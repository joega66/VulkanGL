#pragma once
#include <DRMShader.h>

enum class EMeshType
{
	StaticMesh
};

template<EMeshType MeshType>
class MeshShader : public drm::Shader
{
public:
	MeshShader(const ShaderCompilationInfo& CompilationInfo)
		: drm::Shader(CompilationInfo)
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