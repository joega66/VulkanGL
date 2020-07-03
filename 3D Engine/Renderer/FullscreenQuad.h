#pragma once
#include "GPU/GPUShader.h"

class FullscreenVS : public gpu::Shader
{
public:
	FullscreenVS(const ShaderCompilationInfo& compilationInfo)
		: gpu::Shader(compilationInfo)
	{
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo base = { "../Shaders/FullscreenVS.glsl", "main", EShaderStage::Vertex };
		return base;
	}
};

enum class EVisualize
{
	Depth,
	RGBA8,
};

template<EVisualize visualize>
class FullscreenFS : public gpu::Shader
{
public:
	FullscreenFS(const ShaderCompilationInfo& compilationInfo)
		: gpu::Shader(compilationInfo)
	{
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo base = { "../Shaders/FullscreenFS.glsl", "main", EShaderStage::Fragment };
		return base;
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& worker)
	{
		worker.SetDefine("TEXTURE", static_cast<uint32>(visualize));
	}
};