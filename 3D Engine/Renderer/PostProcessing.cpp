#include "SceneRenderer.h"
#include <ECS/EntityManager.h>
#include <Components/RenderSettings.h>

BEGIN_PUSH_CONSTANTS(PostProcessingParams)
	MEMBER(float, _ExposureAdjustment)
	MEMBER(float, _ExposureBias)
	MEMBER(gpu::ImageID, _DisplayColor)
	MEMBER(gpu::ImageID, _HDRColor)
END_PUSH_CONSTANTS(PostProcessingParams)

class PostProcessingCS : public gpu::Shader
{
public:
	PostProcessingCS() = default;
};

REGISTER_SHADER(PostProcessingCS, "../Shaders/PostProcessingCS.glsl", "main", EShaderStage::Compute);

void SceneRenderer::ComputePostProcessing(const gpu::Image& displayImage, CameraRender& camera, gpu::CommandBuffer& cmdBuf)
{
	auto& settings = _ECS.GetSingletonComponent<RenderSettings>();

	const gpu::Shader* shader = _Device.FindShader<PostProcessingCS>();

	ComputePipelineDesc computeDesc;
	computeDesc.shader = shader;

	gpu::Pipeline pipeline = _Device.CreatePipeline(computeDesc);

	cmdBuf.BindPipeline(pipeline);

	cmdBuf.BindDescriptorSets(pipeline, 1, &_Device.GetImages(), 0, nullptr);

	PostProcessingParams postProcessingParams;
	postProcessingParams._ExposureAdjustment = settings.ExposureAdjustment;
	postProcessingParams._ExposureBias = settings.ExposureBias;
	postProcessingParams._DisplayColor = displayImage.GetImageID();
	postProcessingParams._HDRColor = camera._SceneColor.GetImageID();

	cmdBuf.PushConstants(pipeline, shader, &postProcessingParams);

	const glm::ivec3 groupCount(
		DivideAndRoundUp(camera._SceneColor.GetWidth(), 8u),
		DivideAndRoundUp(camera._SceneColor.GetHeight(), 8u),
		1
	);

	cmdBuf.Dispatch(groupCount.x, groupCount.y, groupCount.z);
}