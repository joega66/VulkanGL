#pragma once
#include "MaterialShader.h"
#include "SceneBindings.h"
#include <DRM.h>

template<EMeshType MeshType>
class DepthPassVS : public MaterialVS<MeshType>
{
public:
	DepthPassVS(const ShaderResourceTable& Resources)
		: SceneBindings(Resources), MaterialVS<MeshType>(Resources)
	{
		Resources.Bind("LocalToWorldUniform", LocalToWorld);
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

	SceneBindings SceneBindings;
	ShaderBinding LocalToWorld;
};

class DepthPassDrawingPlan
{
public:
	DepthPassDrawingPlan(const struct MeshElement& Element, drm::UniformBufferRef LocalToWorldUniform);
	void SetPipelineState(PipelineStateInitializer& PSOInit) const;
	void SetUniforms(RenderCommandList& CmdList, const class SceneRenderer& SceneRenderer);
	void Draw(RenderCommandList& CmdList) const;

protected:
	Ref<DepthPassVS<EMeshType::StaticMesh>> VertexShader;

private:
	const struct MeshElement& Element;
	drm::UniformBufferRef LocalToWorldUniform;
};