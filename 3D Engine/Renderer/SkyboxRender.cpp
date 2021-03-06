#include "SceneRenderer.h"
#include <ECS/EntityManager.h>
#include <Engine/AssetManager.h>
#include <Components/SkyboxComponent.h>
#include <Systems/CameraSystem.h>

class SkyboxVS : public gpu::Shader
{
public:
	SkyboxVS() = default;

	static void SetEnvironmentVariables(ShaderCompilerWorker& worker)
	{
	}
};

REGISTER_SHADER(SkyboxVS, "../Shaders/SkyboxVS.glsl", "main", EShaderStage::Vertex);

BEGIN_PUSH_CONSTANTS(SkyboxParams)
	MEMBER(gpu::TextureID, _Skybox)
END_PUSH_CONSTANTS(SkyboxParams)

class SkyboxFS : public gpu::Shader
{
public:
	SkyboxFS() = default;
};

REGISTER_SHADER(SkyboxFS, "../Shaders/SkyboxFS.glsl", "main", EShaderStage::Fragment);

void SceneRenderer::RenderSkybox(CameraRender& camera, gpu::CommandBuffer& cmdBuf)
{
	cmdBuf.BeginRenderPass(camera._SkyboxRP);

	cmdBuf.SetViewportAndScissor({ .width = camera._SceneDepth.GetWidth(), .height = camera._SceneDepth.GetHeight() });

	const SkyboxVS* vertShader = _Device.FindShader<SkyboxVS>();
	const SkyboxFS* fragShader = _Device.FindShader<SkyboxFS>();

	GraphicsPipelineDesc graphicsDesc = {};
	graphicsDesc.renderPass = camera._SkyboxRP;
	graphicsDesc.depthStencilState.depthTestEnable = true;
	graphicsDesc.depthStencilState.depthWriteEnable = true;
	graphicsDesc.depthStencilState.depthCompareTest = ECompareOp::LessOrEqual;
	graphicsDesc.shaderStages = { vertShader, nullptr, nullptr, nullptr, fragShader };

	gpu::Pipeline pipeline = _Device.CreatePipeline(graphicsDesc);

	cmdBuf.BindPipeline(pipeline);

	const VkDescriptorSet descriptorSets[] = { CameraDescriptors::_DescriptorSet, _Device.GetTextures() };
	const uint32 dynamicOffsets[] = { camera.GetDynamicOffset() };

	cmdBuf.BindDescriptorSets(pipeline, std::size(descriptorSets), descriptorSets, std::size(dynamicOffsets), dynamicOffsets);

	for (auto& entity : _ECS.GetEntities<SkyboxComponent>())
	{
		auto& skybox = _ECS.GetComponent<SkyboxComponent>(entity)._Skybox->GetImage();
		const auto skyboxSampler = _Device.CreateSampler({ EFilter::Linear, ESamplerAddressMode::ClampToEdge, ESamplerMipmapMode::Linear });

		SkyboxParams skyboxParams;
		skyboxParams._Skybox = skybox.GetTextureID(skyboxSampler);

		cmdBuf.PushConstants(pipeline, fragShader, &skyboxParams);

		const StaticMesh* cube = _Assets.GetStaticMesh("Cube");

		for (const auto& submesh : cube->_Submeshes)
		{
			cmdBuf.BindVertexBuffers(1, &submesh.GetPositionBuffer());
			cmdBuf.DrawIndexed(submesh.GetIndexBuffer(), submesh.GetIndexCount(), 1, 0, 0, 0, submesh.GetIndexType());
		}
	}

	cmdBuf.EndRenderPass();
}