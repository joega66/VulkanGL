#pragma once
#include <DRMShader.h>

enum class EMeshType
{
	StaticMesh
};

class MaterialShader : public drm::Shader
{
public:
	MaterialShader(const ShaderResourceTable& Resources)
		: drm::Shader(Resources)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker);
};

template<EMeshType MeshType>
class MaterialVS : public MaterialShader
{
public:
	MaterialVS(const ShaderResourceTable& Resources)
		: MaterialShader(Resources)
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

template<bool bHasNormalMap, EMeshType MeshType>
class MaterialFS : public MaterialShader
{
public:
	MaterialFS(const ShaderResourceTable& Resources)
		: MaterialShader(Resources)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		MaterialShader::SetEnvironmentVariables(Worker);

		if constexpr (bHasNormalMap)
		{
			Worker.SetDefine("HAS_NORMAL_MAP");
		}

		// REFLECTION WHEN FFS???
		if constexpr (MeshType == EMeshType::StaticMesh)
		{
			Worker.SetDefine("STATIC_MESH");
		}
	}
};