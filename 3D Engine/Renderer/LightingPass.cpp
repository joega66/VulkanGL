#include "CameraProxy.h"
#include "SceneRenderer.h"
#include "Voxels.h"
#include "ShadowProxy.h"
#include <ECS/EntityManager.h>
#include <Components/Light.h>
#include <Components/Transform.h>

BEGIN_SHADER_STRUCT(LightData)
	SHADER_PARAMETER(glm::vec4, _L)
	SHADER_PARAMETER(glm::vec4, _Radiance)
	SHADER_PARAMETER(glm::mat4, _LightViewProj)
	SHADER_PARAMETER(drm::TextureID, _ShadowMap)
	SHADER_PARAMETER(drm::SamplerID, _ShadowMapSampler)
END_SHADER_STRUCT(LightData)

class LightingPassCS : public drm::Shader
{
public:
	static constexpr uint32 DirectionalLight = 0;
	static constexpr uint32 PointLight = 1;

	LightingPassCS(const ShaderCompilationInfo& CompilationInfo)
		: drm::Shader(CompilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		VoxelShader::SetEnvironmentVariables(Worker);
		Worker << LightData::Decl;
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo BaseInfo = { "../Shaders/LightingPassCS.glsl", "main", EShaderStage::Compute };
		return BaseInfo;
	}
};

void SceneRenderer::ComputeLightingPass(CameraProxy& Camera, drm::CommandList& CmdList)
{
	const ImageMemoryBarrier ImageBarrier
	{
		Camera.SceneColor,
		EAccess::ShaderRead | EAccess::ShaderWrite,
		EAccess::ShaderRead | EAccess::ShaderWrite,
		EImageLayout::General,
		EImageLayout::General
	};

	for (auto Entity : ECS.GetEntities<DirectionalLight>())
	{
		const auto& DirectionalLight = ECS.GetComponent<struct DirectionalLight>(Entity);
		const auto& Shadow = ECS.GetComponent<ShadowProxy>(Entity);

		LightData Light;
		Light._L = glm::vec4(glm::normalize(DirectionalLight.Direction), 0.0f);
		Light._Radiance = glm::vec4(DirectionalLight.Intensity * DirectionalLight.Color, 1.0f);
		Light._LightViewProj = Shadow.GetLightViewProjMatrix();
		Light._ShadowMap = Shadow.GetShadowMap().GetTextureID();
		Light._ShadowMapSampler = Device.CreateSampler({}).GetSamplerID();

		ComputeDeferredLight(Camera, CmdList, Light);

		CmdList.PipelineBarrier(EPipelineStage::ComputeShader, EPipelineStage::ComputeShader, 0, nullptr, 1, &ImageBarrier);
	}

	for (auto Entity : ECS.GetEntities<PointLight>())
	{
		const auto& PointLight = ECS.GetComponent<struct PointLight>(Entity);
		const auto& LightTransform = ECS.GetComponent<Transform>(Entity);

		LightData Light;
		Light._L = glm::vec4(LightTransform.GetPosition(), 1.0f);
		Light._Radiance = glm::vec4(PointLight.Intensity * PointLight.Color, 1.0f);

		ComputeDeferredLight(Camera, CmdList, Light);

		CmdList.PipelineBarrier(EPipelineStage::ComputeShader, EPipelineStage::ComputeShader, 0, nullptr, 1, &ImageBarrier);
	}
}

void SceneRenderer::ComputeDeferredLight(CameraProxy& Camera, drm::CommandList& CmdList, const LightData& Light)
{
	const drm::Shader* Shader = ShaderLibrary.FindShader<LightingPassCS>();

	ComputePipelineDesc ComputeDesc;
	ComputeDesc.ComputeShader = Shader;
	ComputeDesc.SpecializationInfo.Add(0, Light._L.w == 0.0f ? LightingPassCS::DirectionalLight : LightingPassCS::PointLight);
	ComputeDesc.Layouts =
	{
		Camera.CameraDescriptorSet.GetLayout(),
		Device.GetTextures().GetLayout(),
		Device.GetSamplers().GetLayout(),
	};

	drm::Pipeline Pipeline = Device.CreatePipeline(ComputeDesc);

	CmdList.BindPipeline(Pipeline);

	const std::vector<VkDescriptorSet> DescriptorSets =
	{
		Camera.CameraDescriptorSet,
		Device.GetTextures().GetSet(),
		Device.GetSamplers().GetSet(),
	};

	CmdList.BindDescriptorSets(Pipeline, static_cast<uint32>(DescriptorSets.size()), DescriptorSets.data());

	CmdList.PushConstants(Pipeline, Shader, &Light);

	const uint32 GroupCountX = DivideAndRoundUp(Camera.SceneColor.GetWidth(), 8u);
	const uint32 GroupCountY = DivideAndRoundUp(Camera.SceneColor.GetHeight(), 8u);

	CmdList.Dispatch(GroupCountX, GroupCountY, 1);
}

class IndirectLightingCS : public drm::Shader
{
public:
	IndirectLightingCS(const ShaderCompilationInfo& CompilationInfo)
		: drm::Shader(CompilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		VoxelShader::SetEnvironmentVariables(Worker);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo BaseInfo = { "../Shaders/IndirectLightingCS.glsl", "main", EShaderStage::Compute };
		return BaseInfo;
	}
};

void SceneRenderer::ComputeIndirectLightingPass(CameraProxy& Camera, drm::CommandList& CmdList)
{
	auto& VCTLighting = ECS.GetSingletonComponent<VCTLightingCache>();

	ComputePipelineDesc ComputeDesc;
	ComputeDesc.ComputeShader = ShaderLibrary.FindShader<IndirectLightingCS>();
	ComputeDesc.Layouts =
	{
		Camera.CameraDescriptorSet.GetLayout(),
		VCTLighting.GetDescriptorSet().GetLayout()
	};

	drm::Pipeline Pipeline = Device.CreatePipeline(ComputeDesc);

	CmdList.BindPipeline(Pipeline);

	const std::vector<VkDescriptorSet> DescriptorSets =
	{
		Camera.CameraDescriptorSet,
		VCTLighting.GetDescriptorSet(),
	};

	CmdList.BindDescriptorSets(Pipeline, static_cast<uint32>(DescriptorSets.size()), DescriptorSets.data());

	const uint32 GroupCountX = DivideAndRoundUp(Camera.SceneColor.GetWidth(), 8u);
	const uint32 GroupCountY = DivideAndRoundUp(Camera.SceneColor.GetHeight(), 8u);

	CmdList.Dispatch(GroupCountX, GroupCountY, 1);
}