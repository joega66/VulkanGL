#include "SceneBindings.h"

SceneBindings::SceneBindings(const ShaderResourceTable& Resources)
{
	Resources.Bind("ViewUniform", ViewUniform);
	Resources.Bind("PointLightBuffer", PointLightBuffer);
}