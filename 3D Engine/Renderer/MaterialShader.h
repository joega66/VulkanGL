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

template<EMeshType MeshType>
class MaterialShader : public MeshShader<MeshType>
{
	using Base = MeshShader<MeshType>;
public:
	MaterialShader(const ShaderCompilationInfo& CompilationInfo)
		: Base(CompilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		Base::SetEnvironmentVariables(Worker);
	}
};