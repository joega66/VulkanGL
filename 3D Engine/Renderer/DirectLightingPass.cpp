#include "CameraRender.h"
#include "SceneRenderer.h"
#include "ShadowRender.h"
#include <ECS/EntityManager.h>
#include <Components/Light.h>
#include <Components/Transform.h>
#include <Components/SkyboxComponent.h>
#include <Components/Camera.h>
#include <Systems/CameraSystem.h>

BEGIN_PUSH_CONSTANTS(DirectLightingParams)
	MEMBER(glm::vec4, _L)
	MEMBER(glm::vec4, _Radiance)
	MEMBER(glm::mat4, _LightViewProj)
	MEMBER(gpu::TextureID, _ShadowMap)
END_PUSH_CONSTANTS(DirectLightingParams)

class DirectLightingPassCS : public gpu::Shader
{
public:
	static constexpr uint32 directionalLight = 0;
	static constexpr uint32 pointLight = 1;

	DirectLightingPassCS() = default;
};

REGISTER_SHADER(DirectLightingPassCS, "../Shaders/DirectLightingPassCS.glsl", "main", EShaderStage::Compute);

void SceneRenderer::ComputeDirectLighting(CameraRender& camera, gpu::CommandBuffer& cmdBuf)
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
		auto& shadowRender = _ECS.GetComponent<ShadowRender>(entity);
		
		DirectLightingParams light;
		light._L = glm::vec4(transform.GetForward(), 0.0f);
		light._Radiance = glm::vec4(directionalLight._Intensity * directionalLight._Color, 1.0f);
		light._LightViewProj = shadowRender.GetLightViewProjMatrix();
		light._ShadowMap = shadowRender.GetShadowMap().GetTextureID(_Device.CreateSampler({}));

		ComputeDirectLighting(camera, cmdBuf, light, isFirstLight);

		isFirstLight = false;
	}
}

void SceneRenderer::ComputeDirectLighting(CameraRender& camera, gpu::CommandBuffer& cmdBuf, const DirectLightingParams& light, bool isFirstLight)
{
	const gpu::Shader* shader = _Device.FindShader<DirectLightingPassCS>();

	ComputePipelineDesc computeDesc;
	computeDesc.shader = shader;
	computeDesc.specInfo.Add(0, light._L.w == 0.0f ? DirectLightingPassCS::directionalLight : DirectLightingPassCS::pointLight);
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

void SceneRenderer::ComputeSSGI(const Camera& camera, CameraRender& cameraRender, gpu::CommandBuffer& cmdBuf)
{
	const ImageMemoryBarrier startBarrier
	{
		cameraRender._SceneColor,
		EAccess::None,
		EAccess::ShaderWrite,
		EImageLayout::Undefined,
		EImageLayout::General
	};

	cmdBuf.PipelineBarrier(EPipelineStage::TopOfPipe, EPipelineStage::ComputeShader, 0, nullptr, 1, &startBarrier);

	const gpu::Shader* shader = _Device.FindShader<SSGI>();

	ComputePipelineDesc computeDesc;
	computeDesc.shader = shader;

	gpu::Pipeline pipeline = _Device.CreatePipeline(computeDesc);

	cmdBuf.BindPipeline(pipeline);

	const VkDescriptorSet descriptorSets[] = { CameraDescriptors::_DescriptorSet, _Device.GetTextures() };
	const uint32 dynamicOffsets[] = { cameraRender.GetDynamicOffset() };

	cmdBuf.BindDescriptorSets(pipeline, std::size(descriptorSets), descriptorSets, std::size(dynamicOffsets), dynamicOffsets);

	static uint32 frameNumber = 0;

	if (camera.GetPrevPosition() != camera.GetPosition() || camera.GetPrevRotation() != camera.GetRotation())
	{
		frameNumber = 0;
	}

	auto& skybox = _ECS.GetComponent<SkyboxComponent>(_ECS.GetEntities<SkyboxComponent>().front())._Skybox->GetImage();
	const auto skyboxSampler = _Device.CreateSampler({ EFilter::Linear, ESamplerAddressMode::ClampToEdge, ESamplerMipmapMode::Linear });

	SSGIParams ssgiParams;
	ssgiParams._Skybox = skybox.GetTextureID(skyboxSampler);
	ssgiParams._FrameNumber = frameNumber++;

	cmdBuf.PushConstants(pipeline, shader, &ssgiParams);

	const uint32 groupCountX = DivideAndRoundUp(cameraRender._SceneColor.GetWidth(), 8u);
	const uint32 groupCountY = DivideAndRoundUp(cameraRender._SceneColor.GetHeight(), 8u);

	cmdBuf.Dispatch(groupCountX, groupCountY, 1);

	const ImageMemoryBarrier endBarrier
	{
		cameraRender._SceneColor,
		EAccess::ShaderWrite,
		EAccess::ShaderRead,
		EImageLayout::General,
		EImageLayout::General
	};

	cmdBuf.PipelineBarrier(EPipelineStage::ComputeShader, EPipelineStage::ComputeShader, 0, nullptr, 1, &endBarrier);
}