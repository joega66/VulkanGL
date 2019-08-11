#pragma once
#include "MaterialShader.h"
#include <DRM.h>

template<EMeshType MeshType>
class DepthPassVS : public MaterialVS<MeshType>
{
public:
	DepthPassVS(const ShaderResourceTable& Resources)
		: MaterialVS<MeshType>(Resources)
	{
		Resources.Bind("ViewUniform", ViewLocation);
		Resources.Bind("LocalToWorldUniform", LocalToWorldLocation);
	}

	void SetUniforms(RenderCommandList& CmdList, drm::UniformBufferRef ViewUniform, drm::UniformBufferRef LocalToWorldUniform)
	{
		CmdList.SetUniformBuffer(drm::Shader::GetShader(), ViewLocation, ViewUniform);
		CmdList.SetUniformBuffer(drm::Shader::GetShader(), LocalToWorldLocation, LocalToWorldUniform);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo Info = { "../Shaders/DepthPassVS.glsl", "main", EShaderStage::Vertex };
		return Info;
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		MaterialVS<MeshType>::SetEnvironmentVariables(Worker);
		Worker.SetDefine("DEPTH_ONLY");
	}

private:
	ShaderBinding ViewLocation;
	ShaderBinding LocalToWorldLocation;
};

class DepthPassDrawingPlan
{
public:
	DepthPassDrawingPlan(const struct MeshElement& Element, drm::UniformBufferRef LocalToWorldUniform);
	void SetPipelineState(PipelineStateInitializer& PSOInit) const;
	void SetUniforms(RenderCommandList& CmdList, const class Scene& Scene);
	void Draw(RenderCommandList& CmdList) const;

protected:
	Ref<DepthPassVS<EMeshType::StaticMesh>> VertexShader;

private:
	const struct MeshElement& Element;
	drm::UniformBufferRef LocalToWorldUniform;
};