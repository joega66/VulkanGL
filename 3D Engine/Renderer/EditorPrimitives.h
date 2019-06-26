#pragma once
#include "DepthPass.h"

class OutlineFS : public drm::Shader
{
public:
	OutlineFS(const ShaderResourceTable& Resources)
		: drm::Shader(Resources)
	{
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo Base = { "../Shaders/OutlineFS.glsl", "main", EShaderStage::Fragment };
		return Base;
	}
};

class OutlineDrawingPlan : public DepthPassDrawingPlan
{
public:
	OutlineDrawingPlan(const struct MeshElement& Element, drm::UniformBufferRef LocalToWorldUniform);
	void GetPipelineState(PipelineStateInitializer& PSOInit) const {};
	GraphicsPipelineState GetGraphicsPipeline() const;
	void SetUniforms(RenderCommandList& CmdList, const View& View);
	void Draw(RenderCommandList& CmdList) const;

private:
	Ref<OutlineFS> FragmentShader;
};

class SkyboxVS : public drm::Shader
{
public:
	SkyboxVS(const ShaderResourceTable& Resources)
		: drm::Shader(Resources)
	{
		Resources.Bind("ViewUniform", View);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo Base = { "../Shaders/SkyboxVS.glsl", "main", EShaderStage::Vertex };
		return Base;
	}

	uint32 View;
};

class SkyboxFS : public drm::Shader
{
public:
	SkyboxFS(const ShaderResourceTable& Resources)
		: drm::Shader(Resources)
	{
		Resources.Bind("Skybox", Skybox);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo Base = { "../Shaders/SkyboxFS.glsl", "main", EShaderStage::Fragment };
		return Base;
	}

	uint32 Skybox;
};

class LinesVS : public drm::Shader
{
public:
	LinesVS(const ShaderResourceTable& Resources)
		: drm::Shader(Resources)
	{
		Resources.Bind("ViewUniform", ViewLocation);
	}

	void SetUniforms(RenderCommandList& CmdList, drm::UniformBufferRef ViewUniform)
	{
		CmdList.SetUniformBuffer(drm::Shader::GetShader(), ViewLocation, ViewUniform);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo Base = { "../Shaders/LinesVS.glsl", "main", EShaderStage::Vertex };
		return Base;
	}

private:
	uint32 ViewLocation;
};

class LinesFS : public drm::Shader
{
public:
	LinesFS(const ShaderResourceTable& Resources)
		: drm::Shader(Resources)
	{
		Resources.Bind("ColorUniform", Color);
	}

	void SetUniforms(RenderCommandList& CmdList, drm::UniformBufferRef ColorUniform)
	{
		CmdList.SetUniformBuffer(drm::Shader::GetShader(), Color, ColorUniform);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo Base = { "../Shaders/LinesFS.glsl", "main", EShaderStage::Fragment };
		return Base;
	}

private:
	uint32 Color;
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