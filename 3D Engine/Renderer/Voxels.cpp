#include "SceneRenderer.h"
#include "MaterialShader.h"
#include "MeshProxy.h"
#include "FullscreenQuad.h"
#include "Voxels.h"
#include <Engine/Engine.h>

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

	VCTLightingCache& VCTLightingCache = SceneRenderer.VCTLightingCache;

	PipelineStateDesc PSODesc = {};
	PSODesc.RenderPass = &VCTLightingCache.GetRenderPass();
	PSODesc.ShaderStages.Vertex = ShaderMap.FindShader<VoxelsVS<MeshType>>();
	PSODesc.ShaderStages.Geometry = ShaderMap.FindShader<VoxelsGS<MeshType>>();
	PSODesc.ShaderStages.Fragment = ShaderMap.FindShader<VoxelsFS<MeshType>>();
	PSODesc.DepthStencilState.DepthTestEnable = false;
	PSODesc.DepthStencilState.DepthWriteEnable = false;
	PSODesc.Viewport.Width = VCTLightingCache.GetVoxelGridSize();
	PSODesc.Viewport.Height = VCTLightingCache.GetVoxelGridSize();
	PSODesc.DescriptorSets = {
		SceneRenderer.CameraDescriptorSet, 
		&MeshProxy.GetSurfaceSet(), 
		&MeshProxy.GetMaterialSet(), 
		SceneRenderer.SceneTexturesDescriptorSet, 
		VCTLightingCache.GetDescriptorSet()
	};

	VoxelsPass.push_back(MeshDrawCommand(Device, MeshProxy, PSODesc));
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

UNIFORM_STRUCT(WorldToVoxelUniform,
	glm::mat4 WorldToVoxel;
	glm::mat4 WorldToVoxelInv;
);

VCTLightingCache::VCTLightingCache(Engine& Engine)
	: VoxelGridSize(Platform::GetInt("Engine.ini", "Voxels", "VoxelGridSize", 256))
	, Device(Engine.Device)
	, ShaderMap(Engine.ShaderMap)
	, VoxelDescriptorSet(Engine.Device)
{
	check(VoxelGridSize <= 1024, "Exceeded voxel bits.");

	WorldToVoxelBuffer = Device.CreateBuffer(EBufferUsage::Uniform | EBufferUsage::HostVisible, sizeof(WorldToVoxelUniform));
	VoxelColors = Device.CreateImage(VoxelGridSize, VoxelGridSize, VoxelGridSize, EFormat::R8G8B8A8_UNORM, EImageUsage::Storage);
	VoxelPositions = Device.CreateBuffer(EBufferUsage::Storage, VoxelGridSize * VoxelGridSize * VoxelGridSize * sizeof(int32));
	VoxelIndirectBuffer = Device.CreateBuffer(EBufferUsage::Storage | EBufferUsage::Indirect | EBufferUsage::HostVisible, sizeof(DrawIndirectCommand));

	VoxelDescriptorSet.WorldToVoxelBuffer = WorldToVoxelBuffer;
	VoxelDescriptorSet.VoxelColors = VoxelColors;
	VoxelDescriptorSet.VoxelPositions = VoxelPositions;
	VoxelDescriptorSet.VoxelIndirectBuffer = VoxelIndirectBuffer;
	VoxelDescriptorSet.Update();

	CreateVoxelRP();
}

void VCTLightingCache::Render(SceneProxy& Scene, drm::CommandList& CmdList)
{
	const glm::vec3 VoxelProbeCenter(
		Platform::GetFloat64("Engine.ini", "Voxels", "VoxelProbeCenterX", 0.0f),
		Platform::GetFloat64("Engine.ini", "Voxels", "VoxelProbeCenterY", 0.0f),
		Platform::GetFloat64("Engine.ini", "Voxels", "VoxelProbeCenterZ", 0.0f));
	const float VoxelSize = static_cast<float>(Platform::GetFloat64("Engine.ini", "Voxels", "VoxelSize", 5.0f));

	glm::mat4 OrthoProj = glm::ortho(
		-(float)VoxelGridSize * 0.5f, (float)VoxelGridSize * 0.5f, 
		-(float)VoxelGridSize * 0.5f, (float)VoxelGridSize * 0.5f, 
		0.0f, (float)VoxelGridSize);
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

	RenderVoxels(Scene, CmdList);
}

void VCTLightingCache::Resize(const drm::Image& SceneColor, const drm::Image& SceneDepth, const drm::DescriptorSet* CameraDescriptorSet)
{
	CreateVoxelVisualizationRP(SceneColor, SceneDepth, CameraDescriptorSet);
}

void VCTLightingCache::RenderVoxels(SceneProxy& Scene, drm::CommandList& CmdList)
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

void VCTLightingCache::RenderVisualization(SceneRenderer& SceneRenderer, drm::CommandList& CmdList)
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

	CmdList.BindPipeline(VoxelVisualizationPipeline);

	std::vector<const drm::DescriptorSet*> DescriptorSets = { SceneRenderer.CameraDescriptorSet, VoxelDescriptorSet };

	CmdList.BindDescriptorSets(VoxelVisualizationPipeline, DescriptorSets.size(), DescriptorSets.data());

	CmdList.DrawIndirect(VoxelIndirectBuffer, 0, 1);

	CmdList.EndRenderPass();
}

void VCTLightingCache::CreateVoxelRP()
{
	RenderPassDesc RPDesc = {}; // Disable ROP
	RPDesc.RenderArea.Extent = glm::uvec2(GetVoxelGridSize());
	VoxelRP = Device.CreateRenderPass(RPDesc);
}

void VCTLightingCache::CreateVoxelVisualizationRP(const drm::Image& SceneColor, const drm::Image& SceneDepth, const drm::DescriptorSet* CameraDescriptorSet)
{
	RenderPassDesc RPDesc = {};
	RPDesc.ColorAttachments.push_back(drm::AttachmentView(
		&SceneColor,
		ELoadAction::DontCare,
		EStoreAction::Store,
		ClearColorValue{},
		EImageLayout::Undefined,
		EImageLayout::TransferSrcOptimal));
	RPDesc.DepthAttachment = drm::AttachmentView(
		&SceneDepth,
		ELoadAction::Clear,
		EStoreAction::Store,
		ClearDepthStencilValue{},
		EImageLayout::Undefined,
		EImageLayout::DepthWriteStencilWrite);
	RPDesc.RenderArea = RenderArea{ glm::ivec2(), glm::uvec2(SceneDepth.GetWidth(), SceneDepth.GetHeight()) };
	VoxelVisualizationRP = Device.CreateRenderPass(RPDesc);

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

	VoxelVisualizationPipeline = Device.CreatePipeline(PSODesc);
}