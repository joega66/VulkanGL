#pragma once
#include "GPU/GPUShader.h"

class FullscreenVS : public gpu::Shader
{
public:
	FullscreenVS() = default;
};

REGISTER_SHADER(FullscreenVS, "../Shaders/FullscreenVS.glsl", "main", EShaderStage::Vertex);