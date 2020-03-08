#include "MaterialShader.h"
#include "SceneProxy.h"
#include "SceneRenderer.h"
#include "GlobalRenderData.h"
#include <ECS/EntityManager.h>

template<EMeshType MeshType>
class DepthPrepassVS : public MeshShader<MeshType>
{
	using Base = MeshShader<MeshType>;
public:
	DepthPrepassVS(const ShaderCompilationInfo& CompilationInfo)
		: Base(CompilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		Base::SetEnvironmentVariables(Worker);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo BaseInfo = { "../Shaders/DepthPrepass.glsl", "main", EShaderStage::Vertex };
		return BaseInfo;
	}
};

template<EMeshType MeshType>
class DepthPrepassFS : public MeshShader<MeshType>
{
	using Base = MeshShader<MeshType>;
public:
	DepthPrepassFS(const ShaderCompilationInfo& CompilationInfo)
		: Base(CompilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		Base::SetEnvironmentVariables(Worker);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo BaseInfo = { "../Shaders/DepthPrepass.glsl", "main", EShaderStage::Fragment };
		return BaseInfo;
	}
};

void SceneProxy::AddToDepthPrepass(DRMDevice& Device, DRMShaderMap& ShaderMap, const MeshProxy& MeshProxy)
{
	auto& GlobalData = ECS.GetSingletonComponent<GlobalRenderData>();

	constexpr EMeshType MeshType = EMeshType::StaticMesh;

	std::vector<const drm::DescriptorSet*> DescriptorSets =
	{
		GlobalData.CameraDescriptorSet, &MeshProxy.GetSurfaceSet(), &MeshProxy.GetMaterialSet()
	};

	PipelineStateDesc PSODesc = {};
	PSODesc.RenderPass = GlobalData.DepthRP;
	PSODesc.ShaderStages.Vertex = ShaderMap.FindShader<DepthPrepassVS<MeshType>>();
	PSODesc.ShaderStages.Fragment = MeshProxy.GetMaterial()->IsMasked() ? ShaderMap.FindShader<DepthPrepassFS<MeshType>>() : nullptr;
	PSODesc.Viewport.Width = GlobalData.SceneDepth.GetWidth();
	PSODesc.Viewport.Height = GlobalData.SceneDepth.GetHeight();
	PSODesc.DescriptorSets = DescriptorSets;

	DepthPrepass.push_back(MeshDrawCommand(Device, MeshProxy, PSODesc));
}

void SceneRenderer::RenderDepthPrepass(SceneProxy& Scene, drm::CommandList& CmdList)
{
	auto& GlobalData = ECS.GetSingletonComponent<GlobalRenderData>();

	CmdList.BeginRenderPass(GlobalData.DepthRP);

	MeshDrawCommand::Draw(CmdList, Scene.DepthPrepass);

	CmdList.EndRenderPass();
}