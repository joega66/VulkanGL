#include "SceneRenderer.h"
#include <ECS/EntityManager.h>
#include <Components/RenderSettings.h>

BEGIN_SHADER_STRUCT(PostProcessingData)
	SHADER_PARAMETER(float, _ExposureAdjustment)
	SHADER_PARAMETER(float, _ExposureBias)
END_SHADER_STRUCT(PostProcessingData)

class PostProcessingCS : public drm::Shader
{
public:
	PostProcessingCS(const ShaderCompilationInfo& CompilationInfo)
		: drm::Shader(CompilationInfo)
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
	drm::DescriptorImageInfo DisplayColor;
	drm::DescriptorImageInfo HDRColor;

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

void SceneRenderer::ComputePostProcessing(const drm::Image& DisplayImage, CameraProxy& Camera, drm::CommandList& CmdList)
{
	auto& RenderSettings = ECS.GetSingletonComponent<class RenderSettings>();

	drm::DescriptorSetLayout Layout = Device.CreateDescriptorSetLayout(PostProcessingDescriptors::GetBindings().size(), PostProcessingDescriptors::GetBindings().data());
	drm::DescriptorSet DescriptorSet = Layout.CreateDescriptorSet(Device);
	
	PostProcessingDescriptors Descriptors;
	Descriptors.DisplayColor = drm::DescriptorImageInfo(DisplayImage);
	Descriptors.HDRColor = drm::DescriptorImageInfo(Camera.SceneColor);

	Layout.UpdateDescriptorSet(Device, DescriptorSet, &Descriptors);

	const drm::Shader* Shader = ShaderLibrary.FindShader<PostProcessingCS>();

	ComputePipelineDesc ComputeDesc;
	ComputeDesc.ComputeShader = Shader;
	ComputeDesc.Layouts = { Layout };

	drm::Pipeline Pipeline = Device.CreatePipeline(ComputeDesc);

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