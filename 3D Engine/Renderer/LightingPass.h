#pragma once
#include "MaterialShader.h"
#include <Components/CStaticMesh.h>
#include <DRM.h>

class LightingPassBaseVS : public MaterialShader
{
public:
	LightingPassBaseVS(const ShaderCompilationInfo& CompilationInfo)
		: MaterialShader(CompilationInfo)
	{
		CompilationInfo.Bind("LocalToWorldUniform", LocalToWorld);
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
	LightingPassVS(const ShaderCompilationInfo& Resources)
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
	LightingPassBaseFS(const ShaderCompilationInfo& CompilationInfo)
		: MaterialShader(CompilationInfo)
	{
		CompilationInfo.Bind("Diffuse", Diffuse);
		CompilationInfo.Bind("Specular", Specular);
		CompilationInfo.Bind("Opacity", Opacity);
		CompilationInfo.Bind("Bump", Bump);
		CompilationInfo.Bind("HasOpacityMap", HasOpacityMap);
		CompilationInfo.Bind("HasSpecularMap", HasSpecularMap);
		CompilationInfo.Bind("HasBumpMap", HasBumpMap);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo BaseInfo = { "../Shaders/LightingPassFS.glsl", "main", EShaderStage::Fragment };
		return BaseInfo;
	}

	ShaderBinding Diffuse;
	ShaderBinding Specular;
	ShaderBinding Opacity;
	ShaderBinding Bump;
	SpecConstant HasSpecularMap;
	SpecConstant HasOpacityMap;
	SpecConstant HasBumpMap;
};

template<EMeshType MeshType>
class LightingPassFS : public LightingPassBaseFS
{
public:
	LightingPassFS(const ShaderCompilationInfo& CompilationInfo)
		: LightingPassBaseFS(CompilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		MaterialFS<MeshType>::SetEnvironmentVariables(Worker);
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
	SpecializationInfo SpecInfo;
	const MeshElement& Element;
};