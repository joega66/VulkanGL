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
	Depth,
	RGBA8,
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
		Worker.SetDefine("TEXTURE", static_cast<uint32>(Visualize));
	}
};