#pragma once
#include "../GLShader.h"
#include "DrawingPlan.h"
#include <Components/CMaterial.h>

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

template<bool bHasDiffuseMap, bool bHasNormalMap, EMeshType MeshType>
class MaterialFS : public GLShader
{
public:
	static void ModifyCompilationEnvironment(ShaderCompilerWorker& Worker)
	{
		if constexpr (bHasDiffuseMap)
		{
			Worker.SetDefine("HAS_DIFFUSE_MAP");
		}

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

class MaterialDrawingPlan
{
public:
	void Construct(const struct StaticMeshResources& Resources, 
		CMaterial& CMaterial,
		GLUniformBufferRef LocalToWorldUniform,
		GraphicsPipeline&& Pipeline);
	void SetUniforms(const View& View, GraphicsPipeline&& Pipeline);
	void Draw() const;

private:
	uint32 ViewLocation;
	std::vector<MaterialSource> Materials;
	std::vector<UniformSource> Uniforms;
	uint32 IndexCount;
	GLIndexBufferRef IndexBuffer;
	std::vector<StreamSource> Streams;
};