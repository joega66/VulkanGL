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
	GraphicsPipeline GetGraphicsPipeline() const;
	void SetUniforms(const View& View);
	void Draw() const;

private:
	GLShaderRef FragmentShader;
};

struct DrawLineInfo
{
};