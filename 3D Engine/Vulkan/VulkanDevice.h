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

	HashTable<std::type_index, std::unique_ptr<VulkanShader>> ShaderCache;

	VulkanDevice();

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

private:
	VkInstance Instance;

	VkDevice Device;

	VkDebugReportCallbackEXT DebugReportCallback;

	struct VulkanRenderPassHash
	{
		struct MinRenderTargetView
		{
			VkImage Image = VK_NULL_HANDLE;
			ELoadAction LoadAction = ELoadAction::DontCare;
			EStoreAction StoreAction = EStoreAction::DontCare;
			EImageLayout InitialLayout = EImageLayout::Undefined;
			EImageLayout FinalLayout = EImageLayout::Undefined;
			MinRenderTargetView() = default;
			MinRenderTargetView(const class VulkanRenderTargetView& RTView);
			bool operator==(const MinRenderTargetView& Other);
		};

		std::vector<MinRenderTargetView> ColorTargets;
		MinRenderTargetView DepthTarget;

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
		uint32 NumRenderTargets;

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
	check(Result == VK_SUCCESS, "Vulkan call failed.");				