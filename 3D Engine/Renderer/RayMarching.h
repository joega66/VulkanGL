#pragma once
#include <GLShader.h>

class RayMarchingFS : public GLShader
{
public:
	static const GLBaseShaderInfo& GetBaseShaderInfo()
	{
		static GLBaseShaderInfo Base = { "../Shaders/RayMarching.glsl", "main", EShaderStage::Fragment };
		return Base;
	}
};