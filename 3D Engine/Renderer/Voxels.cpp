#include "Voxels.h"
#include "SceneRenderer.h"
#include "MaterialShader.h"
#include "MeshProxy.h"

const glm::uvec2 gVoxelGridSize = { 128, 128 };

template<EMeshType MeshType>
class VoxelsVS : public MaterialShader<MeshType>
{
	using Base = MaterialShader<MeshType>;
public:
	VoxelsVS(const ShaderCompilationInfo& CompilationInfo)
		: Base(CompilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		Base::SetEnvironmentVariables(Worker);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo BaseInfo = { "../Shaders/VoxelsVS.glsl", "main", EShaderStage::Vertex };
		return BaseInfo;
	}
};

template<EMeshType MeshType>
class VoxelsGS : public MaterialShader<MeshType>
{
	using Base = MaterialShader<MeshType>;
public:
	VoxelsGS(const ShaderCompilationInfo& CompilationInfo)
		: Base(CompilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		Base::SetEnvironmentVariables(Worker);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo BaseInfo = { "../Shaders/VoxelsGS.glsl", "main", EShaderStage::Geometry };
		return BaseInfo;
	}
};

template<EMeshType MeshType>
class VoxelsFS : public MaterialShader<MeshType>
{
	using Base = MaterialShader<MeshType>;
public:
	VoxelsFS(const ShaderCompilationInfo& CompilationInfo)
		: Base(CompilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		Base::SetEnvironmentVariables(Worker);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo BaseInfo = { "../Shaders/VoxelsFS.glsl", "main", EShaderStage::Fragment };
		return BaseInfo;
	}
};

VoxelizationPass::VoxelizationPass(const MeshProxy& MeshProxy)
{
	static constexpr EMeshType MeshType = EMeshType::StaticMesh;

	VertShader = *ShaderMapRef<VoxelsVS<MeshType>>();
	GeomShader = *ShaderMapRef<VoxelsGS<MeshType>>();
	FragShader = *ShaderMapRef<VoxelsFS<MeshType>>();
}

void VoxelizationPass::BindDescriptorSets(RenderCommandList& CmdList, const MeshProxy& MeshProxy, const PassDescriptors& Pass) const
{
	const std::array<drm::DescriptorSetRef, 3> DescriptorSets = 
	{
		Pass.Scene,
		MeshProxy.MaterialSet,
		Pass.Voxels
	};

	CmdList.BindDescriptorSets(DescriptorSets.size(), DescriptorSets.data());
}

void VoxelizationPass::SetPipelineState(PipelineStateInitializer& PSOInit, const MeshProxy& MeshProxy) const
{
	PSOInit.SpecializationInfos[(int32)EShaderStage::Fragment] = MeshProxy.SpecInfo;

	PSOInit.GraphicsPipelineState =
	{
		VertShader,
		nullptr,
		nullptr,
		GeomShader,
		FragShader
	};
}

void VoxelizationPass::Draw(RenderCommandList& CmdList, const MeshElement& MeshElement) const
{
	CmdList.BindVertexBuffers(MeshElement.VertexBuffers.size(), MeshElement.VertexBuffers.data());
	CmdList.DrawIndexed(MeshElement.IndexBuffer, MeshElement.IndexCount, 1, 0, 0, 0);
}

void SceneRenderer::RenderVoxels(SceneProxy& Scene, RenderCommandList& CmdList)
{
	VoxelizationPass::PassDescriptors Descriptors = { Scene.DescriptorSet, VoxelsDescriptorSet };

	RenderPassInitializer RPInit = { 0 }; // Disable ROP
	RPInit.RenderArea.Extent = gVoxelGridSize;

	CmdList.BeginRenderPass(RPInit);

	PipelineStateInitializer PSOInit;
	PSOInit.DepthStencilState.DepthTestEnable = false;
	PSOInit.DepthStencilState.DepthCompareTest = EDepthCompareTest::Always;
	PSOInit.Viewport.Width = gVoxelGridSize.x;
	PSOInit.Viewport.Height = gVoxelGridSize.y;

	Scene.VoxelsPass.Draw(CmdList, PSOInit, Descriptors);

	CmdList.EndRenderPass();
}