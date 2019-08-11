#pragma once
#include <DRMShader.h>

class SceneBindings
{
public:
	SceneBindings(const ShaderResourceTable& Resources);

	ShaderBinding ViewUniform;
	ShaderBinding PointLightBuffer;
};