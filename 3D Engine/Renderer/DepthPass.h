#pragma once
#include "MaterialShader.h"

template<EMeshType MeshType>
class DepthPassVS : public MaterialVS<MeshType>
{
public:
	static const GLBaseShaderInfo& GetBaseShaderInfo()
	{
		static GLBaseShaderInfo Base = { "../Shaders/DepthPassVS.glsl", "main", EShaderStage::Vertex };
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
	DepthPassDrawingPlan(const struct StaticMeshResources& Resources, GLUniformBufferRef LocalToWorldUniform);
	void GetPipelineState(PipelineStateInitializer& PSOInit) const {};
	GraphicsPipeline GetGraphicsPipeline() const;
	void SetUniforms(const View& View);
	void Draw() const;

private:
	uint32 ViewLocation;
	uint32 LocalToWorldLocation;
	GLUniformBufferRef LocalToWorldUniform;
	uint32 IndexCount;
	GLIndexBufferRef IndexBuffer;
	StreamSource PositionStream;

	GLShaderRef VertexShader;
};