#pragma once
#include "DRMShader.h"

class FullscreenVS : public drm::Shader
{
public:
	FullscreenVS(const ShaderCompilationInfo& CompilationInfo)
		: drm::Shader(CompilationInfo)
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
	Voxels,
};

template<EVisualize Visualize>
class FullscreenFS : public drm::Shader
{
public:
	FullscreenFS(const ShaderCompilationInfo& CompilationInfo)
		: drm::Shader(CompilationInfo)
	{
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo Base = { "../Shaders/FullscreenFS.glsl", "main", EShaderStage::Fragment };
		return Base;
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		if constexpr (Visualize == EVisualize::Voxels)
		{
			Worker.SetDefine("VOXELS");
		}
	}
};