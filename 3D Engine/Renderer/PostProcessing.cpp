#include "SceneRenderer.h"
#include <ECS/EntityManager.h>
#include <Components/RenderSettings.h>

BEGIN_SHADER_STRUCT(PostProcessingData)
	SHADER_PARAMETER(float, _ExposureAdjustment)
	SHADER_PARAMETER(float, _ExposureBias)
END_SHADER_STRUCT(PostProcessingData)

class PostProcessingCS : public gpu::Shader
{
public:
	PostProcessingCS(const ShaderCompilationInfo& CompilationInfo)
		: gpu::Shader(CompilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		Worker << PostProcessingData::Decl;
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo BaseInfo = { "../Shaders/PostProcessingCS.glsl", "main", EShaderStage::Compute };
		return BaseInfo;
	}
};

struct PostProcessingDescriptors
{
	gpu::DescriptorImageInfo DisplayColor;
	gpu::DescriptorImageInfo HDRColor;

	static auto& GetBindings()
	{
		static std::vector<DescriptorBinding> Bindings =
		{
			{ 0, 1, EDescriptorType::StorageImage },
			{ 1, 1, EDescriptorType::StorageImage },
		};
		return Bindings;
	}
};

void SceneRenderer::ComputePostProcessing(const gpu::Image& DisplayImage, CameraProxy& Camera, gpu::CommandList& CmdList)
{
	auto& RenderSettings = ECS.GetSingletonComponent<class RenderSettings>();

	gpu::DescriptorSetLayout Layout = Device.CreateDescriptorSetLayout(PostProcessingDescriptors::GetBindings().size(), PostProcessingDescriptors::GetBindings().data());
	gpu::DescriptorSet DescriptorSet = Layout.CreateDescriptorSet(Device);
	
	PostProcessingDescriptors Descriptors;
	Descriptors.DisplayColor = gpu::DescriptorImageInfo(DisplayImage);
	Descriptors.HDRColor = gpu::DescriptorImageInfo(Camera.SceneColor);

	Layout.UpdateDescriptorSet(Device, DescriptorSet, &Descriptors);

	const gpu::Shader* Shader = ShaderLibrary.FindShader<PostProcessingCS>();

	ComputePipelineDesc ComputeDesc;
	ComputeDesc.computeShader = Shader;
	ComputeDesc.Layouts = { Layout };

	gpu::Pipeline Pipeline = Device.CreatePipeline(ComputeDesc);

	CmdList.BindPipeline(Pipeline);

	CmdList.BindDescriptorSets(Pipeline, 1, &DescriptorSet.GetHandle());

	PostProcessingData PostProcessingData;
	PostProcessingData._ExposureAdjustment = RenderSettings.ExposureAdjustment;
	PostProcessingData._ExposureBias = RenderSettings.ExposureBias;

	CmdList.PushConstants(Pipeline, Shader, &PostProcessingData);

	const glm::ivec3 GroupCount(
		DivideAndRoundUp(Camera.SceneColor.GetWidth(), 8u),
		DivideAndRoundUp(Camera.SceneColor.GetHeight(), 8u),
		1
	);

	CmdList.Dispatch(GroupCount.x, GroupCount.y, GroupCount.z);
}