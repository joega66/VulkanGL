#pragma once
#include <GPU/GPU.h>
#include "VulkanQueues.h"
#include "VulkanCache.h"
#include "VulkanRenderPass.h"
#include "VulkanCommandList.h"
#include "VulkanBindlessResources.h"

static const std::vector<const char*> ValidationLayers =
{
	"VK_LAYER_LUNARG_standard_validation"
};

static const std::vector<const char*> DeviceExtensions =
{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_DESCRIPTOR_UPDATE_TEMPLATE_EXTENSION_NAME,
};

class VulkanDevice final : public gpu::Device
{
public:
	VulkanDevice(const DeviceDesc& deviceDesc);

	~VulkanDevice() override {}

	void EndFrame() override;

	void SubmitCommands(gpu::CommandList& CmdList) override;

	gpu::CommandList CreateCommandList(EQueue Queue) override;

	gpu::Pipeline CreatePipeline(const PipelineStateDesc& PSODesc) override;

	gpu::Pipeline CreatePipeline(const ComputePipelineDesc& ComputePipelineDesc) override;

	gpu::DescriptorSetLayout CreateDescriptorSetLayout(std::size_t NumEntries, const DescriptorBinding* Entries) override;

	gpu::Buffer CreateBuffer(EBufferUsage Usage, uint64 Size, const void* Data = nullptr) override;

	gpu::Image CreateImage(
		uint32 Width,
		uint32 Height,
		uint32 Depth,
		EFormat Format,
		EImageUsage UsageFlags,
		uint32 MipLevels
	) override;

	gpu::ImageView CreateImageView(
		const gpu::Image& Image,
		uint32 BaseMipLevel,
		uint32 LevelCount,
		uint32 BaseArrayLayer,
		uint32 LayerCount
	) override;

	gpu::Sampler CreateSampler(const SamplerDesc& SamplerDesc) override;

	gpu::RenderPass CreateRenderPass(const RenderPassDesc& RPDesc) override;

	gpu::TextureID CreateTextureID(const VulkanImageView& ImageView) override;
	
	gpu::ImageID CreateImageID(const VulkanImageView& ImageView) override;

	gpu::BindlessResources& GetTextures() override;

	gpu::BindlessResources& GetSamplers() override;

	gpu::BindlessResources& GetImages() override;

	operator VkDevice() const { return Device; }

	inline const VkPhysicalDevice& GetPhysicalDevice() const { return PhysicalDevice; }
	inline const VkInstance& GetInstance() const { return Instance; }
	inline VkSurfaceKHR GetSurface() const { return _Surface; }
	inline const VkPhysicalDeviceProperties& GetProperties() const { return Properties; }
	inline const VkPhysicalDeviceFeatures& GetFeatures() const { return Features; }
	inline VulkanCache& GetCache() { return VulkanCache; }
	inline VulkanQueues& GetQueues() { return Queues; }
	inline VulkanDescriptorPoolManager& GetDescriptorPoolManager() { return DescriptorPoolManager; }
	

	static inline VkAccessFlags GetAccessFlags(EAccess access) 
	{ 
		return VkAccessFlags(access); 
	}

	static inline VkPipelineStageFlags GetPipelineStageFlags(EPipelineStage pipelineStage)
	{
		return VkPipelineStageFlags(pipelineStage);
	}

private:
	VkInstance Instance;

	VkDebugReportCallbackEXT DebugReportCallback;

	VkPhysicalDevice PhysicalDevice;

	VkSurfaceKHR _Surface;

	VulkanQueues Queues;

	VulkanAllocator Allocator;

	VkPhysicalDeviceProperties Properties;

	VkPhysicalDeviceFeatures Features;

	VulkanCache VulkanCache;

	VkDevice Device;

	VulkanDescriptorPoolManager DescriptorPoolManager;

	std::shared_ptr<VulkanBindlessResources> BindlessTextures;

	std::shared_ptr<VulkanBindlessResources> BindlessSamplers;

	std::shared_ptr<VulkanBindlessResources> BindlessImages;
};

#define vulkan(Result) \
	check(Result == VK_SUCCESS, "%s", VulkanCache::GetVulkanErrorString(Result));