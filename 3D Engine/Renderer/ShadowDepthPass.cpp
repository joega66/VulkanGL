#include "MaterialShader.h"
#include "MeshProxy.h"
#include "SceneRenderer.h"
#include "FullscreenQuad.h"
#include "ShadowProxy.h"
#include <ECS/EntityManager.h>

template<EMeshType MeshType>
class ShadowDepthVS : public MeshShader<MeshType>
{
	using Base = MeshShader<MeshType>;
public:
	ShadowDepthVS(const ShaderCompilationInfo& CompilationInfo)
		: Base(CompilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		Base::SetEnvironmentVariables(Worker);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo BaseInfo = { "../Shaders/ShadowDepthVS.glsl", "main", EShaderStage::Vertex };
		return BaseInfo;
	}
};

template<EMeshType MeshType>
class ShadowDepthFS : public MeshShader<MeshType>
{
	using Base = MeshShader<MeshType>;
public:
	ShadowDepthFS(const ShaderCompilationInfo& CompilationInfo)
		: Base(CompilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		Base::SetEnvironmentVariables(Worker);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo BaseInfo = { "../Shaders/ShadowDepthFS.glsl", "main", EShaderStage::Fragment };
		return BaseInfo;
	}
};

void SceneProxy::AddToShadowDepthPass(DRMShaderMap& ShaderMap, const MeshProxy& MeshProxy)
{
	constexpr EMeshType MeshType = EMeshType::StaticMesh;

	ShaderStages ShaderStages =
	{
		ShaderMap.FindShader<ShadowDepthVS<MeshType>>(),
		nullptr,
		nullptr,
		nullptr,
		ShaderMap.FindShader<ShadowDepthFS<MeshType>>()
	};

	ShadowDepthPass.push_back(MeshDrawCommand(std::move(ShaderStages), MeshProxy));
}

class ShadowProjectionFS : public drm::Shader
{
public:
	ShadowProjectionFS(const ShaderCompilationInfo& CompilationInfo)
		: drm::Shader(CompilationInfo)
	{
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo BaseInfo = { "../Shaders/ShadowProjectionFS.glsl", "main", EShaderStage::Fragment };
		return BaseInfo;
	}
};

struct ShadowDepthPassDescriptorSets
{
	const drm::DescriptorSet* ShadowProxyDescriptorSet;

	void Set(drm::CommandList& CmdList, const MeshProxy& MeshProxy) const
	{
		std::array<const drm::DescriptorSet*, 3> DescriptorSets =
		{
			ShadowProxyDescriptorSet,
			&MeshProxy.GetSurfaceSet(),
			&MeshProxy.GetMaterialSet()
		};

		CmdList.BindDescriptorSets(DescriptorSets.size(), DescriptorSets.data());
	}
};

void SceneRenderer::RenderShadowDepths(SceneProxy& Scene, drm::CommandList& CmdList)
{
	for (auto Entity : Scene.ECS.GetEntities<ShadowProxy>())
	{
		const ShadowProxy& ShadowProxy = Scene.ECS.GetComponent<class ShadowProxy>(Entity);
		const drm::RenderPass& RenderPass = ShadowProxy.GetRenderPass();
		const drm::Image& ShadowMap = ShadowProxy.GetShadowMap();

		CmdList.BeginRenderPass(RenderPass);

		PipelineStateDesc PSODesc = {};
		PSODesc.RenderPass = &RenderPass;
		PSODesc.Viewport.Width = ShadowMap.GetWidth();
		PSODesc.Viewport.Height = ShadowMap.GetHeight();
		PSODesc.RasterizationState.DepthBiasEnable = true;
		PSODesc.RasterizationState.DepthBiasConstantFactor = ShadowProxy.GetDepthBiasConstantFactor();
		PSODesc.RasterizationState.DepthBiasSlopeFactor = ShadowProxy.GetDepthBiasSlopeFactor();

		ShadowDepthPassDescriptorSets DescriptorSets = { &ShadowProxy.GetDescriptorSet() };

		MeshDrawCommand::Draw(Scene.ShadowDepthPass, CmdList, DescriptorSets, PSODesc);

		CmdList.EndRenderPass();
	}

	RenderShadowMask(Scene, CmdList);
}

void SceneRenderer::RenderShadowMask(SceneProxy& Scene, drm::CommandList& CmdList)
{
	CmdList.BeginRenderPass(ShadowMaskRP);

	for (auto Entity : Scene.ECS.GetEntities<ShadowProxy>())
	{
		const ShadowProxy& ShadowProxy = Scene.ECS.GetComponent<class ShadowProxy>(Entity);

		std::array<const drm::DescriptorSet*, 3> Descriptors =
		{
			Scene.CameraDescriptorSet,
			SceneTextures,
			&ShadowProxy.GetDescriptorSet()
		};

		CmdList.BindDescriptorSets(Descriptors.size(), Descriptors.data());

		PipelineStateDesc PSODesc = {};
		PSODesc.RenderPass = &ShadowMaskRP;
		PSODesc.Viewport.Width = ShadowMask.GetWidth();
		PSODesc.Viewport.Height = ShadowMask.GetHeight();
		PSODesc.DepthStencilState.DepthCompareTest = EDepthCompareTest::Always;
		PSODesc.DepthStencilState.DepthWriteEnable = false;
		PSODesc.ShaderStages.Vertex = ShaderMap.FindShader<FullscreenVS>();
		PSODesc.ShaderStages.Fragment = ShaderMap.FindShader<ShadowProjectionFS>();

		CmdList.BindPipeline(PSODesc);

		CmdList.Draw(3, 1, 0, 0);
	}

	CmdList.EndRenderPass();
}