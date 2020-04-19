#include "SceneRenderer.h"
#include <ECS/EntityManager.h>
#include <Engine/AssetManager.h>
#include <Components/SkyboxComponent.h>

class SkyboxVS : public drm::Shader
{
public:
	SkyboxVS(const ShaderCompilationInfo& CompilationInfo)
		: drm::Shader(CompilationInfo)
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

class SkyboxFS : public drm::Shader
{
public:
	SkyboxFS(const ShaderCompilationInfo& CompilationInfo)
		: drm::Shader(CompilationInfo)
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

struct SkyboxDescriptors
{
	drm::DescriptorImageInfo Skybox;

	static const std::vector<DescriptorBinding>& GetBindings()
	{
		static std::vector<DescriptorBinding> Bindings =
		{
			{ 0, 1, EDescriptorType::SampledImage }
		};
		return Bindings;
	}
};

void SceneRenderer::RenderSkybox(CameraProxy& Camera, drm::CommandList& CmdList)
{
	drm::DescriptorSetLayout SkyboxLayout = Device.CreateDescriptorSetLayout(SkyboxDescriptors::GetBindings().size(), SkyboxDescriptors::GetBindings().data());

	const SkyboxVS* VertShader = ShaderMap.FindShader<SkyboxVS>();
	const SkyboxFS* FragShader = ShaderMap.FindShader<SkyboxFS>();

	PipelineStateDesc PSODesc = {};
	PSODesc.RenderPass = Camera.SceneRP;
	PSODesc.DepthStencilState.DepthTestEnable = true;
	PSODesc.DepthStencilState.DepthWriteEnable = true;
	PSODesc.Viewport.Width = Camera.SceneColor.GetWidth();
	PSODesc.Viewport.Height = Camera.SceneColor.GetHeight();
	PSODesc.ShaderStages = { VertShader, nullptr, nullptr, nullptr, FragShader };
	PSODesc.Layouts = { Camera.CameraDescriptorSet.GetLayout(), SkyboxLayout };

	std::shared_ptr<drm::Pipeline> Pipeline = Device.CreatePipeline(PSODesc);

	CmdList.BindPipeline(Pipeline);

	for (auto& Entity : ECS.GetEntities<SkyboxComponent>())
	{
		auto& Skybox = ECS.GetComponent<SkyboxComponent>(Entity);

		SkyboxDescriptors Descriptors;
		Descriptors.Skybox = drm::DescriptorImageInfo(Skybox.Skybox->GetImage(), Device.CreateSampler({ EFilter::Linear, ESamplerAddressMode::ClampToEdge, ESamplerMipmapMode::Linear }));

		drm::DescriptorSet DescriptorSet = SkyboxLayout.CreateDescriptorSet(Device);
		SkyboxLayout.UpdateDescriptorSet(Device, DescriptorSet, &Descriptors);

		const std::vector<VkDescriptorSet> DescriptorSets = { Camera.CameraDescriptorSet, DescriptorSet };
		CmdList.BindDescriptorSets(Pipeline, static_cast<uint32>(DescriptorSets.size()), DescriptorSets.data());

		const StaticMesh* Cube = Assets.GetStaticMesh("Cube");

		for (const auto& Submesh : Cube->Submeshes)
		{
			CmdList.BindVertexBuffers(1, &Submesh.GetPositionBuffer());
			CmdList.DrawIndexed(Submesh.GetIndexBuffer(), Submesh.GetIndexCount(), 1, 0, 0, 0, Submesh.GetIndexType());
		}
	}
}