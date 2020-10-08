#include "SceneRenderer.h"
#include <Components/Camera.h>
#include <ECS/EntityManager.h>
#include <Components/SkyboxComponent.h>

BEGIN_PUSH_CONSTANTS(RayTracingParams)
	MEMBER(glm::vec4, _Origin)
	MEMBER(glm::vec4, _Horizontal)
	MEMBER(glm::vec4, _Vertical)
	MEMBER(glm::vec4, _LowerLeftCorner)
	MEMBER(gpu::TextureID, _Skybox)
	MEMBER(gpu::SamplerID, _SkyboxSampler)
	MEMBER(uint32, _FrameNumber)
END_PUSH_CONSTANTS(RayTracingParams)

class RayTracingCS : public gpu::Shader
{
public:
	RayTracingCS(const ShaderCompilationInfo& compilationInfo)
		: gpu::Shader(compilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& worker)
	{
		worker << RayTracingParams::decl;
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo baseInfo = { "../Shaders/RayTracingCS.glsl", "main", EShaderStage::Compute };
		return baseInfo;
	}
};

void SceneRenderer::ComputeRayTracing(const Camera& camera, CameraProxy& cameraProxy, gpu::CommandList& cmdList)
{
	ImageMemoryBarrier imageBarrier
	{
		cameraProxy._SceneColor,
		EAccess::None,
		EAccess::ShaderWrite,
		EImageLayout::Undefined,
		EImageLayout::General
	};

	cmdList.PipelineBarrier(EPipelineStage::TopOfPipe, EPipelineStage::ComputeShader, 0, nullptr, 1, &imageBarrier);

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

	RayTracingParams rayTracingParams;
	rayTracingParams._Origin = glm::vec4(camera.GetPosition(), 0);
	rayTracingParams._Horizontal = glm::vec4(focusDistance * viewportWidth * u, 0);
	rayTracingParams._Vertical = glm::vec4(focusDistance * viewportHeight * v, 0);
	rayTracingParams._LowerLeftCorner = rayTracingParams._Origin - rayTracingParams._Horizontal / 2.0f - rayTracingParams._Vertical / 2.0f - glm::vec4(focusDistance * w, 0);
	rayTracingParams._Skybox = _ECS.GetComponent<SkyboxComponent>(_ECS.GetEntities<SkyboxComponent>().front()).Skybox->GetImage().GetTextureID();
	rayTracingParams._SkyboxSampler = _Device.CreateSampler({ EFilter::Linear, ESamplerAddressMode::ClampToEdge, ESamplerMipmapMode::Linear }).GetSamplerID();
	rayTracingParams._FrameNumber = frameNumber++;

	ComputePipelineDesc computeDesc = {};
	computeDesc.computeShader = _ShaderLibrary.FindShader<RayTracingCS>();

	gpu::Pipeline pipeline = _Device.CreatePipeline(computeDesc);

	cmdList.BindPipeline(pipeline);

	std::vector<VkDescriptorSet> descriptorSets = { cameraProxy._CameraDescriptorSet, _Device.GetTextures(), _Device.GetSamplers() };
	cmdList.BindDescriptorSets(pipeline, descriptorSets.size(), descriptorSets.data());

	cmdList.PushConstants(pipeline, computeDesc.computeShader, &rayTracingParams);

	const uint32 groupCountX = DivideAndRoundUp(camera.GetWidth(), 8u);
	const uint32 groupCountY = DivideAndRoundUp(camera.GetHeight(), 8u);

	cmdList.Dispatch(groupCountX, groupCountY, 1);

	imageBarrier.srcAccessMask = EAccess::ShaderWrite;
	imageBarrier.dstAccessMask = EAccess::ShaderRead;
	imageBarrier.oldLayout = EImageLayout::General;
	imageBarrier.newLayout = EImageLayout::General;

	cmdList.PipelineBarrier(EPipelineStage::ComputeShader, EPipelineStage::ComputeShader, 0, nullptr, 1, &imageBarrier);
}