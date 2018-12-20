#pragma once
#include "DepthPass.h"

class ObjectHighlightFS : public GLShader
{
public:
	static const GLBaseShaderInfo& GetBaseShaderInfo()
	{
		static GLBaseShaderInfo Base = { "../Shaders/ObjectHighlightFS.glsl", "main", EShaderStage::Fragment };
		return Base;
	}
};

class ObjectHighlightDrawingPlan : public DepthPassDrawingPlan
{
public:
	ObjectHighlightDrawingPlan(const struct StaticMeshResources& Resources, GLUniformBufferRef LocalToWorldUniform);
	GraphicsPipeline GetGraphicsPipeline() const;
	void SetUniforms(const View& View);
	void Draw() const;

private:
	GLShaderRef FragmentShader;
};