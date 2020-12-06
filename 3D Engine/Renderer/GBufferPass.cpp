#include "MaterialShader.h"
#include "CameraRender.h"
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

void SceneRenderer::RenderGBufferPass(const Camera& camera, CameraRender& cameraRender, gpu::CommandBuffer& cmdBuf)
{
	cmdBuf.BeginRenderPass(cameraRender._GBufferRP);

	cmdBuf.SetViewportAndScissor({ .width = cameraRender._SceneDepth.GetWidth(), .height = cameraRender._SceneDepth.GetHeight() });
	
	const FrustumPlanes viewFrustumPlanes = camera.GetFrustumPlanes();

	for (auto entity : _ECS.GetEntities<SurfaceGroup>())
	{
		auto& surfaceGroup = _ECS.GetComponent<SurfaceGroup>(entity);
		const VkDescriptorSet descriptorSets[] = { CameraDescriptors::_DescriptorSet, surfaceGroup.GetSurfaceSet(), _Device.GetTextures() };
		const uint32 dynamicOffsets[] = { cameraRender.GetDynamicOffset() };

		surfaceGroup.Draw<true>(_Device, cmdBuf, std::size(descriptorSets), descriptorSets, std::size(dynamicOffsets), dynamicOffsets, [&] ()
		{
			GraphicsPipelineDesc graphicsDesc = {};
			graphicsDesc.renderPass = cameraRender._GBufferRP;
			graphicsDesc.shaderStages.vertex = _Device.FindShader<GBufferPassVS>();
			graphicsDesc.shaderStages.fragment = _Device.FindShader<GBufferPassFS>();

			return graphicsDesc;
		}, &viewFrustumPlanes);
	}

	cmdBuf.EndRenderPass();
}