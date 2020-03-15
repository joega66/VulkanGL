#include "MaterialShader.h"
#include "SceneProxy.h"
#include "SceneRenderer.h"
#include "GlobalRenderData.h"
#include <ECS/EntityManager.h>

template<EMeshType MeshType>
class LightingPassVS : public MeshShader<MeshType>
{
	using Base = MeshShader<MeshType>;
public:
	LightingPassVS(const ShaderCompilationInfo& CompilationInfo)
		: Base(CompilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		Base::SetEnvironmentVariables(Worker);
		VoxelShader::SetEnvironmentVariables(Worker);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo BaseInfo = { "../Shaders/LightingPassVS.glsl", "main", EShaderStage::Vertex };
		return BaseInfo;
	}
};

template<EMeshType MeshType>
class LightingPassFS : public MeshShader<MeshType>
{
	using Base = MeshShader<MeshType>;
public:
	LightingPassFS(const ShaderCompilationInfo& CompilationInfo)
		: Base(CompilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		Base::SetEnvironmentVariables(Worker);
		VoxelShader::SetEnvironmentVariables(Worker);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo BaseInfo = { "../Shaders/LightingPassFS.glsl", "main", EShaderStage::Fragment };
		return BaseInfo;
	}
};

void SceneProxy::AddToLightingPass(DRMDevice& Device, DRMShaderMap& ShaderMap, const MeshProxy& MeshProxy)
{
	auto& GlobalData = ECS.GetSingletonComponent<GlobalRenderData>();

	static constexpr EMeshType MeshType = EMeshType::StaticMesh;

	PipelineStateDesc PSODesc = {};
	PSODesc.RenderPass = GlobalData.LightingRP;
	PSODesc.ShaderStages.Vertex = ShaderMap.FindShader<LightingPassVS<MeshType>>();
	PSODesc.ShaderStages.Fragment = ShaderMap.FindShader<LightingPassFS<MeshType>>();
	PSODesc.Viewport.Width = GlobalData.SceneDepth.GetWidth();
	PSODesc.Viewport.Height = GlobalData.SceneDepth.GetHeight();
	PSODesc.DepthStencilState.DepthCompareTest = EDepthCompareTest::Equal;
	PSODesc.DepthStencilState.DepthWriteEnable = false;
	PSODesc.Layouts = 
	{
		GlobalData.CameraDescriptorSet.GetLayout(), 
		MeshProxy.GetSurfaceSet().GetLayout(), 
		MeshProxy.GetMaterialSet().GetLayout(), 
		GlobalData.SceneTexturesDescriptorSet.GetLayout(),
		GlobalData.VCTLightingCache.GetDescriptorSet().GetLayout()
	};

	const EStaticDrawListType::EStaticDrawListType StaticDrawListType =
		MeshProxy.GetMaterial()->IsMasked() ?
		EStaticDrawListType::Masked : EStaticDrawListType::Opaque;

	const std::vector<VkDescriptorSet> DescriptorSets =
	{
		GlobalData.CameraDescriptorSet,
		MeshProxy.GetSurfaceSet(),
		MeshProxy.GetMaterialSet(),
		GlobalData.SceneTexturesDescriptorSet,
		GlobalData.VCTLightingCache.GetDescriptorSet()
	};

	LightingPass[StaticDrawListType].push_back(MeshDrawCommand(Device, MeshProxy, PSODesc, DescriptorSets));
}

void SceneRenderer::RenderLightingPass(SceneProxy& Scene, drm::CommandList& CmdList)
{
	auto& GlobalData = ECS.GetSingletonComponent<GlobalRenderData>();
	auto& VCTLighting = GlobalData.VCTLightingCache;

	ImageMemoryBarrier ImageBarrier(VCTLighting.GetVoxelRadiance(), EAccess::ShaderWrite, EAccess::ShaderRead, EImageLayout::General, EImageLayout::ShaderReadOnlyOptimal);
	CmdList.PipelineBarrier(EPipelineStage::ComputeShader, EPipelineStage::FragmentShader, 0, nullptr, 1, &ImageBarrier);

	CmdList.BeginRenderPass(GlobalData.LightingRP);

	MeshDrawCommand::Draw(CmdList, Scene.LightingPass[EStaticDrawListType::Opaque]);
	MeshDrawCommand::Draw(CmdList, Scene.LightingPass[EStaticDrawListType::Masked]);
}