#include "CameraProxy.h"
#include "SceneRenderer.h"
#include "ShadowProxy.h"
#include <ECS/EntityManager.h>
#include <Components/Light.h>
#include <Components/Transform.h>
#include <Components/SkyboxComponent.h>
#include <Components/Camera.h>
#include <Systems/CameraSystem.h>

BEGIN_PUSH_CONSTANTS(LightingParams)
	MEMBER(glm::vec4, _L)
	MEMBER(glm::vec4, _Radiance)
	MEMBER(glm::mat4, _LightViewProj)
	MEMBER(gpu::TextureID, _ShadowMap)
END_PUSH_CONSTANTS(LightingParams)

class LightingPassCS : public gpu::Shader
{
public:
	static constexpr uint32 directionalLight = 0;
	static constexpr uint32 pointLight = 1;

	LightingPassCS() = default;
};

REGISTER_SHADER(LightingPassCS, "../Shaders/LightingPassCS.glsl", "main", EShaderStage::Compute);

void SceneRenderer::ComputeLightingPass(CameraProxy& camera, gpu::CommandBuffer& cmdBuf)
{
	const ImageMemoryBarrier imageBarrier
	{
		camera._DirectLighting,
		EAccess::None,
		EAccess::ShaderRead | EAccess::ShaderWrite,
		EImageLayout::Undefined,
		EImageLayout::General
	};

	cmdBuf.PipelineBarrier(EPipelineStage::TopOfPipe, EPipelineStage::ComputeShader, 0, nullptr, 1, &imageBarrier);

	bool isFirstLight = true;

	for (auto entity : _ECS.GetEntities<DirectionalLight>())
	{
		const auto& directionalLight = _ECS.GetComponent<DirectionalLight>(entity);
		const auto& transform = _ECS.GetComponent<Transform>(entity);
		auto& shadow = _ECS.GetComponent<ShadowProxy>(entity);
		
		LightingParams light;
		light._L = glm::vec4(transform.GetForward(), 0.0f);
		light._Radiance = glm::vec4(directionalLight.Intensity * directionalLight.Color, 1.0f);
		light._LightViewProj = shadow.GetLightViewProjMatrix();
		light._ShadowMap = shadow.GetShadowMap().GetTextureID(_Device.CreateSampler({}));

		ComputeDeferredLight(camera, cmdBuf, light, isFirstLight);

		isFirstLight = false;
	}
}

void SceneRenderer::ComputeDeferredLight(CameraProxy& camera, gpu::CommandBuffer& cmdBuf, const LightingParams& light, bool isFirstLight)
{
	const gpu::Shader* shader = _ShaderLibrary.FindShader<LightingPassCS>();

	ComputePipelineDesc computeDesc;
	computeDesc.computeShader = shader;
	computeDesc.specInfo.Add(0, light._L.w == 0.0f ? LightingPassCS::directionalLight : LightingPassCS::pointLight);
	computeDesc.specInfo.Add(1, static_cast<int>(isFirstLight));

	gpu::Pipeline pipeline = _Device.CreatePipeline(computeDesc);

	cmdBuf.BindPipeline(pipeline);

	const VkDescriptorSet descriptorSets[] = { CameraDescriptors::_DescriptorSet, _Device.GetTextures() };
	const uint32 dynamicOffsets[] = { camera.GetDynamicOffset() };

	cmdBuf.BindDescriptorSets(pipeline, std::size(descriptorSets), descriptorSets, std::size(dynamicOffsets), dynamicOffsets);

	cmdBuf.PushConstants(pipeline, shader, &light);

	const uint32 groupCountX = DivideAndRoundUp(camera._DirectLighting.GetWidth(), 8u);
	const uint32 groupCountY = DivideAndRoundUp(camera._DirectLighting.GetHeight(), 8u);

	cmdBuf.Dispatch(groupCountX, groupCountY, 1);
}

BEGIN_PUSH_CONSTANTS(SSGIParams)
	MEMBER(gpu::TextureID, _Skybox)
	MEMBER(uint32, _FrameNumber)
END_PUSH_CONSTANTS(SSGIParams)

class SSGI : public gpu::Shader
{
public:
	SSGI() = default;
};

REGISTER_SHADER(SSGI, "../Shaders/SSGI.glsl", "main", EShaderStage::Compute);

void SceneRenderer::ComputeSSGI(const Camera& camera, CameraProxy& cameraProxy, gpu::CommandBuffer& cmdBuf)
{
	const ImageMemoryBarrier startBarrier
	{
		cameraProxy._SceneColor,
		EAccess::None,
		EAccess::ShaderWrite,
		EImageLayout::Undefined,
		EImageLayout::General
	};

	cmdBuf.PipelineBarrier(EPipelineStage::TopOfPipe, EPipelineStage::ComputeShader, 0, nullptr, 1, &startBarrier);

	const gpu::Shader* shader = _ShaderLibrary.FindShader<SSGI>();

	ComputePipelineDesc computeDesc;
	computeDesc.computeShader = shader;

	gpu::Pipeline pipeline = _Device.CreatePipeline(computeDesc);

	cmdBuf.BindPipeline(pipeline);

	const VkDescriptorSet descriptorSets[] = { CameraDescriptors::_DescriptorSet, _Device.GetTextures() };
	const uint32 dynamicOffsets[] = { cameraProxy.GetDynamicOffset() };

	cmdBuf.BindDescriptorSets(pipeline, std::size(descriptorSets), descriptorSets, std::size(dynamicOffsets), dynamicOffsets);

	static uint32 frameNumber = 0;

	if (camera.GetPrevPosition() != camera.GetPosition() || camera.GetPrevRotation() != camera.GetRotation())
	{
		frameNumber = 0;
	}

	auto& skybox = _ECS.GetComponent<SkyboxComponent>(_ECS.GetEntities<SkyboxComponent>().front()).Skybox->GetImage();
	const auto skyboxSampler = _Device.CreateSampler({ EFilter::Linear, ESamplerAddressMode::ClampToEdge, ESamplerMipmapMode::Linear });

	SSGIParams ssgiParams;
	ssgiParams._Skybox = skybox.GetTextureID(skyboxSampler);
	ssgiParams._FrameNumber = frameNumber++;

	cmdBuf.PushConstants(pipeline, shader, &ssgiParams);

	const uint32 groupCountX = DivideAndRoundUp(cameraProxy._SceneColor.GetWidth(), 8u);
	const uint32 groupCountY = DivideAndRoundUp(cameraProxy._SceneColor.GetHeight(), 8u);

	cmdBuf.Dispatch(groupCountX, groupCountY, 1);

	const ImageMemoryBarrier endBarrier
	{
		cameraProxy._SceneColor,
		EAccess::ShaderWrite,
		EAccess::ShaderRead,
		EImageLayout::General,
		EImageLayout::General
	};

	cmdBuf.PipelineBarrier(EPipelineStage::ComputeShader, EPipelineStage::ComputeShader, 0, nullptr, 1, &endBarrier);
}