#include "CameraProxy.h"
#include "SceneRenderer.h"
#include "ShadowProxy.h"
#include <ECS/EntityManager.h>
#include <Components/Light.h>
#include <Components/Transform.h>
#include <Components/SkyboxComponent.h>
#include <Engine/Camera.h>

BEGIN_PUSH_CONSTANTS(LightingParams)
	MEMBER(glm::vec4, _L)
	MEMBER(glm::vec4, _Radiance)
	MEMBER(glm::mat4, _LightViewProj)
	MEMBER(gpu::TextureID, _ShadowMap)
	MEMBER(gpu::SamplerID, _ShadowMapSampler)
END_PUSH_CONSTANTS(LightingParams)

class LightingPassCS : public gpu::Shader
{
public:
	static constexpr uint32 directionalLight = 0;
	static constexpr uint32 pointLight = 1;

	LightingPassCS(const ShaderCompilationInfo& compilationInfo)
		: gpu::Shader(compilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& worker)
	{
		worker << LightingParams::decl;
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo info = { "../Shaders/LightingPassCS.glsl", "main", EShaderStage::Compute };
		return info;
	}
};

void SceneRenderer::ComputeLightingPass(CameraProxy& camera, gpu::CommandList& cmdList)
{
	const ImageMemoryBarrier imageBarrier
	{
		camera._SceneColor,
		EAccess::ShaderRead | EAccess::ShaderWrite,
		EAccess::ShaderRead | EAccess::ShaderWrite,
		EImageLayout::General,
		EImageLayout::General
	};

	for (auto entity : _ECS.GetEntities<DirectionalLight>())
	{
		const auto& directionalLight = _ECS.GetComponent<DirectionalLight>(entity);
		const auto& transform = _ECS.GetComponent<Transform>(entity);
		const auto& shadow = _ECS.GetComponent<ShadowProxy>(entity);
		
		LightingParams light;
		light._L = glm::vec4(transform.GetForward(), 0.0f);
		light._Radiance = glm::vec4(directionalLight.Intensity * directionalLight.Color, 1.0f);
		light._LightViewProj = shadow.GetLightViewProjMatrix();
		light._ShadowMap = shadow.GetShadowMap().GetTextureID();
		light._ShadowMapSampler = _Device.CreateSampler({}).GetSamplerID();

		ComputeDeferredLight(camera, cmdList, light);

		cmdList.PipelineBarrier(EPipelineStage::ComputeShader, EPipelineStage::ComputeShader, 0, nullptr, 1, &imageBarrier);
	}

	/*for (auto entity : _ECS.GetEntities<PointLight>())
	{
		const auto& pointLight = _ECS.GetComponent<PointLight>(entity);
		const auto& transform = _ECS.GetComponent<Transform>(entity);

		LightingParams light;
		light._L = glm::vec4(transform.GetPosition(), 1.0f);
		light._Radiance = glm::vec4(pointLight.Intensity * pointLight.Color, 1.0f);

		ComputeDeferredLight(camera, cmdList, light);

		cmdList.PipelineBarrier(EPipelineStage::ComputeShader, EPipelineStage::ComputeShader, 0, nullptr, 1, &imageBarrier);
	}*/

	ComputeSSGI(camera, cmdList);
}

void SceneRenderer::ComputeDeferredLight(CameraProxy& camera, gpu::CommandList& cmdList, const LightingParams& light)
{
	const gpu::Shader* shader = _ShaderLibrary.FindShader<LightingPassCS>();

	ComputePipelineDesc computeDesc;
	computeDesc.computeShader = shader;
	computeDesc.specInfo.Add(0, light._L.w == 0.0f ? LightingPassCS::directionalLight : LightingPassCS::pointLight);

	gpu::Pipeline pipeline = _Device.CreatePipeline(computeDesc);

	cmdList.BindPipeline(pipeline);

	const std::vector<VkDescriptorSet> descriptorSets =
	{
		camera._CameraDescriptorSet,
		_Device.GetTextures().GetSet(),
		_Device.GetSamplers().GetSet(),
	};

	cmdList.BindDescriptorSets(pipeline, descriptorSets.size(), descriptorSets.data());

	cmdList.PushConstants(pipeline, shader, &light);

	const uint32 groupCountX = DivideAndRoundUp(camera._SceneColor.GetWidth(), 8u);
	const uint32 groupCountY = DivideAndRoundUp(camera._SceneColor.GetHeight(), 8u);

	cmdList.Dispatch(groupCountX, groupCountY, 1);
}

BEGIN_PUSH_CONSTANTS(SSGIParams)
	MEMBER(gpu::TextureID, _Skybox)
	MEMBER(gpu::SamplerID, _SkyboxSampler)
	MEMBER(uint32, _FrameNumber)
END_PUSH_CONSTANTS(SSGIParams)

class SSGI : public gpu::Shader
{
public:
	SSGI(const ShaderCompilationInfo& compilationInfo)
		: gpu::Shader(compilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& worker)
	{
		worker << SSGIParams::decl;
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo baseInfo = { "../Shaders/SSGI.glsl", "main", EShaderStage::Compute };
		return baseInfo;
	}
};

void SceneRenderer::ComputeSSGI(CameraProxy& camera, gpu::CommandList& cmdList)
{
	const gpu::Shader* shader = _ShaderLibrary.FindShader<SSGI>();

	ComputePipelineDesc computeDesc;
	computeDesc.computeShader = shader;

	gpu::Pipeline pipeline = _Device.CreatePipeline(computeDesc);

	cmdList.BindPipeline(pipeline);

	const std::vector<VkDescriptorSet> descriptorSets =
	{
		camera._CameraDescriptorSet,
		_Device.GetTextures().GetSet(),
		_Device.GetSamplers().GetSet(),
	};

	cmdList.BindDescriptorSets(pipeline, descriptorSets.size(), descriptorSets.data());

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
	ssgiParams._Skybox = _ECS.GetComponent<SkyboxComponent>(_ECS.GetEntities<SkyboxComponent>().front()).Skybox->GetImage().GetTextureID();
	ssgiParams._SkyboxSampler = _Device.CreateSampler({ EFilter::Linear, ESamplerAddressMode::ClampToEdge, ESamplerMipmapMode::Linear }).GetSamplerID();
	ssgiParams._FrameNumber = frameNumber++;

	cmdList.PushConstants(pipeline, shader, &ssgiParams);

	const uint32 groupCountX = DivideAndRoundUp(camera._SceneColor.GetWidth(), 8u);
	const uint32 groupCountY = DivideAndRoundUp(camera._SceneColor.GetHeight(), 8u);

	cmdList.Dispatch(groupCountX, groupCountY, 1);

	const ImageMemoryBarrier barrier
	{
		camera._SceneColor,
		EAccess::ShaderRead | EAccess::ShaderWrite,
		EAccess::ColorAttachmentRead | EAccess::ColorAttachmentWrite,
		EImageLayout::General,
		EImageLayout::General
	};

	cmdList.PipelineBarrier(EPipelineStage::ComputeShader, EPipelineStage::ColorAttachmentOutput, 0, nullptr, 1, &barrier);
}