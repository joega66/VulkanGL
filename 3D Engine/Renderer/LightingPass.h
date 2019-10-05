#pragma once
#include "MaterialShader.h"
#include <Components/CStaticMesh.h>
#include <DRM.h>

class LightingPassBaseVS : public MaterialShader
{
public:
	LightingPassBaseVS(const ShaderResourceTable& Resources)
		: MaterialShader(Resources)
	{
		Resources.Bind("LocalToWorldUniform", LocalToWorld);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo BaseInfo = { "../Shaders/LightingPassVS.glsl", "main", EShaderStage::Vertex };
		return BaseInfo;
	}

	ShaderBinding LocalToWorld;
};

template<EMeshType MeshType>
class LightingPassVS : public LightingPassBaseVS
{
public:
	LightingPassVS(const ShaderResourceTable& Resources)
		: LightingPassBaseVS(Resources)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		MaterialVS<MeshType>::SetEnvironmentVariables(Worker);
	}
};

class LightingPassBaseFS : public MaterialShader
{
public:
	LightingPassBaseFS(const ShaderResourceTable& Resources)
		: MaterialShader(Resources)
	{
		Resources.Bind("Diffuse", Diffuse);
		Resources.Bind("Opacity", Opacity);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo BaseInfo = { "../Shaders/LightingPassFS.glsl", "main", EShaderStage::Fragment };
		return BaseInfo;
	}

	ShaderBinding Diffuse;
	ShaderBinding Opacity;
};

template<bool bHasOpacityMap, EMeshType MeshType>
class LightingPassFS : public LightingPassBaseFS
{
public:
	LightingPassFS(const ShaderResourceTable& Resources)
		: LightingPassBaseFS(Resources)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		MaterialFS<bHasOpacityMap, MeshType>::SetEnvironmentVariables(Worker);
	}
};

class LightingPassDrawPlan
{
public:
	LightingPassDrawPlan(const MeshElement& Element, CMaterial& Material, drm::UniformBufferRef LocalToWorldUniform);

	void BindDescriptorSets(RenderCommandList& CmdList, const class SceneProxy& Scene) const;
	void SetPipelineState(PipelineStateInitializer& PSOInit) const;
	void Draw(RenderCommandList& CmdList) const;

private:
	Ref<LightingPassBaseVS> VertShader;
	Ref<LightingPassBaseFS> FragShader;
	drm::DescriptorSetRef DescriptorSet;
	const MeshElement& Element;
};