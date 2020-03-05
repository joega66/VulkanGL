#include "SceneRenderer.h"
#include "Voxels.h"
#include "MaterialShader.h"
#include "MeshProxy.h"
#include "FullscreenQuad.h"
#include "ShadowProxy.h"
#include "GlobalRenderResources.h"
#include <Engine/Engine.h>

struct VoxelDescriptors
{
	drm::BufferView WorldToVoxelBuffer;
	drm::ImageView VoxelBaseColor;
	drm::ImageView VoxelNormal;
	drm::ImageView VoxelRadiance;

	static const std::vector<DescriptorBinding>& GetBindings()
	{
		static const std::vector<DescriptorBinding> Bindings =
		{
			{ 0, 1, UniformBuffer },
			{ 1, 1, StorageImage },
			{ 2, 1, StorageImage },
			{ 3, 1, StorageImage }
		};
		return Bindings;
	}
};

struct DebugVoxelsDescriptors : public VoxelDescriptors
{
	drm::BufferView VoxelPositions;
	drm::BufferView VoxelIndirectBuffer;

	static std::vector<DescriptorBinding> GetBindings()
	{
		auto Bindings = VoxelDescriptors::GetBindings();
		Bindings.push_back({ 4, 1, StorageBuffer });
		Bindings.push_back({ 5, 1, StorageBuffer });
		return Bindings;
	}
};

static void SetVoxelEnvironmentVariables(ShaderCompilerWorker& Worker)
{
	Worker.SetDefine("VOXEL_GRID_SIZE", Platform::GetInt("Engine.ini", "Voxels", "VoxelGridSize", 256));
	Worker.SetDefine("DEBUG_VOXELS", Platform::GetBool("Engine.ini", "Voxels", "DebugVoxels", false));
}

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
		SetVoxelEnvironmentVariables(Worker);
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
		SetVoxelEnvironmentVariables(Worker);
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
		SetVoxelEnvironmentVariables(Worker);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo BaseInfo = { "../Shaders/VoxelsFS.glsl", "main", EShaderStage::Fragment };
		return BaseInfo;
	}
};

void SceneProxy::AddToVoxelsPass(DRMDevice& Device, DRMShaderMap& ShaderMap, const MeshProxy& MeshProxy)
{
	auto& GlobalResources = ECS.GetSingletonComponent<GlobalRenderResources>();

	static constexpr EMeshType MeshType = EMeshType::StaticMesh;

	VCTLightingCache& VCTLightingCache = GlobalResources.VCTLightingCache;

	PipelineStateDesc PSODesc = {};
	PSODesc.RenderPass = VCTLightingCache.GetRenderPass();
	PSODesc.ShaderStages.Vertex = ShaderMap.FindShader<VoxelsVS<MeshType>>();
	PSODesc.ShaderStages.Geometry = ShaderMap.FindShader<VoxelsGS<MeshType>>();
	PSODesc.ShaderStages.Fragment = ShaderMap.FindShader<VoxelsFS<MeshType>>();
	PSODesc.DepthStencilState.DepthTestEnable = false;
	PSODesc.DepthStencilState.DepthWriteEnable = false;
	PSODesc.Viewport.Width = VCTLightingCache.GetVoxelGridSize();
	PSODesc.Viewport.Height = VCTLightingCache.GetVoxelGridSize();
	PSODesc.DescriptorSets = {
		GlobalResources.CameraDescriptorSet,
		&MeshProxy.GetSurfaceSet(), 
		&MeshProxy.GetMaterialSet(),
		&VCTLightingCache.GetDescriptorSet()
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
		SetVoxelEnvironmentVariables(Worker);
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
		SetVoxelEnvironmentVariables(Worker);
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
		SetVoxelEnvironmentVariables(Worker);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo BaseInfo = { "../Shaders/DrawVoxels.glsl", "main", EShaderStage::Fragment };
		return BaseInfo;
	}
};

class LightInjectionCS : public drm::Shader
{
public:
	LightInjectionCS(const ShaderCompilationInfo& CompilationInfo)
		: drm::Shader(CompilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		SetVoxelEnvironmentVariables(Worker);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo BaseInfo = { "../Shaders/LightInjectionCS.glsl", "main", EShaderStage::Compute };
		return BaseInfo;
	}
};

UNIFORM_STRUCT(WorldToVoxelUniform,
	glm::mat4 WorldToVoxel;
	glm::mat4 WorldToVoxelInv;
);

VCTLightingCache::VCTLightingCache(Engine& Engine)
	: VoxelGridSize(Platform::GetInt("Engine.ini", "Voxels", "VoxelGridSize", 256))
	, DebugVoxels(Platform::GetBool("Engine.ini", "Voxels", "DebugVoxels", false))
	, Device(Engine.Device)
	, ShaderMap(Engine.ShaderMap)
{
	check(VoxelGridSize <= 1024, "Exceeded voxel bits.");

	WorldToVoxelBuffer = Device.CreateBuffer(EBufferUsage::Uniform | EBufferUsage::HostVisible, sizeof(WorldToVoxelUniform));

	VoxelBaseColor = Device.CreateImage(VoxelGridSize, VoxelGridSize, VoxelGridSize, EFormat::R8G8B8A8_UNORM, EImageUsage::Storage | EImageUsage::Sampled);
	VoxelNormal = Device.CreateImage(VoxelGridSize, VoxelGridSize, VoxelGridSize, EFormat::R16G16B16A16_SFLOAT, EImageUsage::Storage | EImageUsage::Sampled);
	VoxelRadiance = Device.CreateImage(VoxelGridSize, VoxelGridSize, VoxelGridSize, EFormat::R8G8B8A8_UNORM, EImageUsage::Storage | EImageUsage::Sampled | EImageUsage::TransferDst);

	auto Bindings = DebugVoxels ? DebugVoxelsDescriptors::GetBindings() : VoxelDescriptors::GetBindings();

	DescriptorSetLayout = Device.CreateDescriptorSetLayout(Bindings.size(), Bindings.data());
	VoxelDescriptorSet = DescriptorSetLayout.CreateDescriptorSet();

	if (DebugVoxels)
	{
		VoxelPositions = Device.CreateBuffer(EBufferUsage::Storage, VoxelGridSize * VoxelGridSize * VoxelGridSize * sizeof(int32));
		VoxelIndirectBuffer = Device.CreateBuffer(EBufferUsage::Storage | EBufferUsage::Indirect | EBufferUsage::HostVisible, sizeof(DrawIndirectCommand));

		DebugVoxelsDescriptors Descriptors;
		Descriptors.WorldToVoxelBuffer = WorldToVoxelBuffer;
		Descriptors.VoxelBaseColor = VoxelBaseColor;
		Descriptors.VoxelNormal = VoxelNormal;
		Descriptors.VoxelRadiance = VoxelRadiance;
		Descriptors.VoxelPositions = VoxelPositions;
		Descriptors.VoxelIndirectBuffer = VoxelIndirectBuffer;
		DescriptorSetLayout.UpdateDescriptorSet(VoxelDescriptorSet, &Descriptors);
	}
	else
	{
		VoxelDescriptors Descriptors;
		Descriptors.WorldToVoxelBuffer = WorldToVoxelBuffer;
		Descriptors.VoxelBaseColor = VoxelBaseColor;
		Descriptors.VoxelNormal = VoxelNormal;
		Descriptors.VoxelRadiance = VoxelRadiance;
		DescriptorSetLayout.UpdateDescriptorSet(VoxelDescriptorSet, &Descriptors);
	}

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

	RenderVoxels(Scene, CmdList);

	ComputeLightInjection(Scene, CmdList);
}

void VCTLightingCache::RenderVoxels(SceneProxy& Scene, drm::CommandList& CmdList)
{
	ImageMemoryBarrier ImageBarrier(
		VoxelRadiance,
		EAccess::None,
		EAccess::TransferWrite,
		EImageLayout::Undefined,
		EImageLayout::TransferDstOptimal
	);

	CmdList.PipelineBarrier(EPipelineStage::TopOfPipe, EPipelineStage::Transfer, 0, nullptr, 1, &ImageBarrier);

	CmdList.ClearColorImage(VoxelRadiance, EImageLayout::TransferDstOptimal, ClearColorValue{});

	const std::vector<ImageMemoryBarrier> ImageBarriers =
	{
		{ VoxelRadiance, EAccess::TransferWrite, EAccess::ShaderWrite, EImageLayout::TransferDstOptimal, EImageLayout::General },
		{ VoxelBaseColor, EAccess::None, EAccess::ShaderWrite, EImageLayout::Undefined, EImageLayout::General },
		{ VoxelNormal, EAccess::None, EAccess::ShaderWrite, EImageLayout::Undefined, EImageLayout::General }
	};

	CmdList.PipelineBarrier(EPipelineStage::Transfer, EPipelineStage::FragmentShader, 0, nullptr, ImageBarriers.size(), ImageBarriers.data());

	CmdList.BeginRenderPass(VoxelRP);

	MeshDrawCommand::Draw(CmdList, Scene.VoxelsPass);

	CmdList.EndRenderPass();
}

void VCTLightingCache::ComputeLightInjection(SceneProxy& SceneProxy, drm::CommandList& CmdList)
{
	auto& ECS = SceneProxy.ECS;

	struct LightInjectionDescriptors
	{
		drm::ImageView ShadowMap;
		drm::BufferView LightProjBuffer;

		static const auto& GetBindings()
		{
			static const std::vector<DescriptorBinding> Bindings =
			{
				{ 0, 1, StorageImage },
				{ 1, 1, UniformBuffer },
			};
			return Bindings;
		}
	};

	auto& GlobalResources = ECS.GetSingletonComponent<GlobalRenderResources>();

	for (auto Entity : ECS.GetEntities<ShadowProxy>())
	{
		const auto& ShadowProxy = ECS.GetComponent<class ShadowProxy>(Entity);
		const drm::Image& ShadowMap = ShadowProxy.GetShadowMap();

		DescriptorSet<LightInjectionDescriptors> LightInjectionSet(Device);
		LightInjectionSet.ShadowMap = ShadowMap;
		LightInjectionSet.LightProjBuffer = ShadowProxy.GetLightViewProjBuffer();
		LightInjectionSet.Update();

		ComputePipelineDesc ComputeDesc = {};
		ComputeDesc.ComputeShader = ShaderMap.FindShader<LightInjectionCS>();
		ComputeDesc.DescriptorSets = { GlobalResources.CameraDescriptorSet, &VoxelDescriptorSet, LightInjectionSet };

		drm::Pipeline Pipeline = Device.CreatePipeline(ComputeDesc);

		CmdList.BindPipeline(Pipeline);

		CmdList.BindDescriptorSets(Pipeline, ComputeDesc.DescriptorSets.size(), ComputeDesc.DescriptorSets.data());
		
		const uint32 GroupCountX = DivideAndRoundUp(ShadowMap.GetWidth(), 8U);
		const uint32 GroupCountY = DivideAndRoundUp(ShadowMap.GetHeight(), 8U);
		CmdList.Dispatch(GroupCountX, GroupCountY, 1);
	}
}

void VCTLightingCache::RenderVisualization(SceneProxy& Scene, drm::CommandList& CmdList)
{
	DrawIndirectCommand DrawIndirectCommand;
	DrawIndirectCommand.VertexCount = 0;
	DrawIndirectCommand.InstanceCount = 1;
	DrawIndirectCommand.FirstVertex = 0;
	DrawIndirectCommand.FirstInstance = 0;

	void* VoxelIndirectBufferPtr = Device.LockBuffer(VoxelIndirectBuffer);
	Platform::Memcpy(VoxelIndirectBufferPtr, &DrawIndirectCommand, sizeof(DrawIndirectCommand));
	Device.UnlockBuffer(VoxelIndirectBuffer);

	std::vector<BufferMemoryBarrier> BufferBarriers =
	{
		{ VoxelPositions, EAccess::ShaderWrite, EAccess::ShaderRead },
		{ VoxelIndirectBuffer, EAccess::ShaderWrite, EAccess::IndirectCommandRead }
	};

	std::vector<const drm::Image*> Images = { &VoxelBaseColor, &VoxelNormal };
	std::vector<ImageMemoryBarrier> ImageBarriers;

	for (auto Image : Images)
	{
		ImageBarriers.push_back({ *Image, EAccess::ShaderWrite, EAccess::ShaderRead, EImageLayout::General, EImageLayout::General });
	}

	ImageBarriers.push_back(ImageMemoryBarrier(VoxelRadiance, EAccess::ShaderWrite, EAccess::ShaderRead, EImageLayout::General, EImageLayout::General));

	CmdList.PipelineBarrier(
		EPipelineStage::FragmentShader | EPipelineStage::ComputeShader,
		EPipelineStage::DrawIndirect | EPipelineStage::VertexShader,
		BufferBarriers.size(), BufferBarriers.data(),
		ImageBarriers.size(), ImageBarriers.data()
	);

	CmdList.BeginRenderPass(DebugRP);

	const float VoxelSize = static_cast<float>(Platform::GetFloat64("Engine.ini", "Voxels", "VoxelSize", 5.0f));

	auto& GlobalResources = Scene.ECS.GetSingletonComponent<GlobalRenderResources>();

	PipelineStateDesc PSODesc = {};
	PSODesc.RenderPass = DebugRP;
	PSODesc.Viewport.Width = GlobalResources.SceneColor.GetWidth();
	PSODesc.Viewport.Height = GlobalResources.SceneColor.GetHeight();
	PSODesc.DepthStencilState.DepthTestEnable = true;
	PSODesc.DepthStencilState.DepthWriteEnable = true;
	PSODesc.ShaderStages.Vertex = ShaderMap.FindShader<DrawVoxelsVS>();
	PSODesc.ShaderStages.Geometry = ShaderMap.FindShader<DrawVoxelsGS>();
	PSODesc.ShaderStages.Fragment = ShaderMap.FindShader<DrawVoxelsFS>();
	PSODesc.InputAssemblyState.Topology = EPrimitiveTopology::PointList;
	PSODesc.SpecializationInfo.Add(0, VoxelSize);
	PSODesc.DescriptorSets = { GlobalResources.CameraDescriptorSet, &VoxelDescriptorSet };

	drm::Pipeline Pipeline = Device.CreatePipeline(PSODesc);

	CmdList.BindPipeline(Pipeline);

	CmdList.BindDescriptorSets(Pipeline, PSODesc.DescriptorSets.size(), PSODesc.DescriptorSets.data());

	CmdList.DrawIndirect(VoxelIndirectBuffer, 0, 1);
}

void VCTLightingCache::CreateDebugRenderPass(const drm::Image& SceneColor, const drm::Image& SceneDepth)
{
	RenderPassDesc RPDesc = {};
	RPDesc.ColorAttachments.push_back(drm::AttachmentView(
		&SceneColor,
		ELoadAction::Clear,
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
	DebugRP = Device.CreateRenderPass(RPDesc);
}

void VCTLightingCache::CreateVoxelRP()
{
	RenderPassDesc RPDesc = {}; // Disable ROP
	RPDesc.RenderArea.Extent = glm::uvec2(GetVoxelGridSize());
	VoxelRP = Device.CreateRenderPass(RPDesc);
}