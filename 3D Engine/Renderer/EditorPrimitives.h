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

class OutlineDrawPlan : public DepthPassDrawPlan
{
public:
	OutlineDrawPlan(const struct MeshElement& Element, drm::UniformBufferRef LocalToWorldUniform);
	void SetPipelineState(PipelineStateInitializer& PSOInit) const;
	void SetUniforms(RenderCommandList& CmdList, const class SceneRenderer& SceneRenderer);
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

	ShaderBinding View;
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

	ShaderBinding Skybox;
};

class LinesVS : public drm::Shader
{
public:
	LinesVS(const ShaderResourceTable& Resources)
		: SceneBindings(Resources), drm::Shader(Resources)
	{
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo Base = { "../Shaders/LinesVS.glsl", "main", EShaderStage::Vertex };
		return Base;
	}

	SceneBindings SceneBindings;
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
		CmdList.SetUniformBuffer(GetShader(), Color, ColorUniform);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo Base = { "../Shaders/LinesFS.glsl", "main", EShaderStage::Fragment };
		return Base;
	}

private:
	ShaderBinding Color;
};

class LineDrawPlan
{
public:
	LineDrawPlan(const glm::vec3& A, const glm::vec3& B, const glm::vec4& Color, float Width = 1.0f);
	void SetPipelineState(PipelineStateInitializer& PSOInit) const;
	void SetUniforms(RenderCommandList& CmdList, const class SceneRenderer& SceneRenderer);
	void Draw(RenderCommandList& CmdList) const;

private:
	drm::VertexBufferRef PositionBuffer;
	drm::UniformBufferRef ColorUniform;
	float LineWidth;
};