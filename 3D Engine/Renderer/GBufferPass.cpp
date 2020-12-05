#include "MaterialShader.h"
#include "CameraProxy.h"
#include "SceneRenderer.h"
#include <ECS/EntityManager.h>
#include <Engine/Engine.h>
#include <Renderer/Surface.h>
#include <Systems/CameraSystem.h>

class GBufferPassVS : public MeshShader
{
	using Base = MeshShader;
public:
	GBufferPassVS() = default;

	static void SetEnvironmentVariables(ShaderCompilerWorker& worker)
	{
		Base::SetEnvironmentVariables(worker);
	}
};

REGISTER_SHADER(GBufferPassVS, "../Shaders/GBufferVS.glsl", "main", EShaderStage::Vertex);

class GBufferPassFS : public MeshShader
{
	using Base = MeshShader;
public:
	GBufferPassFS() = default;

	static void SetEnvironmentVariables(ShaderCompilerWorker& worker)
	{
		Base::SetEnvironmentVariables(worker);
	}
};

REGISTER_SHADER(GBufferPassFS, "../Shaders/GBufferFS.glsl", "main", EShaderStage::Fragment);

void SceneRenderer::RenderGBufferPass(const Camera& camera, CameraProxy& cameraProxy, gpu::CommandList& cmdList)
{
	cmdList.BeginRenderPass(cameraProxy._GBufferRP);

	cmdList.SetViewportAndScissor({ .width = cameraProxy._SceneDepth.GetWidth(), .height = cameraProxy._SceneDepth.GetHeight() });

	const FrustumPlanes viewFrustumPlanes = camera.GetFrustumPlanes();

	for (auto entity : _ECS.GetEntities<SurfaceGroup>())
	{
		auto& surfaceGroup = _ECS.GetComponent<SurfaceGroup>(entity);
		const VkDescriptorSet descriptorSets[] = { CameraDescriptors::_DescriptorSet, surfaceGroup.GetSurfaceSet(), _Device.GetTextures() };
		const uint32 dynamicOffsets[] = { cameraProxy.GetDynamicOffset() };

		surfaceGroup.Draw<true>(_Device, cmdList, std::size(descriptorSets), descriptorSets, std::size(dynamicOffsets), dynamicOffsets, [&] ()
		{
			PipelineStateDesc psoDesc = {};
			psoDesc.renderPass = cameraProxy._GBufferRP;
			psoDesc.shaderStages.vertex = _ShaderLibrary.FindShader<GBufferPassVS>();
			psoDesc.shaderStages.fragment = _ShaderLibrary.FindShader<GBufferPassFS>();

			return psoDesc;
		}, &viewFrustumPlanes);
	}

	cmdList.EndRenderPass();
}