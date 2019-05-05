#pragma once
#include "MaterialShader.h"
#include <DRM.h>

template<EMeshType MeshType>
class DepthPassVS : public MaterialVS<MeshType>
{
public:
	static const BaseShaderInfo& GetBaseShaderInfo()
	{
		static BaseShaderInfo Base = { "../Shaders/DepthPassVS.glsl", "main", EShaderStage::Vertex };
		return Base;
	}

	static void ModifyCompilationEnvironment(ShaderCompilerWorker& Worker)
	{
		MaterialVS<MeshType>::ModifyCompilationEnvironment(Worker);
		Worker.SetDefine("DEPTH_ONLY");
	}
};

class DepthPassDrawingPlan
{
public:
	DepthPassDrawingPlan(const struct StaticMeshResources& Resources, drm::UniformBufferRef LocalToWorldUniform);
	void GetPipelineState(PipelineStateInitializer& PSOInit) const {};
	GraphicsPipelineState GetGraphicsPipeline() const;
	void SetUniforms(RenderCommandList& CmdList, const View& View);
	void Draw(RenderCommandList& CmdList) const;

private:
	uint32 ViewLocation;
	uint32 LocalToWorldLocation;
	drm::UniformBufferRef LocalToWorldUniform;
	uint32 IndexCount;
	drm::IndexBufferRef IndexBuffer;
	StreamSource PositionStream;
	drm::ShaderRef VertexShader;
};