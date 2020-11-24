#include "SceneRenderer.h"
#include <ECS/EntityManager.h>
#include <Engine/AssetManager.h>
#include <Components/SkyboxComponent.h>

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

void SceneRenderer::RenderSkybox(CameraProxy& camera, gpu::CommandList& cmdList)
{
	cmdList.BeginRenderPass(camera._SkyboxRP);

	const SkyboxVS* vertShader = _ShaderLibrary.FindShader<SkyboxVS>();
	const SkyboxFS* fragShader = _ShaderLibrary.FindShader<SkyboxFS>();

	PipelineStateDesc psoDesc = {};
	psoDesc.renderPass = camera._SkyboxRP;
	psoDesc.depthStencilState.depthTestEnable = true;
	psoDesc.depthStencilState.depthWriteEnable = true;
	psoDesc.viewport.width = camera._SceneColor.GetWidth();
	psoDesc.viewport.height = camera._SceneColor.GetHeight();
	psoDesc.shaderStages = { vertShader, nullptr, nullptr, nullptr, fragShader };

	gpu::Pipeline pipeline = _Device.CreatePipeline(psoDesc);

	cmdList.BindPipeline(pipeline);

	const VkDescriptorSet descriptorSets[] = { camera._CameraDescriptorSet, _Device.GetTextures() };
	cmdList.BindDescriptorSets(pipeline, std::size(descriptorSets), descriptorSets);

	for (auto& entity : _ECS.GetEntities<SkyboxComponent>())
	{
		auto& skybox = _ECS.GetComponent<SkyboxComponent>(entity).Skybox->GetImage();
		const auto skyboxSampler = _Device.CreateSampler({ EFilter::Linear, ESamplerAddressMode::ClampToEdge, ESamplerMipmapMode::Linear });

		SkyboxParams skyboxParams;
		skyboxParams._Skybox = skybox.GetTextureID(skyboxSampler);

		cmdList.PushConstants(pipeline, fragShader, &skyboxParams);

		const StaticMesh* cube = _Assets.GetStaticMesh("Cube");

		for (const auto& submesh : cube->Submeshes)
		{
			cmdList.BindVertexBuffers(1, &submesh.GetPositionBuffer());
			cmdList.DrawIndexed(submesh.GetIndexBuffer(), submesh.GetIndexCount(), 1, 0, 0, 0, submesh.GetIndexType());
		}
	}

	cmdList.EndRenderPass();
}