#pragma once
#include <DRM.h>
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

class VulkanDevice final : public DRMDevice
{
public:
	VulkanDevice(Platform& Platform);

	virtual ~VulkanDevice() override {}

	virtual void EndFrame() override;

	virtual void SubmitCommands(drm::CommandList& CmdList) override;

	virtual drm::CommandList CreateCommandList(EQueue Queue) override;

	virtual drm::Pipeline CreatePipeline(const PipelineStateDesc& PSODesc) override;

	virtual drm::Pipeline CreatePipeline(const ComputePipelineDesc& ComputePipelineDesc) override;

	virtual drm::DescriptorSetLayout CreateDescriptorSetLayout(std::size_t NumEntries, const DescriptorBinding* Entries) override;

	virtual drm::Buffer CreateBuffer(EBufferUsage Usage, uint64 Size, const void* Data = nullptr) override;

	virtual drm::Image CreateImage(
		uint32 Width,
		uint32 Height,
		uint32 Depth,
		EFormat Format,
		EImageUsage UsageFlags,
		uint32 MipLevels
	) override;

	virtual drm::ImageView CreateImageView(
		const drm::Image& Image,
		uint32 BaseMipLevel,
		uint32 LevelCount,
		uint32 BaseArrayLayer,
		uint32 LayerCount
	) override;

	virtual drm::Sampler CreateSampler(const SamplerDesc& SamplerDesc) override;

	virtual drm::RenderPass CreateRenderPass(const RenderPassDesc& RPDesc) override;

	virtual drm::TextureID CreateTextureID(const VulkanImageView& ImageView) override;
	
	virtual drm::ImageID CreateImageID(const VulkanImageView& ImageView) override;

	virtual drm::BindlessResources& GetTextures() override;

	virtual drm::BindlessResources& GetSamplers() override;

	virtual drm::BindlessResources& GetImages() override;

	operator VkDevice() const { return Device; }

	inline const VkPhysicalDevice& GetPhysicalDevice() const { return PhysicalDevice; }
	inline const VkInstance& GetInstance() const { return Instance; }
	inline const VkPhysicalDeviceProperties& GetProperties() const { return Properties; }
	inline const VkPhysicalDeviceFeatures& GetFeatures() const { return Features; }
	inline VulkanCache& GetCache() { return VulkanCache; }
	inline VulkanQueues& GetQueues() { return Queues; }
	inline VulkanDescriptorPoolManager& GetDescriptorPoolManager() { return DescriptorPoolManager; }
	
	void CreateLogicalDevice();

private:
	VkInstance Instance;

	VkDebugReportCallbackEXT DebugReportCallback;

	VkPhysicalDevice PhysicalDevice;

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