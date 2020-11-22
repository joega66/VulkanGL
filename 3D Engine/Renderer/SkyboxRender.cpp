#include "SceneRenderer.h"
#include <ECS/EntityManager.h>
#include <Engine/AssetManager.h>
#include <Components/SkyboxComponent.h>

class SkyboxVS : public gpu::Shader
{
public:
	SkyboxVS() = default;
	SkyboxVS(const ShaderCompilationInfo& compilationInfo)
		: gpu::Shader(compilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& worker)
	{
	}
};

REGISTER_SHADER(SkyboxVS, "../Shaders/SkyboxVS.glsl", "main", EShaderStage::Vertex);

BEGIN_PUSH_CONSTANTS(SkyboxShaderParams)
	MEMBER(gpu::TextureID, _Skybox)
	MEMBER(gpu::SamplerID, _Sampler)
END_PUSH_CONSTANTS(SkyboxShaderParams)

class SkyboxFS : public gpu::Shader
{
public:
	SkyboxFS() = default;
	SkyboxFS(const ShaderCompilationInfo& compilationInfo)
		: gpu::Shader(compilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& worker)
	{
		worker << SkyboxShaderParams::decl;
	}
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

	const VkDescriptorSet descriptorSets[] = { camera._CameraDescriptorSet, _Device.GetTextures(), _Device.GetSamplers() };
	cmdList.BindDescriptorSets(pipeline, std::size(descriptorSets), descriptorSets);

	for (auto& entity : _ECS.GetEntities<SkyboxComponent>())
	{
		auto& skybox = _ECS.GetComponent<SkyboxComponent>(entity);

		SkyboxShaderParams skyboxShaderParams;
		skyboxShaderParams._Skybox = skybox.Skybox->GetImage().GetTextureID();
		skyboxShaderParams._Sampler = _Device.CreateSampler({ EFilter::Linear, ESamplerAddressMode::ClampToEdge, ESamplerMipmapMode::Linear }).GetSamplerID();

		cmdList.PushConstants(pipeline, fragShader, &skyboxShaderParams);

		const StaticMesh* cube = _Assets.GetStaticMesh("Cube");

		for (const auto& submesh : cube->Submeshes)
		{
			cmdList.BindVertexBuffers(1, &submesh.GetPositionBuffer());
			cmdList.DrawIndexed(submesh.GetIndexBuffer(), submesh.GetIndexCount(), 1, 0, 0, 0, submesh.GetIndexType());
		}
	}

	cmdList.EndRenderPass();
}