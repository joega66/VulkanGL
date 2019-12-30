#include "LightingPass.h"
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

LightingPass::LightingPass(const MeshProxy& MeshProxy)
{
	static constexpr EMeshType MeshType = EMeshType::StaticMesh;

	VertShader = *ShaderMapRef<LightingPassVS<MeshType>>();
	FragShader = *ShaderMapRef<LightingPassFS<MeshType>>();
}

void LightingPass::BindDescriptorSets(drm::CommandList& CmdList, const MeshProxy& MeshProxy, const PassDescriptors& Pass) const
{
	const std::array<drm::DescriptorSetRef, 4> DescriptorSets =
	{
		Pass.SceneSet,
		MeshProxy.MeshSet,
		MeshProxy.MaterialSet,
		Pass.SceneTextures,
	};

	CmdList.BindDescriptorSets(DescriptorSets.size(), DescriptorSets.data());
}

void LightingPass::SetPipelineState(PipelineStateInitializer& PSOInit, const MeshProxy& MeshProxy) const
{
	PSOInit.SpecializationInfo = MeshProxy.SpecializationInfo;

	PSOInit.GraphicsPipelineState =
	{
		VertShader,
		nullptr,
		nullptr,
		nullptr,
		FragShader
	};
}

void LightingPass::Draw(drm::CommandList& CmdList, const MeshElement& MeshElement) const
{
	CmdList.BindVertexBuffers(MeshElement.VertexBuffers.size(), MeshElement.VertexBuffers.data());
	CmdList.DrawIndexed(MeshElement.IndexBuffer, MeshElement.IndexCount, 1, 0, 0, 0);
}

void SceneRenderer::RenderLightingPass(SceneProxy& Scene, drm::CommandList& CmdList)
{
	PipelineStateInitializer PSOInit = {};
	PSOInit.Viewport.Width = SceneDepth->Width;
	PSOInit.Viewport.Height = SceneDepth->Height;
	PSOInit.DepthStencilState.DepthCompareTest = EDepthCompareTest::Equal;
	PSOInit.DepthStencilState.DepthWriteEnable = false;

	LightingPass::PassDescriptors Descriptors = { Scene.DescriptorSet, SceneTextures };

	Scene.LightingPass[EStaticDrawListType::Opaque].Draw(CmdList, PSOInit, Descriptors);
	Scene.LightingPass[EStaticDrawListType::Masked].Draw(CmdList, PSOInit, Descriptors);
}