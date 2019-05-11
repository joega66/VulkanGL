#pragma once
#include "MaterialShader.h"
#include <Components/CStaticMesh.h>

class LightingPassBaseVS : public drm::Shader
{
public:
	LightingPassBaseVS(const ShaderResourceTable& Resources)
		: drm::Shader(Resources)
	{
		Resources.Bind("ViewUniform", View);
		Resources.Bind("LocalToWorldUniform", LocalToWorld);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo BaseInfo = { "../Shaders/LightingPassVS.glsl", "main", EShaderStage::Vertex };
		return BaseInfo;
	}

	uint32 View;
	uint32 LocalToWorld;
};

template<EMeshType MeshType>
class LightingPassVS : public LightingPassBaseVS
{
public:
	LightingPassVS(const ShaderResourceTable& Resources)
		: LightingPassBaseVS(Resources)
	{
	}

	static void ModifyCompilationEnvironment(ShaderCompilerWorker& Worker)
	{
		MaterialVS<MeshType>::ModifyCompilationEnvironment(Worker);
	}
};

class LightingPassBaseFS : public drm::Shader
{
public:
	LightingPassBaseFS(const ShaderResourceTable& Resources)
		: drm::Shader(Resources)
	{
		Resources.Bind("Diffuse", Diffuse);
		Resources.Bind("DiffuseUniform", DiffuseUniform);
		Resources.Bind("Normal", Normal);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo BaseInfo = { "../Shaders/LightingPassFS.glsl", "main", EShaderStage::Fragment };
		return BaseInfo;
	}

	uint32 Diffuse;
	uint32 DiffuseUniform;
	uint32 Normal;
};

template<bool bHasDiffuseMap, bool bHasNormalMap, EMeshType MeshType>
class LightingPassFS : public LightingPassBaseFS
{
public:
	LightingPassFS(const ShaderResourceTable& Resources)
		: LightingPassBaseFS(Resources)
	{

	}

	static void ModifyCompilationEnvironment(ShaderCompilerWorker& Worker)
	{
		MaterialFS<bHasDiffuseMap, bHasNormalMap, MeshType>::ModifyCompilationEnvironment(Worker);
	}
};

class LightingPassDrawingPlan
{
public:
	LightingPassDrawingPlan(const struct StaticMeshResources& Resources, CMaterial& CMaterial, drm::UniformBufferRef LocalToWorldUniform);

	void GetPipelineState(PipelineStateInitializer& PSOInit) const {};
	GraphicsPipelineState GetGraphicsPipeline() const;
	void SetUniforms(RenderCommandList& CmdList, const View& View);
	void Draw(RenderCommandList& CmdList) const;

private:
	Ref<LightingPassBaseVS> VertShader;
	Ref<LightingPassBaseFS> FragShader;

	std::vector<MaterialSource> Materials;
	std::vector<UniformSource> Uniforms;
	uint32 IndexCount;
	drm::IndexBufferRef IndexBuffer;
	std::vector<StreamSource> Streams;
};