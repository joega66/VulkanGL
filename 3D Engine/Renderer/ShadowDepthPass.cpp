#include "MaterialShader.h"
#include "MeshProxy.h"
#include "SceneRenderer.h"
#include "FullscreenQuad.h"
#include "ShadowProxy.h"

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

void SceneProxy::AddToShadowDepthPass(const MeshProxy& MeshProxy)
{
	constexpr EMeshType MeshType = EMeshType::StaticMesh;

	ShaderStages ShaderStages =
	{
		*ShaderMapRef<ShadowDepthVS<MeshType>>(),
		nullptr,
		nullptr,
		nullptr,
		*ShaderMapRef<ShadowDepthFS<MeshType>>()
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
	drm::DescriptorSetRef ShadowProxyDescriptorSet;

	void Set(drm::CommandList& CmdList, const MeshProxy& MeshProxy) const
	{
		std::array<drm::DescriptorSetRef, 3> DescriptorSets =
		{
			ShadowProxyDescriptorSet,
			MeshProxy.MeshSet,
			MeshProxy.MaterialSet
		};

		CmdList.BindDescriptorSets(DescriptorSets.size(), DescriptorSets.data());
	}
};

void SceneRenderer::RenderShadowDepths(SceneProxy& Scene, drm::CommandList& CmdList)
{
	for (auto Entity : Scene.ECS.GetEntities<ShadowProxy>())
	{
		const ShadowProxy& ShadowProxy = Scene.ECS.GetComponent<class ShadowProxy>(Entity);
		drm::ImageRef ShadowMap = ShadowProxy.GetShadowMap();

		RenderPassInitializer RPInit = { 0 };
		RPInit.DepthAttachment = drm::AttachmentView(
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

		ShadowDepthPassDescriptorSets DescriptorSets = { ShadowProxy.GetDescriptorSet() };

		MeshDrawCommand::Draw(Scene.ShadowDepthPass, CmdList, DescriptorSets, PSOInit);

		CmdList.EndRenderPass();
	}

	RenderShadowMask(Scene, CmdList);
}

void SceneRenderer::RenderShadowMask(SceneProxy& Scene, drm::CommandList& CmdList)
{
	RenderPassInitializer RPInit = { 1 };
	RPInit.ColorAttachments[0] = drm::AttachmentView(ShadowMask, ELoadAction::Clear, EStoreAction::Store, ClearColorValue{}, EImageLayout::ShaderReadOnlyOptimal);
	RPInit.RenderArea = RenderArea{ glm::ivec2{}, glm::uvec2(ShadowMask->Width, ShadowMask->Height) };

	CmdList.BeginRenderPass(RPInit);

	for (auto Entity : Scene.ECS.GetEntities<ShadowProxy>())
	{
		const ShadowProxy& ShadowProxy = Scene.ECS.GetComponent<class ShadowProxy>(Entity);

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
		PSOInit.ShaderStages.Vertex = *ShaderMapRef<FullscreenVS>();
		PSOInit.ShaderStages.Fragment = *ShaderMapRef<ShadowProjectionFS>();

		CmdList.BindPipeline(PSOInit);

		CmdList.Draw(3, 1, 0, 0);
	}

	CmdList.EndRenderPass();
}