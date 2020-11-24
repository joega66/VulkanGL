#include "MaterialShader.h"
#include "SceneRenderer.h"
#include "ShadowProxy.h"
#include "FullscreenQuad.h"
#include "Surface.h"
#include <ECS/EntityManager.h>

class ShadowDepthVS : public MeshShader
{
	using Base = MeshShader;
public:
	ShadowDepthVS() = default;

	static void SetEnvironmentVariables(ShaderCompilerWorker& worker)
	{
		Base::SetEnvironmentVariables(worker);
	}
};

REGISTER_SHADER(ShadowDepthVS, "../Shaders/ShadowDepthVS.glsl", "main", EShaderStage::Vertex);

class ShadowDepthFS : public MeshShader
{
	using Base = MeshShader;
public:
	ShadowDepthFS() = default;

	static void SetEnvironmentVariables(ShaderCompilerWorker& worker)
	{
		Base::SetEnvironmentVariables(worker);
	}
};

REGISTER_SHADER(ShadowDepthFS, "../Shaders/ShadowDepthFS.glsl", "main", EShaderStage::Fragment);

void SceneRenderer::RenderShadowDepths(CameraProxy& camera, gpu::CommandList& cmdList)
{
	for (auto entity : _ECS.GetEntities<ShadowProxy>())
	{
		ShadowProxy& shadowProxy = _ECS.GetComponent<ShadowProxy>(entity);

		cmdList.BeginRenderPass(shadowProxy.GetRenderPass());

		for (auto entity : _ECS.GetEntities<SurfaceGroup>())
		{
			auto& surfaceGroup = _ECS.GetComponent<SurfaceGroup>(entity);

			const VkDescriptorSet descriptorSets[] = { shadowProxy.GetDescriptorSet(), surfaceGroup.GetSurfaceSet(), _Device.GetTextures() };

			surfaceGroup.Draw<false>(_Device, cmdList, std::size(descriptorSets), descriptorSets, [&] ()
			{
				PipelineStateDesc psoDesc = {};
				psoDesc.renderPass = shadowProxy.GetRenderPass();
				psoDesc.shaderStages.vertex = _ShaderLibrary.FindShader<ShadowDepthVS>();
				psoDesc.shaderStages.fragment = _ShaderLibrary.FindShader<ShadowDepthFS>();
				psoDesc.viewport.width = shadowProxy.GetShadowMap().GetWidth();
				psoDesc.viewport.height = shadowProxy.GetShadowMap().GetHeight();
				psoDesc.rasterizationState.depthBiasEnable = true;
				psoDesc.rasterizationState.depthBiasConstantFactor = shadowProxy.GetDepthBiasConstantFactor();
				psoDesc.rasterizationState.depthBiasSlopeFactor = shadowProxy.GetDepthBiasSlopeFactor();

				return psoDesc;
			});
		}

		cmdList.EndRenderPass();
	}
}