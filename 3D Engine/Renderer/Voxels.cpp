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

void SceneProxy::AddToVoxelsPass(SceneRenderer& SceneRenderer, DRMDevice& Device, DRMShaderMap& ShaderMap, const MeshProxy& MeshProxy)
{
	static constexpr EMeshType MeshType = EMeshType::StaticMesh;

	PipelineStateDesc PSODesc = {};
	PSODesc.RenderPass = &SceneRenderer.VoxelRP;
	PSODesc.ShaderStages.Vertex = ShaderMap.FindShader<VoxelsVS<MeshType>>();
	PSODesc.ShaderStages.Geometry = ShaderMap.FindShader<VoxelsGS<MeshType>>();
	PSODesc.ShaderStages.Fragment = ShaderMap.FindShader<VoxelsFS<MeshType>>();
	PSODesc.DepthStencilState.DepthTestEnable = false;
	PSODesc.DepthStencilState.DepthWriteEnable = false;
	PSODesc.Viewport.Width = SceneRenderer.VoxelColors.GetWidth();
	PSODesc.Viewport.Height = SceneRenderer.VoxelColors.GetHeight();
	PSODesc.DescriptorSets = { SceneRenderer.CameraDescriptorSet, &MeshProxy.GetSurfaceSet(), &MeshProxy.GetMaterialSet(), SceneRenderer.SceneTexturesDescriptorSet, SceneRenderer.VoxelDescriptorSet };

	VoxelsPass.push_back(MeshDrawCommand(Device, MeshProxy, PSODesc));
}

void SceneRenderer::RenderVoxels(SceneProxy& Scene, drm::CommandList& CmdList)
{
	// Set the shadow mask to black so that voxels don't have shadows.
	SceneTexturesDescriptorSet.ShadowMask = drm::ImageView(Material::Black, Device.CreateSampler({ EFilter::Nearest }));
	SceneTexturesDescriptorSet.Update();

	const uint32 VoxelGridSize = Platform::GetInt("Engine.ini", "Voxels", "VoxelGridSize", 256);
	const glm::vec3 VoxelProbeCenter(
		Platform::GetFloat64("Engine.ini", "Voxels", "VoxelProbeCenterX", 0.0f),
		Platform::GetFloat64("Engine.ini", "Voxels", "VoxelProbeCenterY", 0.0f),
		Platform::GetFloat64("Engine.ini", "Voxels", "VoxelProbeCenterZ", 0.0f));
	const float VoxelSize = static_cast<float>(Platform::GetFloat64("Engine.ini", "Voxels", "VoxelSize", 5.0f));

	glm::mat4 OrthoProj = glm::ortho(-(float)VoxelGridSize * 0.5f, (float)VoxelGridSize * 0.5f, -(float)VoxelGridSize * 0.5f, (float)VoxelGridSize * 0.5f, 0.0f, (float)VoxelGridSize);
	OrthoProj[1][1] *= -1;

	WorldToVoxelUniform WorldToVoxelUniform;
	WorldToVoxelUniform.WorldToVoxel = glm::scale(glm::mat4(), glm::vec3(1.0f / VoxelSize)) * OrthoProj * glm::translate(glm::mat4(), -VoxelProbeCenter);
	WorldToVoxelUniform.WorldToVoxelInv = glm::inverse(WorldToVoxelUniform.WorldToVoxel);

	void* WorldToVoxelBufferPtr = Device.LockBuffer(WorldToVoxelBuffer);
	Platform::Memcpy(WorldToVoxelBufferPtr, &WorldToVoxelUniform, sizeof(WorldToVoxelUniform));
	Device.UnlockBuffer(WorldToVoxelBuffer);

	DrawIndirectCommand DrawIndirectCommand;
	DrawIndirectCommand.VertexCount = 0;
	DrawIndirectCommand.InstanceCount = 1;
	DrawIndirectCommand.FirstVertex = 0;
	DrawIndirectCommand.FirstInstance = 0;

	void* VoxelIndirectBufferPtr = Device.LockBuffer(VoxelIndirectBuffer);
	Platform::Memcpy(VoxelIndirectBufferPtr, &DrawIndirectCommand, sizeof(DrawIndirectCommand));
	Device.UnlockBuffer(VoxelIndirectBuffer);

	RenderVoxelization(Scene, CmdList);

	RenderVoxelVisualization(Scene, CmdList);
}

void SceneRenderer::RenderVoxelization(SceneProxy& Scene, drm::CommandList& CmdList)
{
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

	MeshDrawCommand::Draw(CmdList, Scene.VoxelsPass);

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
		1, &ImageBarrier
	);

	CmdList.BeginRenderPass(VoxelVisualizationRP);

	const float VoxelSize = static_cast<float>(Platform::GetFloat64("Engine.ini", "Voxels", "VoxelSize", 5.0f));

	PipelineStateDesc PSODesc = {};
	PSODesc.RenderPass = &VoxelVisualizationRP;
	PSODesc.Viewport.Width = SceneColor.GetWidth();
	PSODesc.Viewport.Height = SceneColor.GetHeight();
	PSODesc.DepthStencilState.DepthTestEnable = true;
	PSODesc.DepthStencilState.DepthWriteEnable = true;
	PSODesc.ShaderStages.Vertex = ShaderMap.FindShader<DrawVoxelsVS>();
	PSODesc.ShaderStages.Geometry = ShaderMap.FindShader<DrawVoxelsGS>();
	PSODesc.ShaderStages.Fragment = ShaderMap.FindShader<DrawVoxelsFS>();
	PSODesc.InputAssemblyState.Topology = EPrimitiveTopology::PointList;
	PSODesc.SpecializationInfo.Add(0, VoxelSize);
	PSODesc.DescriptorSets = { CameraDescriptorSet, VoxelDescriptorSet };

	drm::Pipeline Pipeline = Device.CreatePipeline(PSODesc);

	CmdList.BindPipeline(Pipeline);

	CmdList.BindDescriptorSets(Pipeline, PSODesc.DescriptorSets.size(), PSODesc.DescriptorSets.data());

	CmdList.DrawIndirect(VoxelIndirectBuffer, 0, 1);

	CmdList.EndRenderPass();
}