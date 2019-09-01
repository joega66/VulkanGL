#pragma once
#include "MaterialShader.h"
#include "SceneBindings.h"
#include <Components/CStaticMesh.h>

class LightingPassBaseVS : public drm::Shader
{
public:
	LightingPassBaseVS(const ShaderResourceTable& Resources)
		: drm::Shader(Resources), SceneBindings(Resources)
	{
		Resources.Bind("LocalToWorldUniform", LocalToWorld);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo BaseInfo = { "../Shaders/LightingPassVS.glsl", "main", EShaderStage::Vertex };
		return BaseInfo;
	}

	SceneBindings SceneBindings;
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

class LightingPassBaseFS : public drm::Shader
{
public:
	LightingPassBaseFS(const ShaderResourceTable& Resources)
		: drm::Shader(Resources), SceneBindings(Resources)
	{
		Resources.Bind("Diffuse", Diffuse);
		Resources.Bind("Normal", Normal);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo BaseInfo = { "../Shaders/LightingPassFS.glsl", "main", EShaderStage::Fragment };
		return BaseInfo;
	}

	SceneBindings SceneBindings;
	ShaderBinding Diffuse;
	ShaderBinding Normal;
};

template<bool bHasNormalMap, EMeshType MeshType>
class LightingPassFS : public LightingPassBaseFS
{
public:
	LightingPassFS(const ShaderResourceTable& Resources)
		: LightingPassBaseFS(Resources)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		MaterialFS<bHasNormalMap, MeshType>::SetEnvironmentVariables(Worker);
	}
};

class LightingPassDrawPlan
{
public:
	LightingPassDrawPlan(const struct MeshElement& Element, CMaterial& CMaterial, drm::UniformBufferRef LocalToWorldUniform);

	void SetPipelineState(PipelineStateInitializer& PSOInit) const;
	void SetUniforms(RenderCommandList& CmdList, const class SceneRenderer& SceneRenderer);
	void Draw(RenderCommandList& CmdList) const;

private:
	Ref<LightingPassBaseVS> VertShader;
	Ref<LightingPassBaseFS> FragShader;

	std::vector<MaterialSource> Materials;
	std::vector<UniformSource> Uniforms;
	const struct MeshElement& Element;
};