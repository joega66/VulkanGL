#include "ShadowDepthPass.h"
#include "MaterialShader.h"
#include "MeshProxy.h"
#include "SceneRenderer.h"
#include "FullscreenQuad.h"

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

ShadowDepthPass::ShadowDepthPass(const MeshProxy& MeshProxy)
{
	constexpr EMeshType MeshType = EMeshType::StaticMesh;

	VertShader = *ShaderMapRef<ShadowDepthVS<MeshType>>();

	if (MeshProxy.Material.IsMasked())
	{
		FragShader = *ShaderMapRef<ShadowDepthFS<MeshType>>();
	}
}

void ShadowDepthPass::BindDescriptorSets(RenderCommandList& CmdList, const MeshProxy& MeshProxy, const PassDescriptors& Pass) const
{
	const std::array<drm::DescriptorSetRef, 3> DescriptorSets =
	{
		Pass.ShadowProxy,
		MeshProxy.MeshSet,
		MeshProxy.MaterialSet
	};

	CmdList.BindDescriptorSets(DescriptorSets.size(), DescriptorSets.data());
}

void ShadowDepthPass::SetPipelineState(PipelineStateInitializer& PSOInit, const MeshProxy& MeshProxy) const
{
	PSOInit.SpecializationInfos[(int32)EShaderStage::Fragment] = MeshProxy.SpecializationInfo;

	PSOInit.GraphicsPipelineState =
	{
		VertShader,
		nullptr,
		nullptr,
		nullptr,
		FragShader
	};
}

void ShadowDepthPass::Draw(RenderCommandList& CmdList, const MeshElement& MeshElement) const
{
	CmdList.BindVertexBuffers(MeshElement.VertexBuffers.size(), MeshElement.VertexBuffers.data());
	CmdList.DrawIndexed(MeshElement.IndexBuffer, MeshElement.IndexCount, 1, 0, 0, 0);
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

void SceneRenderer::RenderShadowDepths(SceneProxy& Scene, RenderCommandList& CmdList)
{
	for (auto Entity : Scene.ECS.GetEntities<CShadowProxy>())
	{
		auto& ShadowProxy = Scene.ECS.GetComponent<CShadowProxy>(Entity);
		drm::ImageRef ShadowMap = ShadowProxy.GetShadowMap();

		RenderPassInitializer RPInit = { 0 };
		RPInit.DepthTarget = drm::RenderTargetView(
			ShadowMap,
			ELoadAction::Clear, EStoreAction::Store,
			ClearDepthStencilValue{},
			EImageLayout::DepthReadStencilWrite
		);
		RPInit.RenderArea = RenderArea{ glm::ivec2{}, glm::uvec2(ShadowMap->Width, ShadowMap->Height) };
		
		CmdList.BeginRenderPass(RPInit);

		PipelineStateInitializer PSOInit = {};
		PSOInit.Viewport.Width = ShadowMap->Width;
		PSOInit.Viewport.Height = ShadowMap->Height;
		PSOInit.RasterizationState.DepthBiasEnable = true;
		PSOInit.RasterizationState.DepthBiasConstantFactor = ShadowProxy.GetDepthBiasConstantFactor();
		PSOInit.RasterizationState.DepthBiasSlopeFactor = ShadowProxy.GetDepthBiasSlopeFactor();

		ShadowDepthPass::PassDescriptors PassDescriptors = { ShadowProxy.GetDescriptorSet() };

		Scene.ShadowDepthPass.Draw(CmdList, PSOInit, PassDescriptors);

		CmdList.EndRenderPass();
	}

	RenderShadowMask(Scene, CmdList);
}

void SceneRenderer::RenderShadowMask(SceneProxy& Scene, RenderCommandList& CmdList)
{
	drm::DescriptorSetRef SceneTextures = drm::CreateDescriptorSet();
	SceneTextures->Write(SceneDepth, SamplerState{ EFilter::Nearest }, ShaderBinding(0));
	SceneTextures->Update();

	RenderPassInitializer RPInit = { 1 };
	RPInit.ColorTargets[0] = drm::RenderTargetView(ShadowMask, ELoadAction::Clear, EStoreAction::Store, ClearColorValue{}, EImageLayout::ShaderReadOnlyOptimal);
	RPInit.RenderArea = RenderArea{ glm::ivec2{}, glm::uvec2(ShadowMask->Width, ShadowMask->Height) };

	CmdList.BeginRenderPass(RPInit);

	for (auto Entity : Scene.ECS.GetEntities<CShadowProxy>())
	{
		auto& ShadowProxy = Scene.ECS.GetComponent<CShadowProxy>(Entity);

		std::array<drm::DescriptorSetRef, 3> Descriptors =
		{
			Scene.DescriptorSet,
			SceneTextures,
			ShadowProxy.GetDescriptorSet()
		};

		CmdList.BindDescriptorSets(Descriptors.size(), Descriptors.data());

		PipelineStateInitializer PSOInit = {};
		PSOInit.Viewport.Width = ShadowMask->Width;
		PSOInit.Viewport.Height = ShadowMask->Height;
		PSOInit.DepthStencilState.DepthCompareTest = EDepthCompareTest::Always;
		PSOInit.DepthStencilState.DepthWriteEnable = false;
		PSOInit.GraphicsPipelineState.Vertex = *ShaderMapRef<FullscreenVS>();
		PSOInit.GraphicsPipelineState.Fragment = *ShaderMapRef<ShadowProjectionFS>();

		CmdList.BindPipeline(PSOInit);

		CmdList.Draw(3, 1, 0, 0);
	}

	CmdList.EndRenderPass();
}