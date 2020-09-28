#include "MaterialShader.h"
#include "SceneRenderer.h"
#include "ShadowProxy.h"
#include "FullscreenQuad.h"
#include "Surface.h"
#include <ECS/EntityManager.h>

template<EMeshType meshType>
class ShadowDepthVS : public MeshShader<meshType>
{
	using Base = MeshShader<meshType>;
public:
	ShadowDepthVS(const ShaderCompilationInfo& compilationInfo)
		: Base(compilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& worker)
	{
		Base::SetEnvironmentVariables(worker);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo info = { "../Shaders/ShadowDepthVS.glsl", "main", EShaderStage::Vertex };
		return info;
	}
};

template<EMeshType meshType>
class ShadowDepthFS : public MeshShader<meshType>
{
	using Base = MeshShader<meshType>;
public:
	ShadowDepthFS(const ShaderCompilationInfo& compilationInfo)
		: Base(compilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& worker)
	{
		Base::SetEnvironmentVariables(worker);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo info = { "../Shaders/ShadowDepthFS.glsl", "main", EShaderStage::Fragment };
		return info;
	}
};

void SceneRenderer::RenderShadowDepths(CameraProxy& camera, gpu::CommandList& cmdList)
{
	for (auto entity : _ECS.GetEntities<ShadowProxy>())
	{
		ShadowProxy& shadowProxy = _ECS.GetComponent<ShadowProxy>(entity);

		cmdList.BeginRenderPass(shadowProxy.GetRenderPass());

		for (auto entity : _ECS.GetEntities<SurfaceGroup>())
		{
			auto& surfaceGroup = _ECS.GetComponent<SurfaceGroup>(entity);

			const VkDescriptorSet descriptorSets[] = { shadowProxy.GetDescriptorSet(), surfaceGroup.GetSurfaceSet(), _Device.GetTextures(), _Device.GetSamplers() };

			surfaceGroup.Draw<false>(_Device, cmdList, std::size(descriptorSets), descriptorSets, [&] ()
			{
				constexpr EMeshType meshType = EMeshType::StaticMesh;

				PipelineStateDesc psoDesc = {};
				psoDesc.renderPass = shadowProxy.GetRenderPass();
				psoDesc.shaderStages.vertex = _ShaderLibrary.FindShader<ShadowDepthVS<meshType>>();
				psoDesc.shaderStages.fragment = _ShaderLibrary.FindShader<ShadowDepthFS<meshType>>();
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