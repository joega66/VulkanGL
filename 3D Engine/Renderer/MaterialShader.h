#pragma once
#include "../GLShader.h"
#include "DrawingPlan.h"
#include "Components/CMaterial.h"

enum class EMeshType
{
	StaticMesh
};

template<EMeshType MeshType>
class MaterialVS : public GLShader
{
public:
	static void ModifyCompilationEnvironment(ShaderCompilerWorker& Worker)
	{
		if constexpr (MeshType == EMeshType::StaticMesh)
		{
			Worker.SetDefine("STATIC_MESH");
		}
	}
};

template<bool bHasNormalMap, EMeshType MeshType>
class MaterialFS : public GLShader
{
public:
	static void ModifyCompilationEnvironment(ShaderCompilerWorker& Worker)
	{
		if constexpr (bHasNormalMap)
		{
			Worker.SetDefine("HAS_NORMAL_MAP");
		}

		if constexpr (MeshType == EMeshType::StaticMesh)
		{
			Worker.SetDefine("STATIC_MESH");
		}
	}
};

class MaterialDrawingPlan
{
public:
	void Construct(const struct StaticMeshResources& Resources, 
		const MaterialProxy& MaterialProxy,
		GLUniformBufferRef LocalToWorldUniform,
		GraphicsPipeline&& Pipeline);
	void SetUniforms(const View& View, GraphicsPipeline&& Pipeline);
	void Draw() const;

private:
	uint32 ViewLocation;
	std::vector<MaterialSource> Materials;
	uint32 LocalToWorldLocation;
	GLUniformBufferRef LocalToWorldUniform;
	uint32 IndexCount;
	GLIndexBufferRef IndexBuffer;
	std::vector<StreamSource> Streams;
};