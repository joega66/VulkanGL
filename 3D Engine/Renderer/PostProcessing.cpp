#include "SceneRenderer.h"
#include <ECS/EntityManager.h>
#include <Components/RenderSettings.h>

BEGIN_PUSH_CONSTANTS(PostProcessingParams)
	MEMBER(float, _ExposureAdjustment)
	MEMBER(float, _ExposureBias)
	MEMBER(uint32, _DisplayColor)
	MEMBER(uint32, _HDRColor)
END_PUSH_CONSTANTS(PostProcessingParams)

class PostProcessingCS : public gpu::Shader
{
public:
	PostProcessingCS() = default;
	PostProcessingCS(const ShaderCompilationInfo& compilationInfo)
		: gpu::Shader(compilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& worker)
	{
		worker << PostProcessingParams::decl;
	}
};

REGISTER_SHADER(PostProcessingCS, "../Shaders/PostProcessingCS.glsl", "main", EShaderStage::Compute);

void SceneRenderer::ComputePostProcessing(const gpu::Image& displayImage, CameraProxy& camera, gpu::CommandList& cmdList)
{
	auto& settings = _ECS.GetSingletonComponent<RenderSettings>();

	const gpu::Shader* shader = _ShaderLibrary.FindShader<PostProcessingCS>();

	ComputePipelineDesc computeDesc;
	computeDesc.computeShader = shader;

	gpu::Pipeline pipeline = _Device.CreatePipeline(computeDesc);

	cmdList.BindPipeline(pipeline);

	cmdList.BindDescriptorSets(pipeline, 1, &_Device.GetImages());

	PostProcessingParams postProcessingParams;
	postProcessingParams._ExposureAdjustment = settings.ExposureAdjustment;
	postProcessingParams._ExposureBias = settings.ExposureBias;
	postProcessingParams._DisplayColor = displayImage.GetImageID();
	postProcessingParams._HDRColor = camera._SceneColor.GetImageID();

	cmdList.PushConstants(pipeline, shader, &postProcessingParams);

	const glm::ivec3 groupCount(
		DivideAndRoundUp(camera._SceneColor.GetWidth(), 8u),
		DivideAndRoundUp(camera._SceneColor.GetHeight(), 8u),
		1
	);

	cmdList.Dispatch(groupCount.x, groupCount.y, groupCount.z);
}