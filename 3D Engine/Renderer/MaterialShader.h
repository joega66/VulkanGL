#pragma once
#include <DRMShader.h>
#include "DrawingPlan.h"
#include <Components/CMaterial.h>

enum class EMeshType
{
	StaticMesh
};

template<EMeshType MeshType>
class MaterialVS : public drm::Shader
{
public:
	MaterialVS(const ShaderResourceTable& Resources)
		: drm::Shader(Resources)
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

template<bool bHasNormalMap, EMeshType MeshType>
class MaterialFS : public drm::Shader
{
public:
	MaterialFS(const ShaderResourceTable& Resources)
		: drm::Shader(Resources)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
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