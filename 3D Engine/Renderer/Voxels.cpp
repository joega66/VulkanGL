#include "Voxels.h"
#include "SceneRenderer.h"
#include "MaterialShader.h"
#include "MeshProxy.h"
#include "FullscreenQuad.h"
#include "ShadowProxy.h"
#include "GlobalRenderData.h"
#include <Engine/Engine.h>
#include <Components/RenderSettings.h>

struct VoxelDescriptors
{
	drm::DescriptorBufferInfo WorldToVoxelBuffer;
	drm::DescriptorImageInfo VoxelBaseColor;
	drm::DescriptorImageInfo VoxelNormal;
	drm::DescriptorImageInfo VoxelRadiance;

	static const std::vector<DescriptorBinding>& GetBindings()
	{
		static const std::vector<DescriptorBinding> Bindings =
		{
			{ 0, 1, EDescriptorType::UniformBuffer },
			{ 1, 1, EDescriptorType::StorageImage },
			{ 2, 1, EDescriptorType::StorageImage },
			{ 3, 1, EDescriptorType::StorageImage }
		};
		return Bindings;
	}
};

struct DebugVoxelsDescriptors : public VoxelDescriptors
{
	drm::DescriptorBufferInfo VoxelPositions;
	drm::DescriptorBufferInfo VoxelIndirectBuffer;

	static std::vector<DescriptorBinding> GetBindings()
	{
		auto Bindings = VoxelDescriptors::GetBindings();
		Bindings.push_back({ 4, 1, EDescriptorType::StorageBuffer });
		Bindings.push_back({ 5, 1, EDescriptorType::StorageBuffer });
		return Bindings;
	}
};

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
		VoxelShader::SetEnvironmentVariables(Worker);
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
		VoxelShader::SetEnvironmentVariables(Worker);
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
		VoxelShader::SetEnvironmentVariables(Worker);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo BaseInfo = { "../Shaders/VoxelsFS.glsl", "main", EShaderStage::Fragment };
		return BaseInfo;
	}
};

void SceneProxy::AddToVoxelsPass(DRMDevice& Device, DRMShaderMap& ShaderMap, const MeshProxy& MeshProxy)
{
	auto& GlobalData = ECS.GetSingletonComponent<GlobalRenderData>();

	static constexpr EMeshType MeshType = EMeshType::StaticMesh;

	VCTLightingCache& VCTLightingCache = GlobalData.VCTLightingCache;

	PipelineStateDesc PSODesc = {};
	PSODesc.RenderPass = VCTLightingCache.GetRenderPass();
	PSODesc.ShaderStages.Vertex = ShaderMap.FindShader<VoxelsVS<MeshType>>();
	PSODesc.ShaderStages.Geometry = ShaderMap.FindShader<VoxelsGS<MeshType>>();
	PSODesc.ShaderStages.Fragment = ShaderMap.FindShader<VoxelsFS<MeshType>>();
	PSODesc.DepthStencilState.DepthTestEnable = false;
	PSODesc.DepthStencilState.DepthWriteEnable = false;
	PSODesc.Viewport.Width = VCTLightingCache.GetVoxelGridSize();
	PSODesc.Viewport.Height = VCTLightingCache.GetVoxelGridSize();
	PSODesc.Layouts = {
		GlobalData.CameraDescriptorSet.GetLayout(),
		MeshProxy.GetSurfaceSet().GetLayout(),
		MeshProxy.GetMaterialSet().GetLayout(),
		VCTLightingCache.GetDescriptorSet().GetLayout()
	};

	const std::vector<VkDescriptorSet> DescriptorSets =
	{
		GlobalData.CameraDescriptorSet,
		MeshProxy.GetSurfaceSet(),
		MeshProxy.GetMaterialSet(),
		VCTLightingCache.GetDescriptorSet()
	};

	VoxelsPass.push_back(MeshDrawCommand(Device, MeshProxy, PSODesc, DescriptorSets));
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
		VoxelShader::SetEnvironmentVariables(Worker);
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
		VoxelShader::SetEnvironmentVariables(Worker);
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
		VoxelShader::SetEnvironmentVariables(Worker);
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
		VoxelShader::SetEnvironmentVariables(Worker);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo BaseInfo = { "../Shaders/LightInjectionCS.glsl", "main", EShaderStage::Compute };
		return BaseInfo;
	}
};

class DownsampleVolumeCS : public drm::Shader
{
public:
	DownsampleVolumeCS(const ShaderCompilationInfo& CompilationInfo)
		: drm::Shader(CompilationInfo)
	{
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo BaseInfo = { "../Shaders/DownsampleVolumeCS.glsl", "main", EShaderStage::Compute };
		return BaseInfo;
	}
};

UNIFORM_STRUCT(WorldToVoxelUniform,
	glm::mat4 WorldToVoxel;
	glm::mat4 WorldToVoxelInv;
);

VCTLightingCache::VCTLightingCache(Engine& Engine)
	: VoxelSize(static_cast<float>(Platform::GetFloat64("Engine.ini", "Voxels", "VoxelSize", 5.0f)))
	, VoxelGridSize(Platform::GetInt("Engine.ini", "Voxels", "VoxelGridSize", 256))
	, DebugVoxels(Platform::GetBool("Engine.ini", "Voxels", "DebugVoxels", false))
	, Device(Engine.Device)
	, ShaderMap(Engine.ShaderMap)
	, DownsampleVolumeSetLayout(Engine.Device)
{
	check(VoxelGridSize <= 1024, "Exceeded voxel bits.");
	
	WorldToVoxelBuffer = Device.CreateBuffer(EBufferUsage::Uniform | EBufferUsage::HostVisible, sizeof(WorldToVoxelUniform));

	VoxelBaseColor = Device.CreateImage(VoxelGridSize, VoxelGridSize, VoxelGridSize, EFormat::R8G8B8A8_UNORM, EImageUsage::Storage | EImageUsage::Sampled);
	VoxelNormal = Device.CreateImage(VoxelGridSize, VoxelGridSize, VoxelGridSize, EFormat::R16G16B16A16_SFLOAT, EImageUsage::Storage | EImageUsage::Sampled);
	VoxelRadiance = Device.CreateImage(
		VoxelGridSize, VoxelGridSize, VoxelGridSize, 
		EFormat::R8G8B8A8_UNORM, 
		EImageUsage::Storage | EImageUsage::Sampled | EImageUsage::TransferDst,
		Platform::GetInt("Engine.ini", "Voxels", "MipLevels", 3));
	VoxelRadianceSampler = Device.CreateSampler(
		{ EFilter::Linear, ESamplerAddressMode::ClampToEdge, ESamplerMipmapMode::Nearest, 0.0f, static_cast<float>(VoxelRadiance.GetMipLevels()) });

	VoxelRadianceMipMaps.reserve(VoxelRadiance.GetMipLevels());
	for (uint32 LevelIndex = 0; LevelIndex < VoxelRadiance.GetMipLevels(); LevelIndex++)
	{
		VoxelRadianceMipMaps.push_back(Device.CreateImageView(VoxelRadiance, LevelIndex, 1, 0, 1));
	}

	auto Bindings = DebugVoxels ? DebugVoxelsDescriptors::GetBindings() : VoxelDescriptors::GetBindings();

	VoxelSetLayout = Device.CreateDescriptorSetLayout(Bindings.size(), Bindings.data());
	VoxelDescriptorSet = VoxelSetLayout.CreateDescriptorSet(Device);

	if (DebugVoxels)
	{
		VoxelPositions = Device.CreateBuffer(EBufferUsage::Storage, uint64(VoxelGridSize) * uint64(VoxelGridSize) * uint64(VoxelGridSize) * sizeof(int32));
		VoxelIndirectBuffer = Device.CreateBuffer(EBufferUsage::Storage | EBufferUsage::Indirect | EBufferUsage::HostVisible, sizeof(DrawIndirectCommand));

		DebugVoxelsDescriptors Descriptors;
		Descriptors.WorldToVoxelBuffer = WorldToVoxelBuffer;
		Descriptors.VoxelBaseColor = VoxelBaseColor;
		Descriptors.VoxelNormal = VoxelNormal;
		Descriptors.VoxelRadiance = VoxelRadiance;
		Descriptors.VoxelPositions = VoxelPositions;
		Descriptors.VoxelIndirectBuffer = VoxelIndirectBuffer;
		VoxelSetLayout.UpdateDescriptorSet(Device, VoxelDescriptorSet, &Descriptors);
	}
	else
	{
		VoxelDescriptors Descriptors;
		Descriptors.WorldToVoxelBuffer = WorldToVoxelBuffer;
		Descriptors.VoxelBaseColor = VoxelBaseColor;
		Descriptors.VoxelNormal = VoxelNormal;
		Descriptors.VoxelRadiance = VoxelRadiance;
		VoxelSetLayout.UpdateDescriptorSet(Device, VoxelDescriptorSet, &Descriptors);
	}

	CreateVoxelRP();

	ComputePipelineDesc ComputeDesc = {};
	ComputeDesc.ComputeShader = ShaderMap.FindShader<DownsampleVolumeCS>();
	ComputeDesc.Layouts = { DownsampleVolumeSetLayout };

	DownsampleVolumePipeline = Device.CreatePipeline(ComputeDesc);

	const glm::vec4 Scale(2);
	DownsampleVolumeUniform = Device.CreateBuffer(EBufferUsage::Uniform | EBufferUsage::HostVisible, sizeof(Scale), &Scale);
}

void VCTLightingCache::Render(SceneProxy& Scene, drm::CommandList& CmdList)
{
	const glm::vec3 VoxelProbeCenter(
		Platform::GetFloat64("Engine.ini", "Voxels", "VoxelProbeCenterX", 0.0f),
		Platform::GetFloat64("Engine.ini", "Voxels", "VoxelProbeCenterY", 0.0f),
		Platform::GetFloat64("Engine.ini", "Voxels", "VoxelProbeCenterZ", 0.0f));
	
	glm::mat4 OrthoProj = glm::ortho(
		-(float)VoxelGridSize * 0.5f, (float)VoxelGridSize * 0.5f,
		-(float)VoxelGridSize * 0.5f, (float)VoxelGridSize * 0.5f,
		0.0f, (float)VoxelGridSize);
	OrthoProj[1][1] *= -1;

	WorldToVoxelUniform WorldToVoxelUniform;
	WorldToVoxelUniform.WorldToVoxel = glm::scale(glm::mat4(), glm::vec3(1.0f / VoxelSize)) * OrthoProj * glm::translate(glm::mat4(), -VoxelProbeCenter);
	WorldToVoxelUniform.WorldToVoxelInv = glm::inverse(WorldToVoxelUniform.WorldToVoxel);

	Platform::Memcpy(WorldToVoxelBuffer.GetData(), &WorldToVoxelUniform, sizeof(WorldToVoxelUniform));

	RenderVoxels(Scene, CmdList);

	ComputeLightInjection(Scene, CmdList);

	if (VoxelRadiance.GetMipLevels() > 1)
	{
		for (uint32 Level = 1; Level < VoxelRadianceMipMaps.size(); Level++)
		{
			const uint32 DownsampleFactor = 1 << (Level);

			ComputeVolumetricDownsample(
				CmdList,
				VoxelRadianceMipMaps[Level - 1], VoxelRadianceMipMaps[Level],
				glm::uvec3(VoxelRadiance.GetWidth(), VoxelRadiance.GetHeight(), VoxelRadiance.GetDepth()) / DownsampleFactor
			);

			ImageMemoryBarrier ImageBarrier{ VoxelRadiance };
			ImageBarrier.SrcAccessMask = EAccess::ShaderWrite;
			ImageBarrier.DstAccessMask = EAccess::ShaderRead;
			ImageBarrier.OldLayout = EImageLayout::General;
			ImageBarrier.NewLayout = EImageLayout::ShaderReadOnlyOptimal;
			ImageBarrier.BaseMipLevel = Level;
			ImageBarrier.LevelCount = 1;

			CmdList.PipelineBarrier(EPipelineStage::ComputeShader, EPipelineStage::ComputeShader, 0, nullptr, 1, &ImageBarrier);
		}
	}
}

void VCTLightingCache::RenderVoxels(SceneProxy& Scene, drm::CommandList& CmdList)
{
	ImageMemoryBarrier ImageBarrier{
		VoxelRadiance,
		EAccess::None,
		EAccess::TransferWrite,
		EImageLayout::Undefined,
		EImageLayout::TransferDstOptimal,
		0,
		VoxelRadiance.GetMipLevels()
	};

	CmdList.PipelineBarrier(EPipelineStage::TopOfPipe, EPipelineStage::Transfer, 0, nullptr, 1, &ImageBarrier);

	CmdList.ClearColorImage(VoxelRadiance, EImageLayout::TransferDstOptimal, ClearColorValue{});

	const std::vector<ImageMemoryBarrier> ImageBarriers =
	{
		{ VoxelBaseColor, EAccess::None, EAccess::ShaderWrite, EImageLayout::Undefined, EImageLayout::General },
		{ VoxelNormal, EAccess::None, EAccess::ShaderWrite, EImageLayout::Undefined, EImageLayout::General },
		{ VoxelRadiance, EAccess::TransferWrite, EAccess::ShaderWrite, EImageLayout::TransferDstOptimal, EImageLayout::General, 0, VoxelRadiance.GetMipLevels() },
	};

	CmdList.PipelineBarrier(EPipelineStage::Transfer, EPipelineStage::FragmentShader, 0, nullptr, ImageBarriers.size(), ImageBarriers.data());

	CmdList.BeginRenderPass(VoxelRP);
	
	MeshDrawCommand::Draw(CmdList, Scene.VoxelsPass);

	CmdList.EndRenderPass();
}

void VCTLightingCache::ComputeLightInjection(SceneProxy& SceneProxy, drm::CommandList& CmdList)
{
	auto& ECS = SceneProxy.ECS;
	auto& GlobalData = ECS.GetSingletonComponent<GlobalRenderData>();

	for (auto Entity : ECS.GetEntities<ShadowProxy>())
	{
		const auto& ShadowProxy = ECS.GetComponent<class ShadowProxy>(Entity);
		const drm::Image& ShadowMap = ShadowProxy.GetShadowMap();

		ComputePipelineDesc ComputeDesc = {};
		ComputeDesc.ComputeShader = ShaderMap.FindShader<LightInjectionCS>();
		ComputeDesc.Layouts = { GlobalData.CameraDescriptorSet.GetLayout(), VoxelDescriptorSet.GetLayout(), ShadowProxy.GetDescriptorSet().GetLayout() };

		std::shared_ptr<drm::Pipeline> Pipeline = Device.CreatePipeline(ComputeDesc);

		CmdList.BindPipeline(Pipeline);

		const std::vector<VkDescriptorSet> DescriptorSets = { GlobalData.CameraDescriptorSet, VoxelDescriptorSet, ShadowProxy.GetDescriptorSet() };
		CmdList.BindDescriptorSets(Pipeline, static_cast<uint32>(DescriptorSets.size()), DescriptorSets.data());
		
		const uint32 GroupCountX = DivideAndRoundUp(ShadowMap.GetWidth(), 8U);
		const uint32 GroupCountY = DivideAndRoundUp(ShadowMap.GetHeight(), 8U);
		CmdList.Dispatch(GroupCountX, GroupCountY, 1);
	}

	const ImageMemoryBarrier ImageBarrier{ VoxelRadiance, EAccess::ShaderWrite, EAccess::ShaderRead, EImageLayout::General, EImageLayout::ShaderReadOnlyOptimal };

	CmdList.PipelineBarrier(EPipelineStage::ComputeShader, EPipelineStage::ComputeShader, 0, nullptr, 1, &ImageBarrier);
}

void VCTLightingCache::ComputeVolumetricDownsample(drm::CommandList& CmdList, const drm::ImageView& SrcVolume, const drm::ImageView& DstVolume, const glm::uvec3& DstVolumeDimensions)
{
	drm::DescriptorSet DescriptorSet = DownsampleVolumeSetLayout.CreateDescriptorSet(Device);

	DownsampleVolumeDescriptors Descriptors;
	Descriptors.DownsampleVolumeUniform = DownsampleVolumeUniform;
	Descriptors.SrcVolume = drm::DescriptorImageInfo(SrcVolume, Device.CreateSampler({ EFilter::Nearest }));
	Descriptors.DstVolume = drm::DescriptorImageInfo(DstVolume);

	DownsampleVolumeSetLayout.UpdateDescriptorSet(Device, DescriptorSet, Descriptors);

	CmdList.BindPipeline(DownsampleVolumePipeline);

	std::vector<VkDescriptorSet> DescriptorSets = { DescriptorSet };

	CmdList.BindDescriptorSets(DownsampleVolumePipeline, static_cast<uint32>(DescriptorSets.size()), DescriptorSets.data());

	const glm::uvec3 GroupCounts(
		DivideAndRoundUp(DstVolumeDimensions.x / 2, 8u),
		DivideAndRoundUp(DstVolumeDimensions.y / 2, 8u),
		DivideAndRoundUp(DstVolumeDimensions.z / 2, 8u));

	CmdList.Dispatch(GroupCounts.x, GroupCounts.y, GroupCounts.z);
}

void VCTLightingCache::RenderVisualization(SceneProxy& Scene, drm::CommandList& CmdList)
{
	DrawIndirectCommand DrawIndirectCommand;
	DrawIndirectCommand.VertexCount = 0;
	DrawIndirectCommand.InstanceCount = 1;
	DrawIndirectCommand.FirstVertex = 0;
	DrawIndirectCommand.FirstInstance = 0;

	Platform::Memcpy(VoxelIndirectBuffer.GetData(), &DrawIndirectCommand, sizeof(DrawIndirectCommand));

	std::vector<BufferMemoryBarrier> BufferBarriers =
	{
		{ VoxelPositions, EAccess::ShaderWrite, EAccess::ShaderRead },
		{ VoxelIndirectBuffer, EAccess::ShaderWrite, EAccess::IndirectCommandRead }
	};

	std::vector<ImageMemoryBarrier> ImageBarriers =
	{
		{ VoxelBaseColor, EAccess::ShaderWrite, EAccess::ShaderRead, EImageLayout::General, EImageLayout::General },
		{ VoxelNormal, EAccess::ShaderWrite, EAccess::ShaderRead, EImageLayout::General, EImageLayout::General },
		{ VoxelRadiance, EAccess::ShaderWrite, EAccess::ShaderRead, EImageLayout::ShaderReadOnlyOptimal, EImageLayout::General, 0, VoxelRadiance.GetMipLevels() }
	};

	CmdList.PipelineBarrier(
		EPipelineStage::FragmentShader | EPipelineStage::ComputeShader,
		EPipelineStage::DrawIndirect | EPipelineStage::VertexShader,
		BufferBarriers.size(), BufferBarriers.data(),
		ImageBarriers.size(), ImageBarriers.data()
	);

	CmdList.BeginRenderPass(DebugRP);

	auto& GlobalData = Scene.ECS.GetSingletonComponent<GlobalRenderData>();

	PipelineStateDesc PSODesc = {};
	PSODesc.RenderPass = DebugRP;
	PSODesc.Viewport.Width = GlobalData.SceneColor.GetWidth();
	PSODesc.Viewport.Height = GlobalData.SceneColor.GetHeight();
	PSODesc.DepthStencilState.DepthTestEnable = true;
	PSODesc.DepthStencilState.DepthWriteEnable = true;
	PSODesc.ShaderStages.Vertex = ShaderMap.FindShader<DrawVoxelsVS>();
	PSODesc.ShaderStages.Geometry = ShaderMap.FindShader<DrawVoxelsGS>();
	PSODesc.ShaderStages.Fragment = ShaderMap.FindShader<DrawVoxelsFS>();
	PSODesc.InputAssemblyState.Topology = EPrimitiveTopology::PointList;
	PSODesc.Layouts = { GlobalData.CameraDescriptorSet.GetLayout(), VoxelDescriptorSet.GetLayout() };
	PSODesc.ColorBlendAttachmentStates.resize(1, {});
	PSODesc.ColorBlendAttachmentStates[0].BlendEnable = true;
	PSODesc.ColorBlendAttachmentStates[0].SrcColorBlendFactor = EBlendFactor::SRC_ALPHA;
	PSODesc.ColorBlendAttachmentStates[0].DstColorBlendFactor = EBlendFactor::ONE_MINUS_SRC_ALPHA;
	PSODesc.ColorBlendAttachmentStates[0].ColorBlendOp = EBlendOp::ADD;
	PSODesc.ColorBlendAttachmentStates[0].SrcAlphaBlendFactor = EBlendFactor::ONE_MINUS_SRC_ALPHA;
	PSODesc.ColorBlendAttachmentStates[0].DstAlphaBlendFactor = EBlendFactor::ZERO;
	PSODesc.ColorBlendAttachmentStates[0].AlphaBlendOp = EBlendOp::ADD;

	std::shared_ptr<drm::Pipeline> Pipeline = Device.CreatePipeline(PSODesc);

	CmdList.BindPipeline(Pipeline);

	const std::vector<VkDescriptorSet> DescriptorSets = { GlobalData.CameraDescriptorSet, VoxelDescriptorSet };
	CmdList.BindDescriptorSets(Pipeline, static_cast<uint32>(DescriptorSets.size()), DescriptorSets.data());

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