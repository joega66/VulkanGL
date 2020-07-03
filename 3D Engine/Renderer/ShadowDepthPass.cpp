#include "MaterialShader.h"
#include "SceneRenderer.h"
#include "ShadowProxy.h"
#include "FullscreenQuad.h"
#include <ECS/EntityManager.h>

void SceneRenderer::RenderShadowDepths(CameraProxy& camera, gpu::CommandList& cmdList)
{
	for (auto entity : _ECS.GetEntities<ShadowProxy>())
	{
		ShadowProxy& shadowProxy = _ECS.GetComponent<ShadowProxy>(entity);
		shadowProxy.Render(cmdList);
	}
}