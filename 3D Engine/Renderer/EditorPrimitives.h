#pragma once
#include <DRMShader.h>

class SkyboxVS : public drm::Shader
{
public:
	SkyboxVS(const ShaderResourceTable& Resources)
		: drm::Shader(Resources)
	{
		Resources.Bind("ViewUniform", View);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo Base = { "../Shaders/SkyboxVS.glsl", "main", EShaderStage::Vertex };
		return Base;
	}

	ShaderBinding View;
};

class SkyboxFS : public drm::Shader
{
public:
	SkyboxFS(const ShaderResourceTable& Resources)
		: drm::Shader(Resources)
	{
		Resources.Bind("Skybox", Skybox);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo Base = { "../Shaders/SkyboxFS.glsl", "main", EShaderStage::Fragment };
		return Base;
	}

	ShaderBinding Skybox;
};