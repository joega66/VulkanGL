#pragma once
#include <DRMShader.h>

class RayMarchingFS : public drm::Shader
{
public:
	RayMarchingFS(const ShaderResourceTable& Resources)
		: drm::Shader(Resources) 
	{
		Resources.Bind("ViewUniform", View);
		Resources.Bind("LightBuffer", LightBuffer);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo Base = { "../Shaders/RayMarching.glsl", "main", EShaderStage::Fragment };
		return Base;
	}

	uint32 View;
	uint32 LightBuffer;
};