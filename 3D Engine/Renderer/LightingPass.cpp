#include "MaterialShader.h"
#include "SceneProxy.h"
#include "SceneRenderer.h"
#include "GlobalRenderResources.h"
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
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo BaseInfo = { "../Shaders/LightingPassFS.glsl", "main", EShaderStage::Fragment };
		return BaseInfo;
	}
};

void SceneProxy::AddToLightingPass(DRMDevice& Device, DRMShaderMap& ShaderMap, const MeshProxy& MeshProxy)
{
	auto& GlobalResources = ECS.GetSingletonComponent<GlobalRenderResources>();

	static constexpr EMeshType MeshType = EMeshType::StaticMesh;

	PipelineStateDesc PSODesc = {};
	PSODesc.RenderPass = GlobalResources.LightingRP;
	PSODesc.ShaderStages.Vertex = ShaderMap.FindShader<LightingPassVS<MeshType>>();
	PSODesc.ShaderStages.Fragment = ShaderMap.FindShader<LightingPassFS<MeshType>>();
	PSODesc.Viewport.Width = GlobalResources.SceneDepth.GetWidth();
	PSODesc.Viewport.Height = GlobalResources.SceneDepth.GetHeight();
	PSODesc.DepthStencilState.DepthCompareTest = EDepthCompareTest::Equal;
	PSODesc.DepthStencilState.DepthWriteEnable = false;
	PSODesc.DescriptorSets = { GlobalResources.CameraDescriptorSet, &MeshProxy.GetSurfaceSet(), &MeshProxy.GetMaterialSet(), GlobalResources.SceneTexturesDescriptorSet };

	const EStaticDrawListType::EStaticDrawListType StaticDrawListType =
		MeshProxy.GetMaterial()->IsMasked() ?
		EStaticDrawListType::Masked : EStaticDrawListType::Opaque;

	LightingPass[StaticDrawListType].push_back(MeshDrawCommand(Device, MeshProxy, PSODesc));
}

void SceneRenderer::RenderLightingPass(SceneProxy& Scene, drm::CommandList& CmdList)
{
	auto& GlobalResources = ECS.GetSingletonComponent<GlobalRenderResources>();

	CmdList.BeginRenderPass(GlobalResources.LightingRP);

	MeshDrawCommand::Draw(CmdList, Scene.LightingPass[EStaticDrawListType::Opaque]);
	MeshDrawCommand::Draw(CmdList, Scene.LightingPass[EStaticDrawListType::Masked]);
}