#include "MaterialShader.h"
#include "SceneRenderer.h"
#include "ShadowProxy.h"
#include "FullscreenQuad.h"
#include "GlobalRenderData.h"
#include <ECS/EntityManager.h>

void SceneRenderer::RenderShadowDepths(CameraProxy& Camera, drm::CommandList& CmdList)
{
	for (auto Entity : ECS.GetEntities<ShadowProxy>())
	{
		ShadowProxy& ShadowProxy = ECS.GetComponent<class ShadowProxy>(Entity);
		ShadowProxy.Render(CmdList);
	}
}