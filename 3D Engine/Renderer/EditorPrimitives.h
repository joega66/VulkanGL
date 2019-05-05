#pragma once
#include "DepthPass.h"

class OutlineFS : public GLShader
{
public:
	static const GLBaseShaderInfo& GetBaseShaderInfo()
	{
		static GLBaseShaderInfo Base = { "../Shaders/OutlineFS.glsl", "main", EShaderStage::Fragment };
		return Base;
	}
};

class OutlineDrawingPlan : public DepthPassDrawingPlan
{
public:
	OutlineDrawingPlan(const struct StaticMeshResources& Resources, GLUniformBufferRef LocalToWorldUniform);
	void GetPipelineState(PipelineStateInitializer& PSOInit) const {};
	GraphicsPipeline GetGraphicsPipeline() const;
	void SetUniforms(RenderCommandList& CmdList, const View& View);
	void Draw(RenderCommandList& CmdList) const;

private:
	GLShaderRef FragmentShader;
};

class SkyboxVS : public GLShader
{
public:
	static const GLBaseShaderInfo& GetBaseShaderInfo()
	{
		static GLBaseShaderInfo Base = { "../Shaders/SkyboxVS.glsl", "main", EShaderStage::Vertex };
		return Base;
	}
};

class SkyboxFS : public GLShader
{
public:
	static const GLBaseShaderInfo& GetBaseShaderInfo()
	{
		static GLBaseShaderInfo Base = { "../Shaders/SkyboxFS.glsl", "main", EShaderStage::Fragment };
		return Base;
	}
};

class LinesVS : public GLShader
{
public:
	static const GLBaseShaderInfo& GetBaseShaderInfo()
	{
		static GLBaseShaderInfo Base = { "../Shaders/LinesVS.glsl", "main", EShaderStage::Vertex };
		return Base;
	}
};

class LinesFS : public GLShader
{
public:
	static const GLBaseShaderInfo& GetBaseShaderInfo()
	{
		static GLBaseShaderInfo Base = { "../Shaders/LinesFS.glsl", "main", EShaderStage::Fragment };
		return Base;
	}
};

class LineDrawingPlan
{
public:
	LineDrawingPlan(const glm::vec3& A, const glm::vec3& B, const glm::vec4& Color, float Width = 1.0f);
	void GetPipelineState(PipelineStateInitializer& PSOInit) const;
	GraphicsPipeline GetGraphicsPipeline() const;
	void SetUniforms(RenderCommandList& CmdList, const View& View);
	void Draw(RenderCommandList& CmdList) const;

private:
	GLVertexBufferRef PositionBuffer;
	GLUniformBufferRef ColorUniform;
	float LineWidth;
};