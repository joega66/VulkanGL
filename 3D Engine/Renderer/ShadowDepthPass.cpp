#include "MaterialShader.h"
#include "SceneRenderer.h"
#include "ShadowProxy.h"
#include "FullscreenQuad.h"
#include "Surface.h"
#include <ECS/EntityManager.h>
#include <Systems/ShadowSystem.h>

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

void SceneRenderer::RenderShadowDepths(CameraProxy& camera, gpu::CommandBuffer& cmdBuf)
{
	for (auto entity : _ECS.GetEntities<ShadowProxy>())
	{
		ShadowProxy& shadowProxy = _ECS.GetComponent<ShadowProxy>(entity);

		cmdBuf.BeginRenderPass(shadowProxy.GetRenderPass());

		cmdBuf.SetViewportAndScissor({ .width = shadowProxy.GetShadowMap().GetWidth(), .height = shadowProxy.GetShadowMap().GetHeight() });

		for (auto entity : _ECS.GetEntities<SurfaceGroup>())
		{
			auto& surfaceGroup = _ECS.GetComponent<SurfaceGroup>(entity);
			const VkDescriptorSet descriptorSets[] = { ShadowDescriptors::_DescriptorSet, surfaceGroup.GetSurfaceSet(), _Device.GetTextures() };
			const uint32 dynamicOffsets[] = { shadowProxy.GetDynamicOffset() };

			surfaceGroup.Draw<false>(_Device, cmdBuf, std::size(descriptorSets), descriptorSets, std::size(dynamicOffsets), dynamicOffsets, [&] ()
			{
				PipelineStateDesc psoDesc = {};
				psoDesc.renderPass = shadowProxy.GetRenderPass();
				psoDesc.shaderStages.vertex = _ShaderLibrary.FindShader<ShadowDepthVS>();
				psoDesc.shaderStages.fragment = _ShaderLibrary.FindShader<ShadowDepthFS>();
				psoDesc.rasterizationState.depthBiasEnable = true;
				psoDesc.rasterizationState.depthBiasConstantFactor = shadowProxy.GetDepthBiasConstantFactor();
				psoDesc.rasterizationState.depthBiasSlopeFactor = shadowProxy.GetDepthBiasSlopeFactor();
				return psoDesc;
			});
		}

		cmdBuf.EndRenderPass();
	}
}