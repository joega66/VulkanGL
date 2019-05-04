#pragma once
#include "MaterialShader.h"
#include <Components/CStaticMesh.h>

class LightingPassBaseVS : public GLShader
{
public:
	static const GLBaseShaderInfo& GetBaseShaderInfo()
	{
		static GLBaseShaderInfo BaseInfo = { "../Shaders/LightingPassVS.glsl", "main", EShaderStage::Vertex };
		return BaseInfo;
	}
};

template<EMeshType MeshType>
class LightingPassVS : public LightingPassBaseVS
{
public:
	static void ModifyCompilationEnvironment(ShaderCompilerWorker& Worker)
	{
		MaterialVS<MeshType>::ModifyCompilationEnvironment(Worker);
	}
};

class LightingPassBaseFS
{
public:
	static const GLBaseShaderInfo& GetBaseShaderInfo()
	{
		static GLBaseShaderInfo BaseInfo = { "../Shaders/LightingPassFS.glsl", "main", EShaderStage::Fragment };
		return BaseInfo;
	}
};

template<bool bHasDiffuseMap, bool bHasNormalMap, EMeshType MeshType>
class LightingPassFS : public LightingPassBaseFS
{
public:
	static void ModifyCompilationEnvironment(ShaderCompilerWorker& Worker)
	{
		MaterialFS<bHasDiffuseMap, bHasNormalMap, MeshType>::ModifyCompilationEnvironment(Worker);
	}
};

class LightingPassDrawingPlan : public MaterialDrawingPlan
{
public:
	LightingPassDrawingPlan(const struct StaticMeshResources& Resources, CMaterial& CMaterial, GLUniformBufferRef LocalToWorldUniform);

	void GetPipelineState(PipelineStateInitializer& PSOInit) const {};
	GraphicsPipeline GetGraphicsPipeline() const;
	void SetUniforms(const View& View);
	void Draw() const;

private:
	GLShaderRef LightingPassVert;
	GLShaderRef LightingPassFrag;
};