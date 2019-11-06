#include "Voxels.h"
#include "SceneRenderer.h"
#include "MaterialShader.h"
#include "MeshProxy.h"
#include "FullscreenQuad.h"
#include <Engine/Screen.h>

uint32 gVoxelGridSize;

template<EMeshType MeshType>
class VoxelsVS : public MeshShader<MeshType>
{
	using Base = MeshShader<MeshType>;
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
class VoxelsGS : public MeshShader<MeshType>
{
	using Base = MeshShader<MeshType>;
public:
	VoxelsGS(const ShaderCompilationInfo& CompilationInfo)
		: Base(CompilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		Base::SetEnvironmentVariables(Worker);
		Worker.SetDefine("VOXEL_GRID_SIZE", gVoxelGridSize);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo BaseInfo = { "../Shaders/VoxelsGS.glsl", "main", EShaderStage::Geometry };
		return BaseInfo;
	}
};

template<EMeshType MeshType>
class VoxelsFS : public MeshShader<MeshType>
{
	using Base = MeshShader<MeshType>;
public:
	VoxelsFS(const ShaderCompilationInfo& CompilationInfo)
		: Base(CompilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		Base::SetEnvironmentVariables(Worker);
		Worker.SetDefine("VOXEL_GRID_SIZE", gVoxelGridSize);
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
	RenderVoxelization(Scene, CmdList);

	RenderVoxelVisualization(Scene, CmdList);
}

void SceneRenderer::RenderVoxelization(SceneProxy& Scene, RenderCommandList& CmdList)
{
	//CmdList.PipelineBarrier(VoxelColor, EImageLayout::TransferDstOptimal, EAccess::TRANSFER_WRITE, EPipelineStage::TRANSFER);

	//CmdList.ClearColorImage(VoxelColor, ClearColorValue{});

	//CmdList.PipelineBarrier(VoxelColor, EImageLayout::General, EAccess::SHADER_WRITE, EPipelineStage::FRAGMENT_SHADER);

	VoxelizationPass::PassDescriptors Descriptors = { Scene.DescriptorSet, VoxelsDescriptorSet };

	RenderPassInitializer RPInit = { 0 }; // Disable ROP
	RPInit.RenderArea.Extent = glm::uvec2(gVoxelGridSize);

	CmdList.BeginRenderPass(RPInit);

	PipelineStateInitializer PSOInit;
	PSOInit.DepthStencilState.DepthTestEnable = false;
	PSOInit.DepthStencilState.DepthWriteEnable = false;
	PSOInit.Viewport.Width = gVoxelGridSize;
	PSOInit.Viewport.Height = gVoxelGridSize;

	Scene.VoxelsPass.Draw(CmdList, PSOInit, Descriptors);

	CmdList.EndRenderPass();
}

class DrawVoxelsVS : public drm::Shader
{
public:
	DrawVoxelsVS(const ShaderCompilationInfo& CompilationInfo)
		: drm::Shader(CompilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		Worker.SetDefine("VOXEL_GRID_SIZE", gVoxelGridSize);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo BaseInfo = { "../Shaders/DrawVoxels.glsl", "main", EShaderStage::Vertex };
		return BaseInfo;
	}
};

class DrawVoxelsGS : public drm::Shader
{
public:
	DrawVoxelsGS(const ShaderCompilationInfo& CompilationInfo)
		: drm::Shader(CompilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		Worker.SetDefine("VOXEL_GRID_SIZE", gVoxelGridSize);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo BaseInfo = { "../Shaders/DrawVoxels.glsl", "main", EShaderStage::Geometry };
		return BaseInfo;
	}
};

class DrawVoxelsFS : public drm::Shader
{
public:
	DrawVoxelsFS(const ShaderCompilationInfo& CompilationInfo)
		: drm::Shader(CompilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		Worker.SetDefine("VOXEL_GRID_SIZE", gVoxelGridSize);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo BaseInfo = { "../Shaders/DrawVoxels.glsl", "main", EShaderStage::Fragment };
		return BaseInfo;
	}
};

void SceneRenderer::RenderVoxelVisualization(SceneProxy& Scene, RenderCommandList& CmdList)
{
	BufferMemoryBarrier BufferBarriers[] =
	{
		{ VoxelColors, EAccess::SHADER_WRITE, EAccess::SHADER_READ },
		{ VoxelPositions, EAccess::SHADER_WRITE, EAccess::SHADER_READ }
	};

	CmdList.PipelineBarrier(EPipelineStage::FRAGMENT_SHADER, EPipelineStage::VERTEX_SHADER, ARRAY_SIZE(BufferBarriers), BufferBarriers, 0, nullptr);

	drm::RenderTargetViewRef SurfaceView = drm::GetSurfaceView(ELoadAction::Clear, EStoreAction::Store, ClearColorValue{});
	drm::RenderTargetViewRef DepthView = drm::CreateRenderTargetView(
		SceneDepth,
		ELoadAction::Clear,
		EStoreAction::Store,
		ClearDepthStencilValue{},
		EImageLayout::DepthWriteStencilWrite);

	RenderPassInitializer RPInit = { 1 };
	RPInit.ColorTargets[0] = SurfaceView;
	RPInit.DepthTarget = DepthView;
	RPInit.RenderArea = RenderArea{ glm::ivec2(), glm::uvec2(SceneDepth->Width, SceneDepth->Height) };

	CmdList.BeginRenderPass(RPInit);

	drm::DescriptorSetRef DescriptorSet = drm::CreateDescriptorSet();
	DescriptorSet->Write(VoxelOrthoProjBuffer, ShaderBinding(0));
	DescriptorSet->Write(VoxelColors, ShaderBinding(1));
	DescriptorSet->Write(VoxelPositions, ShaderBinding(2));
	DescriptorSet->Update();

	const std::vector<drm::DescriptorSetRef> DescriptorSets =
	{
		Scene.DescriptorSet,
		DescriptorSet
	};

	CmdList.BindDescriptorSets(DescriptorSets.size(), DescriptorSets.data());

	PipelineStateInitializer PSOInit = {};
	PSOInit.Viewport.Width = gScreen.GetWidth();
	PSOInit.Viewport.Height = gScreen.GetHeight();
	PSOInit.DepthStencilState.DepthTestEnable = true;
	PSOInit.DepthStencilState.DepthWriteEnable = true;
	PSOInit.GraphicsPipelineState.Vertex = *ShaderMapRef<DrawVoxelsVS>();
	PSOInit.GraphicsPipelineState.Geometry = *ShaderMapRef<DrawVoxelsGS>();
	PSOInit.GraphicsPipelineState.Fragment = *ShaderMapRef<DrawVoxelsFS>();
	PSOInit.InputAssemblyState.Topology = EPrimitiveTopology::PointList;

	CmdList.BindPipeline(PSOInit);

	CmdList.Draw(gVoxelGridSize * gVoxelGridSize * gVoxelGridSize, 1, 0, 0);

	CmdList.EndRenderPass();
}