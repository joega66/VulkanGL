#include "SceneRenderer.h"
#include "MaterialShader.h"
#include "MeshProxy.h"
#include "FullscreenQuad.h"
#include <Engine/Screen.h>

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

void SceneProxy::AddToVoxelsPass(const MeshProxy& MeshProxy)
{
	static constexpr EMeshType MeshType = EMeshType::StaticMesh;

	ShaderStages ShaderStages =
	{
		*ShaderMapRef<VoxelsVS<MeshType>>(),
		nullptr,
		nullptr,
		*ShaderMapRef<VoxelsGS<MeshType>>(),
		*ShaderMapRef<VoxelsFS<MeshType>>()
	};

	VoxelsPass.push_back(MeshDrawCommand(std::move(ShaderStages), MeshProxy));
}

void SceneRenderer::RenderVoxels(SceneProxy& Scene, drm::CommandList& CmdList)
{
	glm::mat4 OrthoProj = glm::ortho(-(float)gVoxelGridSize * 0.5f, (float)gVoxelGridSize * 0.5f, -(float)gVoxelGridSize * 0.5f, (float)gVoxelGridSize * 0.5f, 0.0f, (float)gVoxelGridSize);
	OrthoProj[1][1] *= -1;

	struct WorldToVoxelUniform
	{
		glm::mat4 WorldToVoxel;
		glm::mat4 WorldToVoxelInv;
	} WorldToVoxelUniform;

	const glm::vec3 VoxelProbeCenter(
		Platform.GetFloat64("Engine.ini", "Voxels", "VoxelProbeCenterX", 0.0f),
		Platform.GetFloat64("Engine.ini", "Voxels", "VoxelProbeCenterY", 0.0f),
		Platform.GetFloat64("Engine.ini", "Voxels", "VoxelProbeCenterZ", 0.0f)
	);

	const float VoxelSize = static_cast<float>(Platform.GetFloat64("Engine.ini", "Voxels", "VoxelSize", 5.0f));

	WorldToVoxelUniform.WorldToVoxel = glm::scale(glm::mat4(), glm::vec3(1.0f / VoxelSize)) * OrthoProj * glm::translate(glm::mat4(), -VoxelProbeCenter);
	WorldToVoxelUniform.WorldToVoxelInv = glm::inverse(WorldToVoxelUniform.WorldToVoxel);

	WorldToVoxelBuffer = drm::CreateBuffer(EBufferUsage::Uniform, sizeof(WorldToVoxelUniform), &WorldToVoxelUniform);

	DrawIndirectCommand DrawIndirectCommand;
	DrawIndirectCommand.VertexCount = 0;
	DrawIndirectCommand.InstanceCount = 1;
	DrawIndirectCommand.FirstVertex = 0;
	DrawIndirectCommand.FirstInstance = 0;

	VoxelIndirectBuffer = drm::CreateBuffer(EBufferUsage::Storage | EBufferUsage::Indirect, sizeof(DrawIndirectCommand), &DrawIndirectCommand);

	VoxelsDescriptorSet->Write(WorldToVoxelBuffer, 0);
	VoxelsDescriptorSet->Write(VoxelIndirectBuffer, 3);
	VoxelsDescriptorSet->Update();

	RenderVoxelization(Scene, CmdList);

	RenderVoxelVisualization(Scene, CmdList);
}

struct VoxelizationPassDescriptorSets
{
	drm::DescriptorSetRef Scene;
	drm::DescriptorSetRef SceneTextures;
	drm::DescriptorSetRef Voxels;

	void Set(drm::CommandList& CmdList, const MeshProxy& MeshProxy) const
	{
		const std::array<drm::DescriptorSetRef, 5> DescriptorSets =
		{
			Scene,
			MeshProxy.MeshSet,
			MeshProxy.MaterialSet,
			SceneTextures,
			Voxels
		};

		CmdList.BindDescriptorSets(DescriptorSets.size(), DescriptorSets.data());
	}
};

void SceneRenderer::RenderVoxelization(SceneProxy& Scene, drm::CommandList& CmdList)
{
	ImageMemoryBarrier ImageMemoryBarrier(VoxelColors, EAccess::None, EAccess::TransferWrite, EImageLayout::TransferDstOptimal);

	CmdList.PipelineBarrier(EPipelineStage::TopOfPipe, EPipelineStage::Transfer, 0, nullptr, 1, &ImageMemoryBarrier);

	CmdList.ClearColorImage(VoxelColors, ClearColorValue{});

	ImageMemoryBarrier.SrcAccessMask = EAccess::TransferWrite;
	ImageMemoryBarrier.DstAccessMask = EAccess::ShaderWrite;
	ImageMemoryBarrier.NewLayout = EImageLayout::General;

	CmdList.PipelineBarrier(EPipelineStage::Transfer, EPipelineStage::FragmentShader, 0, nullptr, 1, &ImageMemoryBarrier);

	RenderPassInitializer RPInit = { 0 }; // Disable ROP
	RPInit.RenderArea.Extent = glm::uvec2(gVoxelGridSize);

	CmdList.BeginRenderPass(RPInit);

	PipelineStateInitializer PSOInit = {};
	PSOInit.DepthStencilState.DepthTestEnable = false;
	PSOInit.DepthStencilState.DepthWriteEnable = false;
	PSOInit.Viewport.Width = gVoxelGridSize;
	PSOInit.Viewport.Height = gVoxelGridSize;

	VoxelizationPassDescriptorSets DescriptorSets = { Scene.DescriptorSet, SceneTextures, VoxelsDescriptorSet };

	MeshDrawCommand::Draw(Scene.VoxelsPass, CmdList, DescriptorSets, PSOInit);

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

void SceneRenderer::RenderVoxelVisualization(SceneProxy& Scene, drm::CommandList& CmdList)
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

	drm::AttachmentView SurfaceView(drm::GetSurface(), ELoadAction::Clear, EStoreAction::Store, ClearColorValue{}, EImageLayout::Present);
	drm::AttachmentView DepthView(
		SceneDepth,
		ELoadAction::Clear,
		EStoreAction::Store,
		ClearDepthStencilValue{},
		EImageLayout::DepthWriteStencilWrite
	);

	RenderPassInitializer RPInit = { 1 };
	RPInit.ColorAttachments[0] = SurfaceView;
	RPInit.DepthAttachment = DepthView;
	RPInit.RenderArea = RenderArea{ glm::ivec2(), glm::uvec2(SceneDepth->Width, SceneDepth->Height) };

	CmdList.BeginRenderPass(RPInit);

	drm::DescriptorSetRef DrawVoxelsDescriptorSet = drm::CreateDescriptorSet();
	DrawVoxelsDescriptorSet->Write(WorldToVoxelBuffer, 0);
	DrawVoxelsDescriptorSet->Write(VoxelColors, 1);
	DrawVoxelsDescriptorSet->Write(VoxelPositions, 2);
	DrawVoxelsDescriptorSet->Update();

	const std::vector<drm::DescriptorSetRef> DescriptorSets =
	{
		Scene.DescriptorSet,
		DrawVoxelsDescriptorSet
	};

	CmdList.BindDescriptorSets(DescriptorSets.size(), DescriptorSets.data());

	const float VoxelSize = static_cast<float>(Platform.GetFloat64("Engine.ini", "Voxels", "VoxelSize", 5.0f));

	PipelineStateInitializer PSOInit = {};
	PSOInit.Viewport.Width = gScreen.GetWidth();
	PSOInit.Viewport.Height = gScreen.GetHeight();
	PSOInit.DepthStencilState.DepthTestEnable = true;
	PSOInit.DepthStencilState.DepthWriteEnable = true;
	PSOInit.ShaderStages.Vertex = *ShaderMapRef<DrawVoxelsVS>();
	PSOInit.ShaderStages.Geometry = *ShaderMapRef<DrawVoxelsGS>();
	PSOInit.ShaderStages.Fragment = *ShaderMapRef<DrawVoxelsFS>();
	PSOInit.InputAssemblyState.Topology = EPrimitiveTopology::PointList;
	PSOInit.SpecializationInfo.Add(0, VoxelSize);

	CmdList.BindPipeline(PSOInit);

	CmdList.DrawIndirect(VoxelIndirectBuffer, 0, 1);

	CmdList.EndRenderPass();
}