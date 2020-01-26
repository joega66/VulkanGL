#include "SceneRenderer.h"
#include "MaterialShader.h"
#include "MeshProxy.h"
#include "FullscreenQuad.h"

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
		Worker.SetDefine("VOXEL_GRID_SIZE", Platform::GetInt("Engine.ini", "Voxels", "VoxelGridSize", 256));
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
		Worker.SetDefine("VOXEL_GRID_SIZE", Platform::GetInt("Engine.ini", "Voxels", "VoxelGridSize", 256));
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
		ShaderMap.FindShader<VoxelsVS<MeshType>>(),
		nullptr,
		nullptr,
		ShaderMap.FindShader<VoxelsGS<MeshType>>(),
		ShaderMap.FindShader<VoxelsFS<MeshType>>()
	};

	VoxelsPass.push_back(MeshDrawCommand(std::move(ShaderStages), MeshProxy));
}

void SceneRenderer::RenderVoxels(SceneProxy& Scene, drm::CommandList& CmdList)
{
	// Set the shadow mask to black so that voxels don't have shadows.
	SceneTextures->Write(Material::Black, SamplerState{ EFilter::Nearest }, 1);
	SceneTextures->Update();

	const uint32 VoxelGridSize = Platform::GetInt("Engine.ini", "Voxels", "VoxelGridSize", 256);

	glm::mat4 OrthoProj = glm::ortho(-(float)VoxelGridSize * 0.5f, (float)VoxelGridSize * 0.5f, -(float)VoxelGridSize * 0.5f, (float)VoxelGridSize * 0.5f, 0.0f, (float)VoxelGridSize);
	OrthoProj[1][1] *= -1;

	struct WorldToVoxelUniform
	{
		glm::mat4 WorldToVoxel;
		glm::mat4 WorldToVoxelInv;
	} WorldToVoxelUniform;

	const glm::vec3 VoxelProbeCenter(
		Platform::GetFloat64("Engine.ini", "Voxels", "VoxelProbeCenterX", 0.0f),
		Platform::GetFloat64("Engine.ini", "Voxels", "VoxelProbeCenterY", 0.0f),
		Platform::GetFloat64("Engine.ini", "Voxels", "VoxelProbeCenterZ", 0.0f)
	);

	const float VoxelSize = static_cast<float>(Platform::GetFloat64("Engine.ini", "Voxels", "VoxelSize", 5.0f));

	WorldToVoxelUniform.WorldToVoxel = glm::scale(glm::mat4(), glm::vec3(1.0f / VoxelSize)) * OrthoProj * glm::translate(glm::mat4(), -VoxelProbeCenter);
	WorldToVoxelUniform.WorldToVoxelInv = glm::inverse(WorldToVoxelUniform.WorldToVoxel);

	WorldToVoxelBuffer = Device.CreateBuffer(EBufferUsage::Uniform, sizeof(WorldToVoxelUniform), &WorldToVoxelUniform);

	DrawIndirectCommand DrawIndirectCommand;
	DrawIndirectCommand.VertexCount = 0;
	DrawIndirectCommand.InstanceCount = 1;
	DrawIndirectCommand.FirstVertex = 0;
	DrawIndirectCommand.FirstInstance = 0;

	VoxelIndirectBuffer = Device.CreateBuffer(EBufferUsage::Storage | EBufferUsage::Indirect, sizeof(DrawIndirectCommand), &DrawIndirectCommand);

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
	const uint32 VoxelGridSize = Platform::GetInt("Engine.ini", "Voxels", "VoxelGridSize", 256);

	ImageMemoryBarrier ImageMemoryBarrier(
		VoxelColors, 
		EAccess::None, 
		EAccess::TransferWrite,
		EImageLayout::Undefined,
		EImageLayout::TransferDstOptimal
	);

	CmdList.PipelineBarrier(EPipelineStage::TopOfPipe, EPipelineStage::Transfer, 0, nullptr, 1, &ImageMemoryBarrier);

	CmdList.ClearColorImage(VoxelColors, EImageLayout::TransferDstOptimal, ClearColorValue{});

	ImageMemoryBarrier.SrcAccessMask = EAccess::TransferWrite;
	ImageMemoryBarrier.DstAccessMask = EAccess::ShaderWrite;
	ImageMemoryBarrier.OldLayout = EImageLayout::TransferDstOptimal;
	ImageMemoryBarrier.NewLayout = EImageLayout::General;

	CmdList.PipelineBarrier(EPipelineStage::Transfer, EPipelineStage::FragmentShader, 0, nullptr, 1, &ImageMemoryBarrier);

	CmdList.BeginRenderPass(VoxelRP);

	PipelineStateInitializer PSOInit = {};
	PSOInit.DepthStencilState.DepthTestEnable = false;
	PSOInit.DepthStencilState.DepthWriteEnable = false;
	PSOInit.Viewport.Width = VoxelGridSize;
	PSOInit.Viewport.Height = VoxelGridSize;

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
		Worker.SetDefine("VOXEL_GRID_SIZE", Platform::GetInt("Engine.ini", "Voxels", "VoxelGridSize", 256));
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
		Worker.SetDefine("VOXEL_GRID_SIZE", Platform::GetInt("Engine.ini", "Voxels", "VoxelGridSize", 256));
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
		Worker.SetDefine("VOXEL_GRID_SIZE", Platform::GetInt("Engine.ini", "Voxels", "VoxelGridSize", 256));
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo BaseInfo = { "../Shaders/DrawVoxels.glsl", "main", EShaderStage::Fragment };
		return BaseInfo;
	}
};

void SceneRenderer::RenderVoxelVisualization(SceneProxy& Scene, drm::CommandList& CmdList)
{
	std::vector<BufferMemoryBarrier> Barriers = 
	{
		{ VoxelPositions, EAccess::ShaderWrite, EAccess::ShaderRead }, 
		{ VoxelIndirectBuffer, EAccess::ShaderWrite, EAccess::IndirectCommandRead }
	};

	ImageMemoryBarrier ImageBarrier(VoxelColors, EAccess::ShaderWrite, EAccess::ShaderRead, EImageLayout::General, EImageLayout::General);

	CmdList.PipelineBarrier(
		EPipelineStage::FragmentShader, 
		EPipelineStage::DrawIndirect | EPipelineStage::VertexShader,
		Barriers.size(), Barriers.data(), 
		1, &ImageBarrier);

	CmdList.BeginRenderPass(VoxelVisualizationRP);

	drm::DescriptorSetRef DrawVoxelsDescriptorSet = Device.CreateDescriptorSet();
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

	const float VoxelSize = static_cast<float>(Platform::GetFloat64("Engine.ini", "Voxels", "VoxelSize", 5.0f));

	PipelineStateInitializer PSOInit = {};
	PSOInit.Viewport.Width = Scene.GetWidth();
	PSOInit.Viewport.Height = Scene.GetHeight();
	PSOInit.DepthStencilState.DepthTestEnable = true;
	PSOInit.DepthStencilState.DepthWriteEnable = true;
	PSOInit.ShaderStages.Vertex = Scene.ShaderMap.FindShader<DrawVoxelsVS>();
	PSOInit.ShaderStages.Geometry = Scene.ShaderMap.FindShader<DrawVoxelsGS>();
	PSOInit.ShaderStages.Fragment = Scene.ShaderMap.FindShader<DrawVoxelsFS>();
	PSOInit.InputAssemblyState.Topology = EPrimitiveTopology::PointList;
	PSOInit.SpecializationInfo.Add(0, VoxelSize);

	CmdList.BindPipeline(PSOInit);

	CmdList.DrawIndirect(VoxelIndirectBuffer, 0, 1);

	CmdList.EndRenderPass();
}