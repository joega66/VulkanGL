#pragma once
#include "GPU/GPUShader.h"

class FullscreenVS : public gpu::Shader
{
public:
	FullscreenVS() = default;
	FullscreenVS(const ShaderCompilationInfo& compilationInfo)
		: gpu::Shader(compilationInfo)
	{
	}
};

REGISTER_SHADER(FullscreenVS, "../Shaders/FullscreenVS.glsl", "main", EShaderStage::Vertex);