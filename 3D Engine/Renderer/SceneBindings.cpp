#include "SceneBindings.h"
#include "SceneProxy.h"

SceneBindings::SceneBindings(const ShaderResourceTable& Resources)
{
	Resources.Bind("ViewUniform", ViewUniform);
	Resources.Bind("PointLightBuffer", PointLightBuffer);
}

void SceneProxy::SetResources(RenderCommandList& CmdList, const drm::ShaderRef& Shader, const SceneBindings& Bindings) const
{
	if (Bindings.ViewUniform)
		CmdList.SetUniformBuffer(Shader, Bindings.ViewUniform, ViewUniform);

	if (Bindings.PointLightBuffer)
		CmdList.SetStorageBuffer(Shader, Bindings.PointLightBuffer, PointLightBuffer);
}