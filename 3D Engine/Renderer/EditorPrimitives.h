#pragma once
#include "DepthPass.h"

class OutlineFS : public drm::Shader
{
public:
	static const BaseShaderInfo& GetBaseShaderInfo()
	{
		static BaseShaderInfo Base = { "../Shaders/OutlineFS.glsl", "main", EShaderStage::Fragment };
		return Base;
	}
};

class OutlineDrawingPlan : public DepthPassDrawingPlan
{
public:
	OutlineDrawingPlan(const struct StaticMeshResources& Resources, drm::UniformBufferRef LocalToWorldUniform);
	void GetPipelineState(PipelineStateInitializer& PSOInit) const {};
	GraphicsPipelineState GetGraphicsPipeline() const;
	void SetUniforms(RenderCommandList& CmdList, const View& View);
	void Draw(RenderCommandList& CmdList) const;

private:
	drm::ShaderRef FragmentShader;
};

class SkyboxVS : public drm::Shader
{
public:
	static const BaseShaderInfo& GetBaseShaderInfo()
	{
		static BaseShaderInfo Base = { "../Shaders/SkyboxVS.glsl", "main", EShaderStage::Vertex };
		return Base;
	}
};

class SkyboxFS : public drm::Shader
{
public:
	static const BaseShaderInfo& GetBaseShaderInfo()
	{
		static BaseShaderInfo Base = { "../Shaders/SkyboxFS.glsl", "main", EShaderStage::Fragment };
		return Base;
	}
};

class LinesVS : public drm::Shader
{
public:
	static const BaseShaderInfo& GetBaseShaderInfo()
	{
		static BaseShaderInfo Base = { "../Shaders/LinesVS.glsl", "main", EShaderStage::Vertex };
		return Base;
	}
};

class LinesFS : public drm::Shader
{
public:
	static const BaseShaderInfo& GetBaseShaderInfo()
	{
		static BaseShaderInfo Base = { "../Shaders/LinesFS.glsl", "main", EShaderStage::Fragment };
		return Base;
	}
};

class LineDrawingPlan
{
public:
	LineDrawingPlan(const glm::vec3& A, const glm::vec3& B, const glm::vec4& Color, float Width = 1.0f);
	void GetPipelineState(PipelineStateInitializer& PSOInit) const;
	GraphicsPipelineState GetGraphicsPipeline() const;
	void SetUniforms(RenderCommandList& CmdList, const View& View);
	void Draw(RenderCommandList& CmdList) const;

private:
	drm::VertexBufferRef PositionBuffer;
	drm::UniformBufferRef ColorUniform;
	float LineWidth;
};