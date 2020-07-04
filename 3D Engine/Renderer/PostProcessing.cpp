#include "SceneRenderer.h"
#include <ECS/EntityManager.h>
#include <Components/RenderSettings.h>

BEGIN_SHADER_STRUCT(PostProcessingParams)
	SHADER_PARAMETER(float, _ExposureAdjustment)
	SHADER_PARAMETER(float, _ExposureBias)
END_SHADER_STRUCT(PostProcessingParams)

class PostProcessingCS : public gpu::Shader
{
public:
	PostProcessingCS(const ShaderCompilationInfo& compilationInfo)
		: gpu::Shader(compilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& worker)
	{
		worker << PostProcessingParams::Decl;
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo info = { "../Shaders/PostProcessingCS.glsl", "main", EShaderStage::Compute };
		return info;
	}
};

struct PostProcessingDescriptors
{
	gpu::DescriptorImageInfo _DisplayColor;
	gpu::DescriptorImageInfo _HDRColor;

	static auto& GetBindings()
	{
		static std::vector<DescriptorBinding> bindings =
		{
			{ 0, 1, EDescriptorType::StorageImage },
			{ 1, 1, EDescriptorType::StorageImage },
		};
		return bindings;
	}
};

void SceneRenderer::ComputePostProcessing(const gpu::Image& displayImage, CameraProxy& camera, gpu::CommandList& cmdList)
{
	auto& settings = _ECS.GetSingletonComponent<RenderSettings>();

	gpu::DescriptorSetLayout layout = _Device.CreateDescriptorSetLayout(PostProcessingDescriptors::GetBindings().size(), PostProcessingDescriptors::GetBindings().data());
	gpu::DescriptorSet descriptorSet = layout.CreateDescriptorSet(_Device);
	
	PostProcessingDescriptors descriptors;
	descriptors._DisplayColor = gpu::DescriptorImageInfo(displayImage);
	descriptors._HDRColor = gpu::DescriptorImageInfo(camera._SceneColor);

	layout.UpdateDescriptorSet(_Device, descriptorSet, &descriptors);

	const gpu::Shader* shader = _ShaderLibrary.FindShader<PostProcessingCS>();

	ComputePipelineDesc computeDesc;
	computeDesc.computeShader = shader;

	gpu::Pipeline pipeline = _Device.CreatePipeline(computeDesc);

	cmdList.BindPipeline(pipeline);

	cmdList.BindDescriptorSets(pipeline, 1, &descriptorSet.GetHandle());

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