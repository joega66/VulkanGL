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
	const std::array<drm::DescriptorSetRef, 4> DescriptorSets = 
	{
		Pass.Scene,
		MeshProxy.MeshSet,
		MeshProxy.MaterialSet,
		Pass.Voxels
	};

	CmdList.BindDescriptorSets(DescriptorSets.size(), DescriptorSets.data());
}

void VoxelizationPass::SetPipelineState(PipelineStateInitializer& PSOInit, const MeshProxy& MeshProxy) const
{
	PSOInit.SpecializationInfo = MeshProxy.SpecializationInfo;

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
	glm::mat4 OrthoProj = glm::ortho(-(float)gVoxelGridSize * 0.5f, (float)gVoxelGridSize * 0.5f, -(float)gVoxelGridSize * 0.5f, (float)gVoxelGridSize * 0.5f, 0.0f, (float)gVoxelGridSize);
	OrthoProj[1][1] *= -1;

	struct WorldToVoxelUniform
	{
		glm::mat4 WorldToVoxel;
		glm::mat4 WorldToVoxelInv;
	} WorldToVoxelUniform;

	const glm::vec3 VoxelProbeCenter(-700, 500, 750);
	const float VoxelSize = 5.0f;

	WorldToVoxelUniform.WorldToVoxel = glm::scale(glm::mat4(), glm::vec3(1.0f / VoxelSize)) * OrthoProj * glm::translate(glm::mat4(), -VoxelProbeCenter);
	WorldToVoxelUniform.WorldToVoxelInv = glm::inverse(WorldToVoxelUniform.WorldToVoxel);

	WorldToVoxelBuffer = drm::CreateBuffer(EBufferUsage::Uniform, sizeof(WorldToVoxelUniform), &WorldToVoxelUniform);

	DrawIndirectCommand DrawIndirectCommand;
	DrawIndirectCommand.VertexCount = 0;
	DrawIndirectCommand.InstanceCount = 1;
	DrawIndirectCommand.FirstVertex = 0;
	DrawIndirectCommand.FirstInstance = 0;

	VoxelIndirectBuffer = drm::CreateBuffer(EBufferUsage::Storage | EBufferUsage::Indirect, sizeof(DrawIndirectCommand), &DrawIndirectCommand);

	VoxelsDescriptorSet->Write(WorldToVoxelBuffer, ShaderBinding(0));
	VoxelsDescriptorSet->Write(VoxelIndirectBuffer, ShaderBinding(3));
	VoxelsDescriptorSet->Update();

	RenderVoxelization(Scene, CmdList);

	RenderVoxelVisualization(Scene, CmdList);
}

void SceneRenderer::RenderVoxelization(SceneProxy& Scene, RenderCommandList& CmdList)
{
	ImageMemoryBarrier ImageMemoryBarrier(VoxelColors, EAccess::None, EAccess::TransferWrite, EImageLayout::TransferDstOptimal);

	CmdList.PipelineBarrier(EPipelineStage::TopOfPipe, EPipelineStage::Transfer, 0, nullptr, 1, &ImageMemoryBarrier);

	CmdList.ClearColorImage(VoxelColors, ClearColorValue{});

	ImageMemoryBarrier.SrcAccessMask = EAccess::TransferWrite;
	ImageMemoryBarrier.DstAccessMask = EAccess::ShaderWrite;
	ImageMemoryBarrier.NewLayout = EImageLayout::General;

	CmdList.PipelineBarrier(EPipelineStage::Transfer, EPipelineStage::FragmentShader, 0, nullptr, 1, &ImageMemoryBarrier);

	VoxelizationPass::PassDescriptors Descriptors = { Scene.DescriptorSet, VoxelsDescriptorSet };

	RenderPassInitializer RPInit = { 0 }; // Disable ROP
	RPInit.RenderArea.Extent = glm::uvec2(gVoxelGridSize);

	CmdList.BeginRenderPass(RPInit);

	PipelineStateInitializer PSOInit = {};
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
	BufferMemoryBarrier BufferBarrier(VoxelPositions, EAccess::ShaderWrite, EAccess::ShaderRead);
	ImageMemoryBarrier ImageBarrier(VoxelColors, EAccess::ShaderWrite, EAccess::ShaderRead, EImageLayout::General);

	CmdList.PipelineBarrier(EPipelineStage::FragmentShader, EPipelineStage::VertexShader, 1, &BufferBarrier, 1, &ImageBarrier);

	BufferMemoryBarrier VoxelIndirectBarrier(
		VoxelIndirectBuffer,
		EAccess::ShaderWrite,
		EAccess::IndirectCommandRead
	);

	CmdList.PipelineBarrier(EPipelineStage::FragmentShader, EPipelineStage::DrawIndirect, 1, &VoxelIndirectBarrier, 0, nullptr);

	drm::RenderTargetView SurfaceView(drm::GetSurface(), ELoadAction::Clear, EStoreAction::Store, ClearColorValue{}, EImageLayout::Present);
	drm::RenderTargetView DepthView(
		SceneDepth,
		ELoadAction::Clear,
		EStoreAction::Store,
		ClearDepthStencilValue{},
		EImageLayout::DepthWriteStencilWrite
	);

	RenderPassInitializer RPInit = { 1 };
	RPInit.ColorTargets[0] = SurfaceView;
	RPInit.DepthTarget = DepthView;
	RPInit.RenderArea = RenderArea{ glm::ivec2(), glm::uvec2(SceneDepth->Width, SceneDepth->Height) };

	CmdList.BeginRenderPass(RPInit);

	drm::DescriptorSetRef DrawVoxelsDescriptorSet = drm::CreateDescriptorSet();
	DrawVoxelsDescriptorSet->Write(WorldToVoxelBuffer, ShaderBinding(0));
	DrawVoxelsDescriptorSet->Write(VoxelColors, ShaderBinding(1));
	DrawVoxelsDescriptorSet->Write(VoxelPositions, ShaderBinding(2));
	DrawVoxelsDescriptorSet->Update();

	const std::vector<drm::DescriptorSetRef> DescriptorSets =
	{
		Scene.DescriptorSet,
		DrawVoxelsDescriptorSet
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

	CmdList.DrawIndirect(VoxelIndirectBuffer, 0, 1);

	CmdList.EndRenderPass();
}