#include "LightingPass.h"
#include "MaterialShader.h"
#include "SceneProxy.h"

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

void LightingPass::BindDescriptorSets(RenderCommandList& CmdList, const MeshProxy& MeshProxy, const PassDescriptors& Pass) const
{
	const std::array<drm::DescriptorSetRef, 2> DescriptorSets =
	{
		Pass.SceneSet,
		MeshProxy.MaterialSet
	};

	CmdList.BindDescriptorSets(DescriptorSets.size(), DescriptorSets.data());
}

void LightingPass::SetPipelineState(PipelineStateInitializer& PSOInit, const MeshProxy& MeshProxy) const
{
	PSOInit.SpecializationInfos[(int32)EShaderStage::Fragment] = MeshProxy.SpecInfo;

	PSOInit.GraphicsPipelineState =
	{
		VertShader,
		nullptr,
		nullptr,
		nullptr,
		FragShader
	};
}

void LightingPass::Draw(RenderCommandList& CmdList, const MeshElement& MeshElement) const
{
	CmdList.BindVertexBuffers(MeshElement.VertexBuffers.size(), MeshElement.VertexBuffers.data());
	CmdList.DrawIndexed(MeshElement.IndexBuffer, MeshElement.IndexCount, 1, 0, 0, 0);
}