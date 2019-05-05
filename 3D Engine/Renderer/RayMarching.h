#pragma once
#include <DRMShader.h>

class RayMarchingFS : public drm::Shader
{
public:
	static const BaseShaderInfo& GetBaseShaderInfo()
	{
		static BaseShaderInfo Base = { "../Shaders/RayMarching.glsl", "main", EShaderStage::Fragment };
		return Base;
	}
};