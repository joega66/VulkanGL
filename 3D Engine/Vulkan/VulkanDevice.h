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
	VulkanDevice(Platform& Platform);

	virtual ~VulkanDevice() override {}

	virtual void EndFrame() override;

	virtual void SubmitCommands(gpu::CommandList& CmdList) override;

	virtual gpu::CommandList CreateCommandList(EQueue Queue) override;

	virtual gpu::Pipeline CreatePipeline(const PipelineStateDesc& PSODesc) override;

	virtual gpu::Pipeline CreatePipeline(const ComputePipelineDesc& ComputePipelineDesc) override;

	virtual gpu::DescriptorSetLayout CreateDescriptorSetLayout(std::size_t NumEntries, const DescriptorBinding* Entries) override;

	virtual gpu::Buffer CreateBuffer(EBufferUsage Usage, uint64 Size, const void* Data = nullptr) override;

	virtual gpu::Image CreateImage(
		uint32 Width,
		uint32 Height,
		uint32 Depth,
		EFormat Format,
		EImageUsage UsageFlags,
		uint32 MipLevels
	) override;

	virtual gpu::ImageView CreateImageView(
		const gpu::Image& Image,
		uint32 BaseMipLevel,
		uint32 LevelCount,
		uint32 BaseArrayLayer,
		uint32 LayerCount
	) override;

	virtual gpu::Sampler CreateSampler(const SamplerDesc& SamplerDesc) override;

	virtual gpu::RenderPass CreateRenderPass(const RenderPassDesc& RPDesc) override;

	virtual gpu::TextureID CreateTextureID(const VulkanImageView& ImageView) override;
	
	virtual gpu::ImageID CreateImageID(const VulkanImageView& ImageView) override;

	virtual gpu::BindlessResources& GetTextures() override;

	virtual gpu::BindlessResources& GetSamplers() override;

	virtual gpu::BindlessResources& GetImages() override;

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