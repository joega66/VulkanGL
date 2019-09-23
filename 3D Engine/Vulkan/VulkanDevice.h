#pragma once
#include <RenderCommandList.h>
#include "VulkanShader.h"
#include "VulkanQueues.h"

class VulkanDevice
{
	template<typename DRMObject, typename ...VulkanObjects>
	using SlowCache = std::vector<std::tuple<DRMObject, VulkanObjects...>>;
public:
	VkPhysicalDevice PhysicalDevice;

	VkSurfaceKHR Surface;

	VulkanQueues Queues;

	VkPhysicalDeviceProperties Properties;

	VkPhysicalDeviceFeatures Features;

	HashTable<std::type_index, VulkanShader> ShaderCache;

	VulkanDevice();

	~VulkanDevice();
	
	std::pair<VkRenderPass, VkFramebuffer> GetRenderPass(const RenderPassInitializer& RPInit);

	std::tuple<VkPipeline, VkPipelineLayout, VkDescriptorSetLayout> GetPipeline(const PipelineStateInitializer& PSOInit, VkRenderPass RenderPass, uint32 NumRenderTargets);

	VkSampler GetSampler(const SamplerState& Sampler);

	operator VkDevice() { return Device; }

	void FreeImage(class VulkanImage& Image);

private:
	VkInstance Instance;

	VkDevice Device;

	VkDebugReportCallbackEXT DebugReportCallback;

	struct VulkanRenderPassHash
	{
		struct MinRenderTargetView
		{
			VkImage Image;
			ELoadAction LoadAction;
			EStoreAction StoreAction;
			EImageLayout InitialLayout;
			EImageLayout FinalLayout;
			MinRenderTargetView(const class VulkanRenderTargetView& RTView);
			bool operator==(const MinRenderTargetView& Other);
		};

		std::vector<MinRenderTargetView> ColorTargets;

		MinRenderTargetView DepthTarget;

		VulkanRenderPassHash(const RenderPassInitializer& RPInit);

		bool operator==(const VulkanRenderPassHash& Other);
	};

	SlowCache<VulkanRenderPassHash, VkRenderPass, VkFramebuffer> RenderPassCache;

	SlowCache<PipelineStateInitializer, VkPipeline> PipelineCache;

	SlowCache<GraphicsPipelineState, VkPipelineLayout, VkDescriptorSetLayout> PipelineLayoutCache;

	std::pair<VkPipelineLayout, VkDescriptorSetLayout> GetPipelineLayout(const GraphicsPipelineState& GraphicsPipelineState);

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
	check(Result == VK_SUCCESS, "Vulkan call failed.");				