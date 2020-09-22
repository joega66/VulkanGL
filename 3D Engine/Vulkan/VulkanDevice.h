#pragma once
#include <GPU/GPU.h>
#include "VulkanQueues.h"
#include "VulkanCache.h"
#include "VulkanRenderPass.h"
#include "VulkanCommandList.h"
#include "VulkanBindlessDescriptors.h"
#include "vk_mem_alloc.h"

class VulkanDevice final : public gpu::Device
{
public:
	VulkanDevice(const DeviceDesc& deviceDesc);

	~VulkanDevice() override {}

	void EndFrame() override;

	void SubmitCommands(gpu::CommandList& cmdList) override;

	gpu::CommandList CreateCommandList(EQueue queue) override;

	gpu::Pipeline CreatePipeline(const PipelineStateDesc& psoDesc) override;

	gpu::Pipeline CreatePipeline(const ComputePipelineDesc& computePipelineDesc) override;

	gpu::DescriptorSetLayout CreateDescriptorSetLayout(std::size_t numBindings, const DescriptorBinding* bindings) override;

	gpu::Buffer CreateBuffer(EBufferUsage bufferUsage, EMemoryUsage memoryUsage, uint64 size, const void* data = nullptr) override;

	gpu::Image CreateImage(
		uint32 width,
		uint32 height,
		uint32 depth,
		EFormat format,
		EImageUsage imageUsage,
		uint32 mipLevels
	) override;

	gpu::ImageView CreateImageView(
		const gpu::Image& image, 
		uint32 baseMipLevel, 
		uint32 levelCount, 
		uint32 baseArrayLayer, 
		uint32 layerCount
	) override;

	gpu::Sampler CreateSampler(const SamplerDesc& samplerDesc) override;

	gpu::RenderPass CreateRenderPass(const RenderPassDesc& rpDesc) override;

	gpu::BindlessDescriptors& GetTextures() override;

	gpu::BindlessDescriptors& GetSamplers() override;

	gpu::BindlessDescriptors& GetImages() override;

	operator VkDevice() const { return _Device; }

	inline const VkPhysicalDevice& GetPhysicalDevice() const { return _PhysicalDevice; }
	inline const VkInstance& GetInstance() const { return _Instance; }
	inline VkSurfaceKHR GetSurface() const { return _Surface; }
	inline const VkPhysicalDeviceProperties& GetProperties() const { return _PhysicalDeviceProperties; }
	inline const VkPhysicalDeviceFeatures& GetFeatures() const { return _PhysicalDeviceFeatures; }
	inline VulkanCache& GetCache() { return _VulkanCache; }
	inline VulkanQueues& GetQueues() { return _Queues; }
	inline gpu::DescriptorPoolManager& GetDescriptorPoolManager() { return _DescriptorPoolManager; }
	
	static inline VkAccessFlags GetAccessFlags(EAccess access) 
	{
		// VkAccessFlags exactly match Access.
		return VkAccessFlags(access); 
	}

	static inline VkPipelineStageFlags GetPipelineStageFlags(EPipelineStage pipelineStage)
	{
		// VkPipelineStageFlags exactly match PipelineStage.
		return VkPipelineStageFlags(pipelineStage);
	}

	static const char* GetErrorString(VkResult result);

private:
	VkInstance _Instance;

	VkDebugReportCallbackEXT _DebugReportCallback;

	VkPhysicalDevice _PhysicalDevice;

	VkSurfaceKHR _Surface;

	VulkanQueues _Queues;

	VmaAllocator _Allocator;

	VkPhysicalDeviceProperties _PhysicalDeviceProperties;

	VkPhysicalDeviceFeatures _PhysicalDeviceFeatures;

	VulkanCache _VulkanCache;

	VkDevice _Device;

	gpu::DescriptorPoolManager _DescriptorPoolManager;

	std::shared_ptr<gpu::BindlessDescriptors> _BindlessTextures;

	std::shared_ptr<gpu::BindlessDescriptors> _BindlessSamplers;

	std::shared_ptr<gpu::BindlessDescriptors> _BindlessImages;
};

#define vulkan(result) \
	check(result == VK_SUCCESS, "%s", VulkanDevice::GetErrorString(result));