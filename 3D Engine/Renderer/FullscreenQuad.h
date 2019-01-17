#pragma once
#include "../GLShader.h"

class FullscreenVS : public GLShader
{
public:
	static const GLBaseShaderInfo& GetBaseShaderInfo()
	{
		static GLBaseShaderInfo Base = { "../Shaders/FullscreenVS.glsl", "main", EShaderStage::Vertex };
		return Base;
	}
};

enum class EVisualize
{
	Depth,
	Default,
};

template<EVisualize Visualize = EVisualize::Default>
class FullscreenFS : public GLShader
{
public:
	static const GLBaseShaderInfo& GetBaseShaderInfo()
	{
		static GLBaseShaderInfo Base = { "../Shaders/FullscreenFS.glsl", "main", EShaderStage::Fragment };
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