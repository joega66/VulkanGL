#include "SceneRenderer.h"
#include <Engine/Camera.h>
#include <ECS/EntityManager.h>
#include <Components/SkyboxComponent.h>

BEGIN_SHADER_STRUCT(RayTracingParams)
	SHADER_PARAMETER(glm::vec4, _Origin)
	SHADER_PARAMETER(glm::vec4, _Horizontal)
	SHADER_PARAMETER(glm::vec4, _Vertical)
	SHADER_PARAMETER(glm::vec4, _LowerLeftCorner)
	SHADER_PARAMETER(drm::TextureID, _Skybox)
	SHADER_PARAMETER(drm::SamplerID, _SkyboxSampler)
	SHADER_PARAMETER(uint32, _FrameNumber)
END_SHADER_STRUCT(RayTracingParams)

class RayTracingCS : public drm::Shader
{
public:
	RayTracingCS(const ShaderCompilationInfo& compilationInfo)
		: drm::Shader(compilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& worker)
	{
		worker << RayTracingParams::Decl;
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo baseInfo = { "../Shaders/RayTracingCS.glsl", "main", EShaderStage::Compute };
		return baseInfo;
	}
};

void SceneRenderer::ComputeRayTracing(CameraProxy& camera, drm::CommandList& cmdList)
{
	ImageMemoryBarrier imageBarrier
	{
		camera.SceneColor,
		EAccess::None,
		EAccess::ShaderWrite,
		EImageLayout::Undefined,
		EImageLayout::General
	};

	cmdList.PipelineBarrier(EPipelineStage::TopOfPipe, EPipelineStage::ComputeShader, 0, nullptr, 1, &imageBarrier);

	const float theta = glm::radians(_Camera.GetFOV());
	const float h = glm::tan(theta / 2.0f);
	const float viewportHeight = 2.0f * h;
	const float viewportWidth = _Camera.GetAspectRatio() * viewportHeight;

	const glm::vec3 w = glm::normalize(_Camera.GetForward());
	const glm::vec3 u = glm::normalize(glm::cross(_Camera.GetWorldUp(), w));
	const glm::vec3 v = glm::cross(w, u);

	const float focusDistance = 1.0;

	static uint32 frameNumber = 0;
	static glm::vec3 oldCameraPosition;
	static glm::quat oldCameraRotation;

	if (oldCameraPosition != _Camera.GetPosition() || oldCameraRotation != _Camera.GetRotation())
	{
		frameNumber = 0;
	}

	oldCameraPosition = _Camera.GetPosition();
	oldCameraRotation = _Camera.GetRotation();

	RayTracingParams rayTracingParams;
	rayTracingParams._Origin = glm::vec4(_Camera.GetPosition(), 0);
	rayTracingParams._Horizontal = glm::vec4(focusDistance * viewportWidth * u, 0);
	rayTracingParams._Vertical = glm::vec4(focusDistance * viewportHeight * v, 0);
	rayTracingParams._LowerLeftCorner = rayTracingParams._Origin - rayTracingParams._Horizontal / 2.0f - rayTracingParams._Vertical / 2.0f - glm::vec4(focusDistance * w, 0);
	rayTracingParams._Skybox = ECS.GetComponent<SkyboxComponent>(ECS.GetEntities<SkyboxComponent>().front()).Skybox->GetImage().GetTextureID();
	rayTracingParams._SkyboxSampler = Device.CreateSampler({ EFilter::Linear, ESamplerAddressMode::ClampToEdge, ESamplerMipmapMode::Linear }).GetSamplerID();
	rayTracingParams._FrameNumber = frameNumber++;

	ComputePipelineDesc computeDesc = {};
	computeDesc.ComputeShader = ShaderLibrary.FindShader<RayTracingCS>();
	computeDesc.Layouts = { camera.CameraDescriptorSet.GetLayout(), Device.GetTextures().GetLayout(), Device.GetSamplers().GetLayout() };
	
	drm::Pipeline pipeline = Device.CreatePipeline(computeDesc);

	cmdList.BindPipeline(pipeline);

	std::vector<VkDescriptorSet> descriptorSets = { camera.CameraDescriptorSet, Device.GetTextures().GetSet(), Device.GetSamplers().GetSet() };
	cmdList.BindDescriptorSets(pipeline, descriptorSets.size(), descriptorSets.data());

	cmdList.PushConstants(pipeline, computeDesc.ComputeShader, &rayTracingParams);

	const uint32 groupCountX = DivideAndRoundUp(_Camera.GetWidth(), 8u);
	const uint32 groupCountY = DivideAndRoundUp(_Camera.GetHeight(), 8u);

	cmdList.Dispatch(groupCountX, groupCountY, 1);

	imageBarrier.SrcAccessMask = EAccess::ShaderWrite;
	imageBarrier.DstAccessMask = EAccess::ShaderRead;
	imageBarrier.OldLayout = EImageLayout::General;
	imageBarrier.NewLayout = EImageLayout::General;

	cmdList.PipelineBarrier(EPipelineStage::ComputeShader, EPipelineStage::ComputeShader, 0, nullptr, 1, &imageBarrier);
}