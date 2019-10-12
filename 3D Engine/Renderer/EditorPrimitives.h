#pragma once
#include "MaterialShader.h"

class SkyboxVS : public MaterialShader
{
public:
	SkyboxVS(const ShaderCompilationInfo& CompilationInfo)
		: MaterialShader(CompilationInfo)
	{
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo Base = { "../Shaders/SkyboxVS.glsl", "main", EShaderStage::Vertex };
		return Base;
	}
};

class SkyboxFS : public MaterialShader
{
public:
	SkyboxFS(const ShaderCompilationInfo& CompilationInfo)
		: MaterialShader(CompilationInfo)
	{
		CompilationInfo.Bind("Skybox", Skybox);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo Base = { "../Shaders/SkyboxFS.glsl", "main", EShaderStage::Fragment };
		return Base;
	}

	ShaderBinding Skybox;
};