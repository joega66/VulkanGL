#pragma once
#include <DRM.h>
#include "VulkanQueues.h"
#include "VulkanCache.h"
#include "VulkanRenderPass.h"
#include "VulkanCommandList.h"

static const std::vector<const char*> ValidationLayers =
{
	"VK_LAYER_LUNARG_standard_validation"
};

static const std::vector<const char*> DeviceExtensions =
{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_MAINTENANCE1_EXTENSION_NAME,
	VK_KHR_DESCRIPTOR_UPDATE_TEMPLATE_EXTENSION_NAME,
};

class VulkanDevice final : public DRMDevice
{
public:
	VulkanDevice(Platform& Platform);

	virtual ~VulkanDevice() override {}

	virtual void EndFrame() override;

	virtual void SubmitCommands(drm::CommandListRef CmdList) override;

	virtual drm::CommandListRef CreateCommandList() override;

	virtual drm::DescriptorTemplateRef CreateDescriptorTemplate(uint32 NumEntries, const DescriptorTemplateEntry* Entries) override;

	virtual drm::BufferRef CreateBuffer(EBufferUsage Usage, uint32 Size, const void* Data = nullptr) override;

	virtual drm::ImageRef CreateImage(
		uint32 Width, 
		uint32 Height, 
		uint32 Depth, 
		EFormat Format, 
		EImageUsage UsageFlags,
		EImageLayout InitialLayout = EImageLayout::Undefined
	) override;

	virtual void* LockBuffer(drm::BufferRef Buffer) override;

	virtual void UnlockBuffer(drm::BufferRef Buffer) override;

	virtual drm::RenderPass CreateRenderPass(const RenderPassDesc& RPDesc) override;

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

	/** The logical device. */
	VkDevice Device;

	VulkanDescriptorPoolManager DescriptorPoolManager;
};

namespace
{
	CAST(drm::Buffer, VulkanBuffer);
	CAST(drm::Image, VulkanImage);
	CAST(drm::CommandList, VulkanCommandList);
	CAST(drm::DescriptorSet, VulkanDescriptorSet);
};

#define vulkan(Result) \
	check(Result == VK_SUCCESS, "%s", VulkanCache::GetVulkanErrorString(Result));