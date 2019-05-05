#pragma once
#include "MaterialShader.h"
#include <Components/CStaticMesh.h>

class LightingPassBaseVS : public drm::Shader
{
public:
	static const BaseShaderInfo& GetBaseShaderInfo()
	{
		static BaseShaderInfo BaseInfo = { "../Shaders/LightingPassVS.glsl", "main", EShaderStage::Vertex };
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
	static const BaseShaderInfo& GetBaseShaderInfo()
	{
		static BaseShaderInfo BaseInfo = { "../Shaders/LightingPassFS.glsl", "main", EShaderStage::Fragment };
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
	LightingPassDrawingPlan(const struct StaticMeshResources& Resources, CMaterial& CMaterial, drm::UniformBufferRef LocalToWorldUniform);

	void GetPipelineState(PipelineStateInitializer& PSOInit) const {};
	GraphicsPipelineState GetGraphicsPipeline() const;
	void SetUniforms(RenderCommandList& CmdList, const View& View);
	void Draw(RenderCommandList& CmdList) const;

private:
	drm::ShaderRef LightingPassVert;
	drm::ShaderRef LightingPassFrag;
};