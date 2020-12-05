#include "SceneRenderer.h"
#include <Components/Camera.h>
#include <ECS/EntityManager.h>
#include <Components/SkyboxComponent.h>
#include <Systems/CameraSystem.h>

BEGIN_PUSH_CONSTANTS(RayTracingParams)
	MEMBER(glm::vec4, _Origin)
	MEMBER(glm::vec4, _Horizontal)
	MEMBER(glm::vec4, _Vertical)
	MEMBER(glm::vec4, _LowerLeftCorner)
	MEMBER(gpu::TextureID, _Skybox)
	MEMBER(uint32, _FrameNumber)
END_PUSH_CONSTANTS(RayTracingParams)

class RayTracingCS : public gpu::Shader
{
public:
	RayTracingCS() = default;
};

REGISTER_SHADER(RayTracingCS, "../Shaders/RayTracingCS.glsl", "main", EShaderStage::Compute);

void SceneRenderer::ComputeRayTracing(const Camera& camera, CameraRender& cameraRender, gpu::CommandBuffer& cmdBuf)
{
	ImageMemoryBarrier imageBarrier
	{
		cameraRender._SceneColor,
		EAccess::None,
		EAccess::ShaderWrite,
		EImageLayout::Undefined,
		EImageLayout::General
	};

	cmdBuf.PipelineBarrier(EPipelineStage::TopOfPipe, EPipelineStage::ComputeShader, 0, nullptr, 1, &imageBarrier);

	const float theta = glm::radians(camera.GetFieldOfView());
	const float h = glm::tan(theta / 2.0f);
	const float viewportHeight = 2.0f * h;
	const float viewportWidth = camera.GetAspectRatio() * viewportHeight;

	const glm::vec3 w = glm::normalize(camera.GetForward());
	const glm::vec3 u = glm::normalize(glm::cross(camera.GetWorldUp(), w));
	const glm::vec3 v = glm::cross(w, u);

	const float focusDistance = 1.0;

	static uint32 frameNumber = 0;

	if (camera.GetPrevPosition() != camera.GetPosition() || camera.GetPrevRotation() != camera.GetRotation())
	{
		frameNumber = 0;
	}

	auto& skybox = _ECS.GetComponent<SkyboxComponent>(_ECS.GetEntities<SkyboxComponent>().front()).Skybox->GetImage();
	const auto skyboxSampler = _Device.CreateSampler({ EFilter::Linear, ESamplerAddressMode::ClampToEdge, ESamplerMipmapMode::Linear });

	RayTracingParams rayTracingParams;
	rayTracingParams._Origin = glm::vec4(camera.GetPosition(), 0);
	rayTracingParams._Horizontal = glm::vec4(focusDistance * viewportWidth * u, 0);
	rayTracingParams._Vertical = glm::vec4(focusDistance * viewportHeight * v, 0);
	rayTracingParams._LowerLeftCorner = rayTracingParams._Origin - rayTracingParams._Horizontal / 2.0f - rayTracingParams._Vertical / 2.0f - glm::vec4(focusDistance * w, 0);
	rayTracingParams._Skybox = skybox.GetTextureID(skyboxSampler);
	rayTracingParams._FrameNumber = frameNumber++;

	ComputePipelineDesc computeDesc = {};
	computeDesc.computeShader = _Device.FindShader<RayTracingCS>();

	gpu::Pipeline pipeline = _Device.CreatePipeline(computeDesc);

	cmdBuf.BindPipeline(pipeline);

	const VkDescriptorSet descriptorSets[] = { CameraDescriptors::_DescriptorSet, _Device.GetTextures() };
	const uint32 dynamicOffsets[] = { cameraRender.GetDynamicOffset() };

	cmdBuf.BindDescriptorSets(pipeline, std::size(descriptorSets), descriptorSets, std::size(dynamicOffsets), dynamicOffsets);

	cmdBuf.PushConstants(pipeline, computeDesc.computeShader, &rayTracingParams);

	const uint32 groupCountX = DivideAndRoundUp(camera.GetWidth(), 8u);
	const uint32 groupCountY = DivideAndRoundUp(camera.GetHeight(), 8u);

	cmdBuf.Dispatch(groupCountX, groupCountY, 1);

	imageBarrier.srcAccessMask = EAccess::ShaderWrite;
	imageBarrier.dstAccessMask = EAccess::ShaderRead;
	imageBarrier.oldLayout = EImageLayout::General;
	imageBarrier.newLayout = EImageLayout::General;

	cmdBuf.PipelineBarrier(EPipelineStage::ComputeShader, EPipelineStage::ComputeShader, 0, nullptr, 1, &imageBarrier);
}