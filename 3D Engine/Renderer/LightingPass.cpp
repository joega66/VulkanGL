#include "MaterialShader.h"
#include "SceneProxy.h"
#include "SceneRenderer.h"

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

void SceneProxy::AddToLightingPass(SceneRenderer& SceneRenderer, DRMDevice& Device, DRMShaderMap& ShaderMap, const MeshProxy& MeshProxy)
{
	static constexpr EMeshType MeshType = EMeshType::StaticMesh;

	PipelineStateDesc PSODesc = {};
	PSODesc.RenderPass = &SceneRenderer.LightingRP;
	PSODesc.ShaderStages.Vertex = ShaderMap.FindShader<LightingPassVS<MeshType>>();
	PSODesc.ShaderStages.Fragment = ShaderMap.FindShader<LightingPassFS<MeshType>>();
	PSODesc.Viewport.Width = SceneRenderer.SceneDepth.GetWidth();
	PSODesc.Viewport.Height = SceneRenderer.SceneDepth.GetHeight();
	PSODesc.DepthStencilState.DepthCompareTest = EDepthCompareTest::Equal;
	PSODesc.DepthStencilState.DepthWriteEnable = false;
	PSODesc.DescriptorSets = { SceneRenderer.CameraDescriptorSet, &MeshProxy.GetSurfaceSet(), &MeshProxy.GetMaterialSet(), SceneRenderer.SceneTexturesDescriptorSet };

	const EStaticDrawListType::EStaticDrawListType StaticDrawListType =
		MeshProxy.GetMaterial()->IsMasked() ?
		EStaticDrawListType::Masked : EStaticDrawListType::Opaque;

	LightingPass[StaticDrawListType].push_back(MeshDrawCommand(Device, MeshProxy, PSODesc));
}

void SceneRenderer::RenderLightingPass(SceneProxy& Scene, drm::CommandList& CmdList)
{
	CmdList.BeginRenderPass(LightingRP);

	MeshDrawCommand::Draw(CmdList, Scene.LightingPass[EStaticDrawListType::Opaque]);
	MeshDrawCommand::Draw(CmdList, Scene.LightingPass[EStaticDrawListType::Masked]);
}