#pragma once
#include <DRMShader.h>

enum class EMeshType
{
	StaticMesh
};

class MaterialShader : public drm::Shader
{
public:
	MaterialShader(const ShaderCompilationInfo& CompilationInfo)
		: drm::Shader(CompilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker);
};

template<EMeshType MeshType>
class MaterialVS : public MaterialShader
{
public:
	MaterialVS(const ShaderCompilationInfo& CompilationInfo)
		: MaterialShader(CompilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		MaterialShader::SetEnvironmentVariables(Worker);

		if constexpr (MeshType == EMeshType::StaticMesh)
		{
			Worker.SetDefine("STATIC_MESH");
		}
	}
};

template<EMeshType MeshType>
class MaterialFS : public MaterialShader
{
public:
	MaterialFS(const ShaderCompilationInfo& CompilationInfo)
		: MaterialShader(CompilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		MaterialShader::SetEnvironmentVariables(Worker);

		// REFLECTION WHEN FFS???
		if constexpr (MeshType == EMeshType::StaticMesh)
		{
			Worker.SetDefine("STATIC_MESH");
		}
	}
};