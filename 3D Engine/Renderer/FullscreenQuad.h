#pragma once
#include "../DRMShader.h"

class FullscreenVS : public drm::Shader
{
public:
	FullscreenVS(const ShaderResourceTable& Resources)
		: drm::Shader(Resources)
	{
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo Base = { "../Shaders/FullscreenVS.glsl", "main", EShaderStage::Vertex };
		return Base;
	}
};

enum class EVisualize
{
	Depth,
	Default,
};

template<EVisualize Visualize = EVisualize::Default>
class FullscreenFS : public drm::Shader
{
public:
	FullscreenFS(const ShaderResourceTable& Resources)
		: drm::Shader(Resources)
	{
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo Base = { "../Shaders/FullscreenFS.glsl", "main", EShaderStage::Fragment };
		return Base;
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		if constexpr (Visualize == EVisualize::Depth)
		{
			Worker.SetDefine("DEPTH");
		}
	}
};