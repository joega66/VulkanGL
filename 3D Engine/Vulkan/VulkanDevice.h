#pragma once
#include <DRMCommandList.h>
#include "VulkanQueues.h"

static const std::vector<const char*> ValidationLayers =
{
	"VK_LAYER_LUNARG_standard_validation"
};

static const std::vector<const char*> DeviceExtensions =
{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_MAINTENANCE1_EXTENSION_NAME,
};

class VulkanDevice
{
	template<typename DRMObject, typename ...VulkanObjects>
	using SlowCache = std::vector<std::tuple<DRMObject, VulkanObjects...>>;
public:
	VkInstance Instance;

	VkDebugReportCallbackEXT DebugReportCallback;

	VkPhysicalDevice PhysicalDevice;

	VkDevice Device;

	VkPhysicalDeviceProperties Properties;

	VkPhysicalDeviceFeatures Features;

	VulkanDevice(Platform& Platform, bool bUseValidationLayers);

	~VulkanDevice();
	
	std::pair<VkRenderPass, VkFramebuffer> GetRenderPass(const RenderPassInitializer& RPInit);

	VkPipelineLayout GetPipelineLayout(const std::vector<VkDescriptorSetLayout>& DescriptorSetLayouts);

	VkPipeline GetPipeline(
		const PipelineStateInitializer& PSOInit, 
		VkPipelineLayout PipelineLayout, 
		VkRenderPass RenderPass, 
		uint32 NumRenderTargets);

	VkSampler GetSampler(const SamplerState& Sampler);

	VkDescriptorSetLayout GetDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& Bindings);

	operator VkDevice() const { return Device; }

	void FreeImage(class VulkanImage& Image);

	void DestroyPipelinesWithShader(const drm::ShaderRef& Shader);

	static const char* GetVulkanErrorString(VkResult Result);

private:

	struct VulkanRenderPassHash
	{
		struct MinAttachmentView
		{
			VkImage Image = VK_NULL_HANDLE;
			ELoadAction LoadAction = ELoadAction::DontCare;
			EStoreAction StoreAction = EStoreAction::DontCare;
			EImageLayout InitialLayout = EImageLayout::Undefined;
			EImageLayout FinalLayout = EImageLayout::Undefined;
			MinAttachmentView() = default;
			MinAttachmentView(const drm::AttachmentView& AView);
			bool operator==(const MinAttachmentView& Other);
		};

		std::vector<MinAttachmentView> ColorAttachments;
		MinAttachmentView DepthAttachment;

		VulkanRenderPassHash(const RenderPassInitializer& RPInit);
		bool operator==(const VulkanRenderPassHash& Other);
	};

	SlowCache<VulkanRenderPassHash, VkRenderPass, VkFramebuffer> RenderPassCache;

	SlowCache<std::vector<VkDescriptorSetLayout>, VkPipelineLayout> PipelineLayoutCache;

	struct VulkanPipelineHash
	{
		PipelineStateInitializer PSOInit;
		VkPipelineLayout PipelineLayout;
		VkRenderPass RenderPass;
		uint32 NumAttachments;

		VulkanPipelineHash(const PipelineStateInitializer& PSOInit, VkPipelineLayout PipelineLayout, VkRenderPass RenderPass, uint32 NumRenderTargets);
		bool operator==(const VulkanPipelineHash& Other) const;
		bool HasShader(const drm::ShaderRef& Shader) const;
	};

	SlowCache<VulkanPipelineHash, VkPipeline> PipelineCache;

	SlowCache<std::vector<VkDescriptorSetLayoutBinding>, VkDescriptorSetLayout> DescriptorSetLayoutCache;

	[[nodiscard]] std::pair<VkRenderPass, VkFramebuffer> CreateRenderPass(const RenderPassInitializer& RPInit);

	[[nodiscard]] VkPipeline CreatePipeline(
		const PipelineStateInitializer& PSOInit,
		VkPipelineLayout PipelineLayout,
		VkRenderPass RenderPass,
		uint32 NumRenderTargets);

	std::unordered_map<SamplerState, VkSampler, SamplerState::Hash> SamplerCache;
};

CLASS(VulkanDevice);

#define vulkan(Result) \
	check(Result == VK_SUCCESS, "%s", VulkanDevice::GetVulkanErrorString(Result));