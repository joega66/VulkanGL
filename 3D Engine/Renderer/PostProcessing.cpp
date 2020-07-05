#include "PostProcessing.h"
#include "SceneRenderer.h"
#include <ECS/EntityManager.h>
#include <Components/RenderSettings.h>

BEGIN_PUSH_CONSTANTS(PostProcessingParams)
	SHADER_PARAMETER(float, _ExposureAdjustment)
	SHADER_PARAMETER(float, _ExposureBias)
END_PUSH_CONSTANTS(PostProcessingParams)

DECLARE_DESCRIPTOR_SET(PostProcessingDescriptors);

class PostProcessingCS : public gpu::Shader
{
public:
	PostProcessingCS(const ShaderCompilationInfo& compilationInfo)
		: gpu::Shader(compilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& worker)
	{
		worker << PostProcessingParams::decl;
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo info = { "../Shaders/PostProcessingCS.glsl", "main", EShaderStage::Compute };
		return info;
	}
};

void SceneRenderer::ComputePostProcessing(const gpu::Image& displayImage, CameraProxy& camera, gpu::CommandList& cmdList)
{
	auto& settings = _ECS.GetSingletonComponent<RenderSettings>();

	PostProcessingDescriptors descriptors;
	descriptors._DisplayColor = displayImage;
	descriptors._HDRColor = camera._SceneColor;

	_Device.UpdateDescriptorSet(_PostProcessingSet, descriptors);
	
	const gpu::Shader* shader = _ShaderLibrary.FindShader<PostProcessingCS>();

	ComputePipelineDesc computeDesc;
	computeDesc.computeShader = shader;

	gpu::Pipeline pipeline = _Device.CreatePipeline(computeDesc);

	cmdList.BindPipeline(pipeline);

	cmdList.BindDescriptorSets(pipeline, 1, &_PostProcessingSet.GetHandle());

	PostProcessingParams postProcessingParams;
	postProcessingParams._ExposureAdjustment = settings.ExposureAdjustment;
	postProcessingParams._ExposureBias = settings.ExposureBias;

	cmdList.PushConstants(pipeline, shader, &postProcessingParams);

	const glm::ivec3 groupCount(
		DivideAndRoundUp(camera._SceneColor.GetWidth(), 8u),
		DivideAndRoundUp(camera._SceneColor.GetHeight(), 8u),
		1
	);

	cmdList.Dispatch(groupCount.x, groupCount.y, groupCount.z);
}