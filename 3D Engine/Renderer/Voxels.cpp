#include "Voxels.h"
#include "SceneRenderer.h"
#include "MaterialShader.h"
#include "MeshProxy.h"
#include "FullscreenQuad.h"
#include "ShadowProxy.h"
#include <Engine/Engine.h>
#include <Components/RenderSettings.h>

struct VoxelDescriptors
{
	drm::DescriptorBufferInfo VoxelUniformBuffer;
	drm::DescriptorImageInfo VoxelBaseColor;
	drm::DescriptorImageInfo VoxelNormal;

	static const std::vector<DescriptorBinding>& GetBindings()
	{
		static const std::vector<DescriptorBinding> Bindings =
		{
			{ 0, 1, EDescriptorType::UniformBuffer },
			{ 1, 1, EDescriptorType::StorageImage },
			{ 2, 1, EDescriptorType::StorageImage },
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
		Bindings.push_back({ 3, 1, EDescriptorType::StorageBuffer });
		Bindings.push_back({ 4, 1, EDescriptorType::StorageBuffer });
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

void CameraProxy::AddToVoxelsPass(Engine& Engine, const MeshProxy& MeshProxy)
{
	auto& VCTLighting = Engine.ECS.GetSingletonComponent<VCTLightingCache>();

	static constexpr EMeshType MeshType = EMeshType::StaticMesh;

	PipelineStateDesc PSODesc = {};
	PSODesc.RenderPass = VCTLighting.GetRenderPass();
	PSODesc.ShaderStages.Vertex = Engine.ShaderMap.FindShader<VoxelsVS<MeshType>>();
	PSODesc.ShaderStages.Geometry = Engine.ShaderMap.FindShader<VoxelsGS<MeshType>>();
	PSODesc.ShaderStages.Fragment = Engine.ShaderMap.FindShader<VoxelsFS<MeshType>>();
	PSODesc.DepthStencilState.DepthTestEnable = false;
	PSODesc.DepthStencilState.DepthWriteEnable = false;
	PSODesc.Viewport.Width = VCTLighting.GetVoxelGridSize();
	PSODesc.Viewport.Height = VCTLighting.GetVoxelGridSize();
	PSODesc.Layouts = {
		CameraDescriptorSet.GetLayout(),
		MeshProxy.GetSurfaceSet().GetLayout(),
		Engine.Device.GetTextures().GetLayout(),
		Engine.Device.GetSamplers().GetLayout(),
		VCTLighting.GetDescriptorSet().GetLayout()
	};

	const std::vector<VkDescriptorSet> DescriptorSets =
	{
		CameraDescriptorSet,
		MeshProxy.GetSurfaceSet(),
		Engine.Device.GetTextures().GetSet(),
		Engine.Device.GetSamplers().GetSet(),
		VCTLighting.GetDescriptorSet()
	};

	VoxelsPass.push_back(MeshDrawCommand(Engine.Device, MeshProxy, PSODesc, DescriptorSets));
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

class LightVolumeCS : public drm::Shader
{
public:
	LightVolumeCS(const ShaderCompilationInfo& CompilationInfo)
		: drm::Shader(CompilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		VoxelShader::SetEnvironmentVariables(Worker);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo BaseInfo = { "../Shaders/LightVolumeCS.glsl", "main", EShaderStage::Compute };
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

UNIFORM_STRUCT(VoxelUniformBufferData,
	glm::mat4 WorldToVoxel;
	glm::mat4 WorldToVoxelInv;
	glm::vec4 VoxelSize;
);

VCTLightingCache::VCTLightingCache(Engine& Engine)
	: VoxelGridSize(Platform::GetInt("Engine.ini", "Voxels", "VoxelGridSize", 256))
	, DebugVoxels(Platform::GetBool("Engine.ini", "Voxels", "DebugVoxels", false))
	, Device(Engine.Device)
	, ShaderMap(Engine.ShaderMap)
	, DownsampleVolumeSetLayout(Engine.Device)
{
	check(VoxelGridSize <= 1024, "Exceeded voxel bits.");
	
	VoxelUniformBuffer = Device.CreateBuffer(EBufferUsage::Uniform | EBufferUsage::HostVisible, sizeof(VoxelUniformBufferData));
	VoxelIndirectBuffer = Device.CreateBuffer(EBufferUsage::Storage | EBufferUsage::Indirect | EBufferUsage::HostVisible, sizeof(DrawIndirectCommand));

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
		
		DebugVoxelsDescriptors Descriptors;
		Descriptors.VoxelUniformBuffer = VoxelUniformBuffer;
		Descriptors.VoxelBaseColor = VoxelBaseColor;
		Descriptors.VoxelNormal = VoxelNormal;
		Descriptors.VoxelPositions = VoxelPositions;
		Descriptors.VoxelIndirectBuffer = VoxelIndirectBuffer;
		VoxelSetLayout.UpdateDescriptorSet(Device, VoxelDescriptorSet, &Descriptors);
	}
	else
	{
		VoxelDescriptors Descriptors;
		Descriptors.VoxelUniformBuffer = VoxelUniformBuffer;
		Descriptors.VoxelBaseColor = VoxelBaseColor;
		Descriptors.VoxelNormal = VoxelNormal;
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

void VCTLightingCache::Render(EntityManager& ECS, CameraProxy& Camera, drm::CommandList& CmdList)
{
	const auto& Settings = ECS.GetSingletonComponent<RenderSettings>();
	const float InvVoxelSize = 1.0f / (Settings.VoxelSize * static_cast<float>(VoxelGridSize));
	const float VoxelHalfSize = VoxelGridSize * 0.5f;

	glm::mat4 OrthoProj = glm::ortho(
		-VoxelHalfSize, VoxelHalfSize,
		-VoxelHalfSize, VoxelHalfSize,
		0.0f, static_cast<float>(VoxelGridSize));
	OrthoProj[1][1] *= -1;
	
	WorldToVoxel = glm::scale(glm::mat4(), glm::vec3(1.0f / Settings.VoxelSize)) * OrthoProj * glm::translate(glm::mat4(), -Settings.VoxelFieldCenter);

	VoxelUniformBufferData* VoxelUniformBufferDataPtr = static_cast<VoxelUniformBufferData*>(VoxelUniformBuffer.GetData());
	VoxelUniformBufferDataPtr->WorldToVoxel = WorldToVoxel;
	VoxelUniformBufferDataPtr->WorldToVoxelInv = glm::inverse(WorldToVoxel);
	VoxelUniformBufferDataPtr->VoxelSize = glm::vec4(InvVoxelSize, InvVoxelSize, Settings.VoxelSize, Settings.VoxelSize);

	DrawIndirectCommand* DrawIndirectCommandPtr = static_cast<DrawIndirectCommand*>(VoxelIndirectBuffer.GetData());
	DrawIndirectCommandPtr->VertexCount = 0;
	DrawIndirectCommandPtr->InstanceCount = 1;
	DrawIndirectCommandPtr->FirstVertex = 0;
	DrawIndirectCommandPtr->FirstInstance = 0;

	RenderVoxels(Camera, CmdList);

	ComputeLightVolume(ECS, Camera, CmdList);

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

void VCTLightingCache::RenderVoxels(CameraProxy& Camera, drm::CommandList& CmdList)
{
	const std::vector<ImageMemoryBarrier> ImageBarriers =
	{
		{ VoxelBaseColor, EAccess::None, EAccess::ShaderWrite, EImageLayout::Undefined, EImageLayout::General },
		{ VoxelNormal, EAccess::None, EAccess::ShaderWrite, EImageLayout::Undefined, EImageLayout::General },
	};

	CmdList.PipelineBarrier(EPipelineStage::Transfer, EPipelineStage::FragmentShader, 0, nullptr, ImageBarriers.size(), ImageBarriers.data());

	CmdList.BeginRenderPass(VoxelRP);
	
	MeshDrawCommand::Draw(CmdList, Camera.VoxelsPass);

	CmdList.EndRenderPass();

	const std::vector<ImageMemoryBarrier> ReadBarriers =
	{
		{ VoxelBaseColor, EAccess::ShaderWrite, EAccess::ShaderRead, EImageLayout::General, EImageLayout::ShaderReadOnlyOptimal },
		{ VoxelNormal, EAccess::ShaderWrite, EAccess::ShaderRead, EImageLayout::General, EImageLayout::ShaderReadOnlyOptimal },
	};

	CmdList.PipelineBarrier(EPipelineStage::FragmentShader, EPipelineStage::ComputeShader, 0, nullptr, ReadBarriers.size(), ReadBarriers.data());
}

void VCTLightingCache::ComputeLightVolume(EntityManager& ECS, CameraProxy& Camera, drm::CommandList& CmdList)
{
	ImageMemoryBarrier ImageBarrier
	{
		VoxelRadiance,
		EAccess::None,
		EAccess::TransferWrite,
		EImageLayout::Undefined,
		EImageLayout::TransferDstOptimal,
		0, VoxelRadiance.GetMipLevels()
	};

	CmdList.PipelineBarrier(EPipelineStage::TopOfPipe, EPipelineStage::Transfer, 0, nullptr, 1, &ImageBarrier);

	CmdList.ClearColorImage(VoxelRadiance, EImageLayout::TransferDstOptimal, ClearColorValue{});

	ImageBarrier.SrcAccessMask = EAccess::TransferWrite;
	ImageBarrier.DstAccessMask = EAccess::ShaderWrite;
	ImageBarrier.OldLayout = EImageLayout::TransferDstOptimal;
	ImageBarrier.NewLayout = EImageLayout::General;

	CmdList.PipelineBarrier(EPipelineStage::Transfer, EPipelineStage::ComputeShader, 0, nullptr, 1, &ImageBarrier);

	struct LightVolumePushConstants
	{
		drm::TextureID ShadowMap;
		drm::TextureID VoxelBaseColor;
		drm::TextureID VoxelNormal;
		drm::ImageID VoxelRadiance;
		glm::mat4 WorldToVoxel;
	};

	LightVolumePushConstants LightVolumePushConstants;
	LightVolumePushConstants.VoxelBaseColor = VoxelBaseColor.GetTextureID();
	LightVolumePushConstants.VoxelNormal = VoxelNormal.GetTextureID();
	LightVolumePushConstants.VoxelRadiance = VoxelRadiance.GetImageID();
	LightVolumePushConstants.WorldToVoxel = WorldToVoxel;

	for (auto Entity : ECS.GetEntities<ShadowProxy>())
	{
		const auto& ShadowProxy = ECS.GetComponent<class ShadowProxy>(Entity);
		const drm::Image& ShadowMap = ShadowProxy.GetShadowMap();

		ComputePipelineDesc ComputeDesc = {};
		ComputeDesc.ComputeShader = ShaderMap.FindShader<LightVolumeCS>();
		ComputeDesc.Layouts =
		{
			Camera.CameraDescriptorSet.GetLayout(),
			ShadowProxy.GetDescriptorSet().GetLayout(),
			Device.GetTextures().GetLayout(),
			Device.GetTextures().GetLayout(),
			Device.GetImages().GetLayout(),
		};
		ComputeDesc.PushConstantRanges.push_back(PushConstantRange{ EShaderStage::Compute, 0, sizeof(LightVolumePushConstants) });

		drm::Pipeline Pipeline = Device.CreatePipeline(ComputeDesc);

		CmdList.BindPipeline(Pipeline);

		const std::vector<VkDescriptorSet> DescriptorSets =
		{
			Camera.CameraDescriptorSet,
			ShadowProxy.GetDescriptorSet(),
			Device.GetTextures().GetSet(),
			Device.GetTextures().GetSet(),
			Device.GetImages().GetSet(),
		};

		CmdList.BindDescriptorSets(Pipeline, static_cast<uint32>(DescriptorSets.size()), DescriptorSets.data());

		LightVolumePushConstants.ShadowMap = ShadowMap.GetTextureID();

		CmdList.PushConstants(Pipeline, EShaderStage::Compute, 0, sizeof(LightVolumePushConstants), &LightVolumePushConstants);

		const uint32 GroupCountX = DivideAndRoundUp(ShadowMap.GetWidth(), 8U);
		const uint32 GroupCountY = DivideAndRoundUp(ShadowMap.GetHeight(), 8U);
		CmdList.Dispatch(GroupCountX, GroupCountY, 1);
	}

	const ImageMemoryBarrier ReadBarrier{ VoxelRadiance, EAccess::ShaderWrite, EAccess::ShaderRead, EImageLayout::General, EImageLayout::ShaderReadOnlyOptimal };

	CmdList.PipelineBarrier(EPipelineStage::ComputeShader, EPipelineStage::ComputeShader, 0, nullptr, 1, &ReadBarrier);
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

void VCTLightingCache::RenderVisualization(CameraProxy& Camera, drm::CommandList& CmdList, EVoxelDebugMode VoxelDebugMode)
{
	std::vector<BufferMemoryBarrier> BufferBarriers =
	{
		{ VoxelPositions, EAccess::ShaderWrite, EAccess::ShaderRead },
		{ VoxelIndirectBuffer, EAccess::ShaderWrite, EAccess::IndirectCommandRead }
	};

	CmdList.PipelineBarrier(
		EPipelineStage::FragmentShader,
		EPipelineStage::DrawIndirect | EPipelineStage::VertexShader,
		BufferBarriers.size(), BufferBarriers.data(),
		0, nullptr
	);

	struct DrawVoxelsPushConstants
	{
		drm::TextureID VoxelRadiance;
		drm::TextureID VoxelBaseColor;
		drm::TextureID VoxelNormal;
	} PushConstants;

	PushConstants.VoxelRadiance = VoxelRadiance.GetTextureID();
	PushConstants.VoxelBaseColor = VoxelBaseColor.GetTextureID();
	PushConstants.VoxelNormal = VoxelNormal.GetTextureID();

	CmdList.BeginRenderPass(DebugRP);

	PipelineStateDesc PSODesc = {};
	PSODesc.RenderPass = DebugRP;
	PSODesc.Viewport.Width = Camera.SceneColor.GetWidth();
	PSODesc.Viewport.Height = Camera.SceneColor.GetHeight();
	PSODesc.DepthStencilState.DepthTestEnable = true;
	PSODesc.DepthStencilState.DepthWriteEnable = true;
	PSODesc.ShaderStages.Vertex = ShaderMap.FindShader<DrawVoxelsVS>();
	PSODesc.ShaderStages.Geometry = ShaderMap.FindShader<DrawVoxelsGS>();
	PSODesc.ShaderStages.Fragment = ShaderMap.FindShader<DrawVoxelsFS>();
	PSODesc.InputAssemblyState.Topology = EPrimitiveTopology::PointList;
	PSODesc.Layouts = { Camera.CameraDescriptorSet.GetLayout(), VoxelDescriptorSet.GetLayout(), Device.GetTextures().GetLayout() };
	PSODesc.ColorBlendAttachmentStates.resize(1, {});
	PSODesc.ColorBlendAttachmentStates[0].BlendEnable = true;
	PSODesc.ColorBlendAttachmentStates[0].SrcColorBlendFactor = EBlendFactor::SRC_ALPHA;
	PSODesc.ColorBlendAttachmentStates[0].DstColorBlendFactor = EBlendFactor::ONE_MINUS_SRC_ALPHA;
	PSODesc.ColorBlendAttachmentStates[0].ColorBlendOp = EBlendOp::ADD;
	PSODesc.ColorBlendAttachmentStates[0].SrcAlphaBlendFactor = EBlendFactor::ONE_MINUS_SRC_ALPHA;
	PSODesc.ColorBlendAttachmentStates[0].DstAlphaBlendFactor = EBlendFactor::ZERO;
	PSODesc.ColorBlendAttachmentStates[0].AlphaBlendOp = EBlendOp::ADD;
	PSODesc.SpecializationInfo.Add(0, static_cast<uint32>(VoxelDebugMode));
	PSODesc.PushConstantRanges.push_back({ EShaderStage::Vertex, 0, sizeof(PushConstants) });

	drm::Pipeline Pipeline = Device.CreatePipeline(PSODesc);

	CmdList.BindPipeline(Pipeline);

	const std::vector<VkDescriptorSet> DescriptorSets = { Camera.CameraDescriptorSet, VoxelDescriptorSet, Device.GetTextures().GetSet() };
	CmdList.BindDescriptorSets(Pipeline, static_cast<uint32>(DescriptorSets.size()), DescriptorSets.data());

	CmdList.PushConstants(Pipeline, EShaderStage::Vertex, 0, sizeof(PushConstants), &PushConstants);

	CmdList.DrawIndirect(VoxelIndirectBuffer, 0, 1);

	CmdList.EndRenderPass();
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
		EImageLayout::General));
	RPDesc.DepthAttachment = drm::AttachmentView(
		&SceneDepth,
		ELoadAction::Clear,
		EStoreAction::Store,
		ClearDepthStencilValue{},
		EImageLayout::Undefined,
		EImageLayout::DepthReadStencilWrite);
	RPDesc.RenderArea = RenderArea{ glm::ivec2(), glm::uvec2(SceneDepth.GetWidth(), SceneDepth.GetHeight()) };
	DebugRP = Device.CreateRenderPass(RPDesc);
}

void VCTLightingCache::CreateVoxelRP()
{
	RenderPassDesc RPDesc = {}; // Disable ROP
	RPDesc.RenderArea.Extent = glm::uvec2(GetVoxelGridSize());
	VoxelRP = Device.CreateRenderPass(RPDesc);
}