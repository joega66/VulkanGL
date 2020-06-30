#include "CameraProxy.h"
#include "SceneRenderer.h"
#include "ShadowProxy.h"
#include <ECS/EntityManager.h>
#include <Components/Light.h>
#include <Components/Transform.h>
#include <Components/SkyboxComponent.h>
#include <Engine/Camera.h>

BEGIN_SHADER_STRUCT(LightData)
	SHADER_PARAMETER(glm::vec4, _L)
	SHADER_PARAMETER(glm::vec4, _Radiance)
	SHADER_PARAMETER(glm::mat4, _LightViewProj)
	SHADER_PARAMETER(gpu::TextureID, _ShadowMap)
	SHADER_PARAMETER(gpu::SamplerID, _ShadowMapSampler)
END_SHADER_STRUCT(LightData)

class LightingPassCS : public gpu::Shader
{
public:
	static constexpr uint32 DirectionalLight = 0;
	static constexpr uint32 PointLight = 1;

	LightingPassCS(const ShaderCompilationInfo& CompilationInfo)
		: gpu::Shader(CompilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		Worker << LightData::Decl;
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo BaseInfo = { "../Shaders/LightingPassCS.glsl", "main", EShaderStage::Compute };
		return BaseInfo;
	}
};

void SceneRenderer::ComputeLightingPass(CameraProxy& Camera, gpu::CommandList& CmdList)
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

	ComputeSSGI(Camera, CmdList);
}

void SceneRenderer::ComputeDeferredLight(CameraProxy& Camera, gpu::CommandList& CmdList, const LightData& Light)
{
	const gpu::Shader* Shader = ShaderLibrary.FindShader<LightingPassCS>();

	ComputePipelineDesc ComputeDesc;
	ComputeDesc.computeShader = Shader;
	ComputeDesc.specInfo.Add(0, Light._L.w == 0.0f ? LightingPassCS::DirectionalLight : LightingPassCS::PointLight);
	ComputeDesc.Layouts =
	{
		Camera.CameraDescriptorSet.GetLayout(),
		Device.GetTextures().GetLayout(),
		Device.GetSamplers().GetLayout(),
	};

	gpu::Pipeline Pipeline = Device.CreatePipeline(ComputeDesc);

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

BEGIN_SHADER_STRUCT(SSGIParams)
	SHADER_PARAMETER(gpu::TextureID, _Skybox)
	SHADER_PARAMETER(gpu::SamplerID, _SkyboxSampler)
	SHADER_PARAMETER(uint32, _FrameNumber)
END_SHADER_STRUCT(SSGIParams)

class SSGI : public gpu::Shader
{
public:
	SSGI(const ShaderCompilationInfo& compilationInfo)
		: gpu::Shader(compilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& worker)
	{
		worker << SSGIParams::Decl;
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo baseInfo = { "../Shaders/SSGI.glsl", "main", EShaderStage::Compute };
		return baseInfo;
	}
};

void SceneRenderer::ComputeSSGI(CameraProxy& camera, gpu::CommandList& cmdList)
{
	const gpu::Shader* shader = ShaderLibrary.FindShader<SSGI>();

	ComputePipelineDesc computeDesc;
	computeDesc.computeShader = shader;
	computeDesc.Layouts =
	{
		camera.CameraDescriptorSet.GetLayout(),
		Device.GetTextures().GetLayout(),
		Device.GetSamplers().GetLayout(),
	};

	gpu::Pipeline pipeline = Device.CreatePipeline(computeDesc);

	cmdList.BindPipeline(pipeline);

	const std::vector<VkDescriptorSet> descriptorSets =
	{
		camera.CameraDescriptorSet,
		Device.GetTextures().GetSet(),
		Device.GetSamplers().GetSet(),
	};

	cmdList.BindDescriptorSets(pipeline, static_cast<uint32>(descriptorSets.size()), descriptorSets.data());

	static uint32 frameNumber = 0;
	static glm::vec3 oldCameraPosition;
	static glm::quat oldCameraRotation;

	if (oldCameraPosition != _Camera.GetPosition() || oldCameraRotation != _Camera.GetRotation())
	{
		frameNumber = 0;
	}

	oldCameraPosition = _Camera.GetPosition();
	oldCameraRotation = _Camera.GetRotation();

	SSGIParams ssgiParams;
	ssgiParams._Skybox = ECS.GetComponent<SkyboxComponent>(ECS.GetEntities<SkyboxComponent>().front()).Skybox->GetImage().GetTextureID();
	ssgiParams._SkyboxSampler = Device.CreateSampler({ EFilter::Linear, ESamplerAddressMode::ClampToEdge, ESamplerMipmapMode::Linear }).GetSamplerID();
	ssgiParams._FrameNumber = frameNumber++;

	cmdList.PushConstants(pipeline, shader, &ssgiParams);

	const uint32 groupCountX = DivideAndRoundUp(camera.SceneColor.GetWidth(), 8u);
	const uint32 groupCountY = DivideAndRoundUp(camera.SceneColor.GetHeight(), 8u);

	cmdList.Dispatch(groupCountX, groupCountY, 1);

	const ImageMemoryBarrier barrier
	{
		camera.SceneColor,
		EAccess::ShaderRead | EAccess::ShaderWrite,
		EAccess::ColorAttachmentRead | EAccess::ColorAttachmentWrite,
		EImageLayout::General,
		EImageLayout::General
	};

	cmdList.PipelineBarrier(EPipelineStage::ComputeShader, EPipelineStage::ColorAttachmentOutput, 0, nullptr, 1, &barrier);
}