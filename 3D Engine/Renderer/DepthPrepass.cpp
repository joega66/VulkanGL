#include "DepthPrepass.h"
#include "MaterialShader.h"
#include "SceneProxy.h"
#include "SceneRenderer.h"

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

DepthPrepass::DepthPrepass(const MeshProxy& MeshProxy)
{
	static constexpr EMeshType MeshType = EMeshType::StaticMesh;

	VertShader = *ShaderMapRef<DepthPrepassVS<MeshType>>();

	const CMaterial& Material = MeshProxy.Material;

	// Optional: Frag shader only needed for masked materials.
	if (Material.IsMasked())
	{
		FragShader = *ShaderMapRef<DepthPrepassFS<MeshType>>();
	}
}

void DepthPrepass::BindDescriptorSets(RenderCommandList& CmdList, const MeshProxy& MeshProxy, const PassDescriptors& Pass) const
{
	std::array<drm::DescriptorSetRef, 3> DescriptorSets =
	{
		Pass.SceneSet,
		MeshProxy.MeshSet,
		MeshProxy.MaterialSet
	};

	CmdList.BindDescriptorSets(DescriptorSets.size(), DescriptorSets.data());
}

void DepthPrepass::SetPipelineState(PipelineStateInitializer& PSOInit, const MeshProxy& MeshProxy) const
{
	if (FragShader)
	{
		PSOInit.SpecializationInfos[(int32)EShaderStage::Fragment] = MeshProxy.SpecializationInfo;
	}
	
	PSOInit.GraphicsPipelineState =
	{
		VertShader,
		nullptr,
		nullptr,
		nullptr,
		FragShader
	};
}

void DepthPrepass::Draw(RenderCommandList& CmdList, const MeshElement& MeshElement) const
{
	CmdList.BindVertexBuffers(MeshElement.VertexBuffers.size(), MeshElement.VertexBuffers.data());
	CmdList.DrawIndexed(MeshElement.IndexBuffer, MeshElement.IndexCount, 1, 0, 0, 0);
}

void SceneRenderer::RenderDepthPrepass(SceneProxy& Scene, RenderCommandList& CmdList)
{
	RenderPassInitializer RPInit = { 0 };
	RPInit.DepthTarget = drm::RenderTargetView(
		SceneDepth,
		ELoadAction::Clear,
		EStoreAction::Store, 
		ClearDepthStencilValue{}, 
		EImageLayout::DepthReadStencilWrite);
	RPInit.RenderArea = RenderArea{ glm::ivec2(), glm::uvec2(SceneDepth->Width, SceneDepth->Height) };

	CmdList.BeginRenderPass(RPInit);

	PipelineStateInitializer PSOInit = {};
	PSOInit.Viewport.Width = SceneDepth->Width;
	PSOInit.Viewport.Height = SceneDepth->Height;

	DepthPrepass::PassDescriptors PassDescriptors = { Scene.DescriptorSet };

	Scene.DepthPrepass.Draw(CmdList, PSOInit, PassDescriptors);

	CmdList.EndRenderPass();
}