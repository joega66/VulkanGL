#include "SceneRenderer.h"
#include <ECS/EntityManager.h>
#include <Engine/AssetManager.h>
#include <Components/SkyboxComponent.h>

class SkyboxVS : public gpu::Shader
{
public:
	SkyboxVS(const ShaderCompilationInfo& CompilationInfo)
		: gpu::Shader(CompilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo Base = { "../Shaders/SkyboxVS.glsl", "main", EShaderStage::Vertex };
		return Base;
	}
};

class SkyboxFS : public gpu::Shader
{
public:
	SkyboxFS(const ShaderCompilationInfo& CompilationInfo)
		: gpu::Shader(CompilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo Base = { "../Shaders/SkyboxFS.glsl", "main", EShaderStage::Fragment };
		return Base;
	}
};

void SceneRenderer::RenderSkybox(CameraProxy& Camera, gpu::CommandList& CmdList)
{
	const SkyboxVS* VertShader = ShaderLibrary.FindShader<SkyboxVS>();
	const SkyboxFS* FragShader = ShaderLibrary.FindShader<SkyboxFS>();

	struct PushConstants
	{
		gpu::TextureID Skybox;
		gpu::SamplerID Sampler;
	};
	
	PipelineStateDesc PSODesc = {};
	PSODesc.renderPass = Camera.SceneRP;
	PSODesc.depthStencilState.depthTestEnable = true;
	PSODesc.depthStencilState.depthWriteEnable = true;
	PSODesc.viewport.width = Camera.SceneColor.GetWidth();
	PSODesc.viewport.height = Camera.SceneColor.GetHeight();
	PSODesc.shaderStages = { VertShader, nullptr, nullptr, nullptr, FragShader };
	PSODesc.layouts = { Camera.CameraDescriptorSet.GetLayout(), Device.GetTextures().GetLayout(), Device.GetSamplers().GetLayout() };
	PSODesc.pushConstantRanges.push_back({ EShaderStage::Fragment, 0, sizeof(PushConstants) });

	gpu::Pipeline Pipeline = Device.CreatePipeline(PSODesc);

	CmdList.BindPipeline(Pipeline);

	const std::vector<VkDescriptorSet> DescriptorSets = { Camera.CameraDescriptorSet, Device.GetTextures().GetSet(), Device.GetSamplers().GetSet() };
	CmdList.BindDescriptorSets(Pipeline, static_cast<uint32>(DescriptorSets.size()), DescriptorSets.data());

	for (auto& Entity : ECS.GetEntities<SkyboxComponent>())
	{
		auto& Skybox = ECS.GetComponent<SkyboxComponent>(Entity);

		PushConstants PushConstants;
		PushConstants.Skybox = Skybox.Skybox->GetImage().GetTextureID();
		PushConstants.Sampler = Device.CreateSampler({ EFilter::Linear, ESamplerAddressMode::ClampToEdge, ESamplerMipmapMode::Linear }).GetSamplerID();

		CmdList.PushConstants(Pipeline, EShaderStage::Fragment, 0, sizeof(PushConstants), &PushConstants);

		const StaticMesh* Cube = Assets.GetStaticMesh("Cube");

		for (const auto& Submesh : Cube->Submeshes)
		{
			CmdList.BindVertexBuffers(1, &Submesh.GetPositionBuffer());
			CmdList.DrawIndexed(Submesh.GetIndexBuffer(), Submesh.GetIndexCount(), 1, 0, 0, 0, Submesh.GetIndexType());
		}
	}
}