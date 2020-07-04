#include "SceneRenderer.h"
#include <ECS/EntityManager.h>
#include <Engine/AssetManager.h>
#include <Components/SkyboxComponent.h>

class SkyboxVS : public gpu::Shader
{
public:
	SkyboxVS(const ShaderCompilationInfo& compilationInfo)
		: gpu::Shader(compilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& worker)
	{
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo base = { "../Shaders/SkyboxVS.glsl", "main", EShaderStage::Vertex };
		return base;
	}
};

BEGIN_SHADER_STRUCT(SkyboxShaderParams)
	SHADER_PARAMETER(gpu::TextureID, _Skybox)
	SHADER_PARAMETER(gpu::SamplerID, _Sampler)
END_SHADER_STRUCT(SkyboxShaderParams)

class SkyboxFS : public gpu::Shader
{
public:
	SkyboxFS(const ShaderCompilationInfo& compilationInfo)
		: gpu::Shader(compilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& worker)
	{
		worker << SkyboxShaderParams::decl;
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo base = { "../Shaders/SkyboxFS.glsl", "main", EShaderStage::Fragment };
		return base;
	}
};

void SceneRenderer::RenderSkybox(CameraProxy& camera, gpu::CommandList& cmdList)
{
	cmdList.BeginRenderPass(camera._SceneRP);

	const SkyboxVS* vertShader = _ShaderLibrary.FindShader<SkyboxVS>();
	const SkyboxFS* fragShader = _ShaderLibrary.FindShader<SkyboxFS>();

	PipelineStateDesc psoDesc = {};
	psoDesc.renderPass = camera._SceneRP;
	psoDesc.depthStencilState.depthTestEnable = true;
	psoDesc.depthStencilState.depthWriteEnable = true;
	psoDesc.viewport.width = camera._SceneColor.GetWidth();
	psoDesc.viewport.height = camera._SceneColor.GetHeight();
	psoDesc.shaderStages = { vertShader, nullptr, nullptr, nullptr, fragShader };

	gpu::Pipeline pipeline = _Device.CreatePipeline(psoDesc);

	cmdList.BindPipeline(pipeline);

	const std::vector<VkDescriptorSet> descriptorSets = { camera._CameraDescriptorSet, _Device.GetTextures().GetSet(), _Device.GetSamplers().GetSet() };
	cmdList.BindDescriptorSets(pipeline, descriptorSets.size(), descriptorSets.data());

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