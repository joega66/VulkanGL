#pragma once
#include <RenderCommandList.h>
#include "VulkanShader.h"
#include <sstream>
#include <iostream>

class VulkanDevice
{
	template<typename DRMObject, typename ...VulkanObjects>
	using SlowCache = std::vector<std::tuple<DRMObject, VulkanObjects...>>;
public:
	VkPhysicalDevice PhysicalDevice;
	VkSurfaceKHR Surface;
	VkQueue GraphicsQueue;
	VkQueue PresentQueue;
	VkCommandPool CommandPool;
	VkPhysicalDeviceProperties Properties;
	VkPhysicalDeviceFeatures Features;
	HashTable<std::type_index, VulkanShader> ShaderCache;

	VulkanDevice();
	~VulkanDevice();
	
	std::tuple<VkPipeline, VkPipelineLayout, VkDescriptorSetLayout> GetPipeline(const PipelineStateInitializer& PSOInit, VkRenderPass RenderPass, uint32 NumRenderTargets);

	operator VkDevice() { return Device; }

private:
	VkInstance Instance;
	VkDevice Device;
	VkDebugReportCallbackEXT DebugReportCallback;

	SlowCache<GraphicsPipelineState, VkPipelineLayout, VkDescriptorSetLayout> PipelineLayoutCache;
	SlowCache<PipelineStateInitializer, VkPipeline> PipelineCache;

	std::pair<VkPipelineLayout, VkDescriptorSetLayout> GetPipelineLayout(const GraphicsPipelineState& GraphicsPipelineState);

	[[nodiscard]] VkPipeline CreatePipeline(
		const PipelineStateInitializer& PSOInit,
		VkPipelineLayout PipelineLayout,
		VkRenderPass RenderPass,
		uint32 NumRenderTargets);
};

CLASS(VulkanDevice);

struct QueueFamilyIndices
{
	int GraphicsFamily = -1;
	int PresentFamily = -1;

	bool IsComplete() const;
	void FindQueueFamilies(VkPhysicalDevice Device, VkSurfaceKHR Surface);
};

#define vulkan(Result) \
		if (Result != VK_SUCCESS)	\
		{							\
			std::stringstream SS;	\
			SS << "Vulkan call ";	\
			SS << #Result;			\
			SS << " in file ";		\
			SS << __FILE__;			\
			SS << " on line ";		\
			SS << __LINE__;			\
			SS << " failed.";		\
			fail(SS.str());			\
		}							