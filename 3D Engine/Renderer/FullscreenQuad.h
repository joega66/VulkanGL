#pragma once
#include "../DRMShader.h"

class FullscreenVS : public drm::Shader
{
public:
	static const BaseShaderInfo& GetBaseShaderInfo()
	{
		static BaseShaderInfo Base = { "../Shaders/FullscreenVS.glsl", "main", EShaderStage::Vertex };
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
	static const BaseShaderInfo& GetBaseShaderInfo()
	{
		static BaseShaderInfo Base = { "../Shaders/FullscreenFS.glsl", "main", EShaderStage::Fragment };
		return Base;
	}

	static void ModifyCompilationEnvironment(ShaderCompilerWorker& Worker)
	{
		if constexpr (Visualize == EVisualize::Depth)
		{
			Worker.SetDefine("DEPTH");
		}
	}
};