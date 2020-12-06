#pragma once
#include <GPU/GPU.h>
#include "VulkanQueue.h"
#include "VulkanRenderPass.h"
#include "VulkanCommandBuffer.h"
#include "VulkanBindlessDescriptors.h"
#include "vk_mem_alloc.h"

class VulkanInstance;
class VulkanPhysicalDevice;

class VulkanDevice final : public gpu::Device
{
public:
	VulkanDevice(VulkanInstance& instance, VulkanPhysicalDevice& physicalDevice, std::vector<uint32> queueFamilyIndices);

	~VulkanDevice() override;

	void EndFrame() override;

	void SubmitCommands(gpu::CommandBuffer& cmdBuf) override;

	void SubmitCommands(
		gpu::CommandBuffer& cmdBuf,
		const gpu::Semaphore& waitSemaphore,
		const gpu::Semaphore& signalSemaphore
	) override;

	gpu::CommandBuffer CreateCommandBuffer(EQueue queue) override;

	gpu::Pipeline CreatePipeline(const GraphicsPipelineDesc& graphicsDesc) override;

	gpu::Pipeline CreatePipeline(const ComputePipelineDesc& computeDesc) override;

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

	VkDescriptorSet& GetTextures() override;

	VkDescriptorSet& GetImages() override;

	gpu::Semaphore CreateSemaphore() override;

	void RecompileShaders() override;

	void UpdateDescriptorSet(
		VkDescriptorSet descriptorSet, 
		VkDescriptorUpdateTemplate descriptorUpdateTemplate, 
		const void* data) override;

	void CreateDescriptorSetLayout(
		std::size_t numBindings, 
		const VkDescriptorSetLayoutBinding* bindings,
		VkDescriptorSetLayout& descriptorSetLayout,
		VkDescriptorUpdateTemplate& descriptorUpdateTemplate);

	/** Recompile all pipelines. */
	void RecompilePipelines();

	operator VkDevice() const { return _Device; }

	inline VulkanInstance& GetInstance() { return _Instance; }
	inline const VulkanPhysicalDevice& GetPhysicalDevice() const { return _PhysicalDevice; }
	inline VulkanQueue& GetGraphicsQueue() { return _GraphicsQueue; }
	inline VulkanQueue& GetTransferQueue() { return _TransferQueue; }
	inline VkDescriptorPool& GetDescriptorPool() { return _DescriptorPool; }
	
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

	static const std::vector<const char*>& GetRequiredExtensions();

	std::unique_ptr<VulkanBindlessDescriptors> _BindlessTextures;

	std::unique_ptr<VulkanBindlessDescriptors> _BindlessImages;

private:
	VulkanInstance& _Instance;

	VulkanPhysicalDevice& _PhysicalDevice;

	VmaAllocator _Allocator;

	VulkanQueue _GraphicsQueue;

	VulkanQueue _TransferQueue;

	VkDevice _Device;

	VkDescriptorPool _DescriptorPool;

	PFN_vkCreateDescriptorUpdateTemplateKHR _VkCreateDescriptorUpdateTemplateKHR;

	PFN_vkUpdateDescriptorSetWithTemplateKHR _VkUpdateDescriptorSetWithTemplateKHR;

	/** Vulkan Resource Caches */

	std::unordered_map<Crc, VkRenderPass> _RenderPassCache;

	std::unordered_map<Crc, gpu::Pipeline> _GraphicsPipelineCache;

	std::unordered_map<Crc, GraphicsPipelineDesc> _CrcToGraphicsPipelineDesc;

	std::unordered_map<Crc, gpu::Pipeline> _ComputePipelineCache;

	std::unordered_map<Crc, ComputePipelineDesc> _CrcToComputeDesc;

	std::unordered_map<Crc, VkPipelineLayout> _PipelineLayoutCache;

	std::unordered_map<Crc, std::pair<VkDescriptorSetLayout, VkDescriptorUpdateTemplate>> _DescriptorSetLayoutCache;

	std::unordered_map<Crc, gpu::Sampler> _SamplerCache;

	ShaderCompilationResult CompileShader(
		const ShaderCompilerWorker& worker,
		const std::filesystem::path& path,
		const std::string& entrypoint,
		EShaderStage stage) override;

	std::pair<VkRenderPass, VkFramebuffer> GetOrCreateRenderPass(const RenderPassDesc& rpDesc);
	[[nodiscard]] static VkRenderPass CreateRenderPass(VkDevice device, const RenderPassDesc& rpDesc);
	[[nodiscard]] VkFramebuffer CreateFramebuffer(VkRenderPass renderPass, const RenderPassDesc& rpDesc) const;
	[[nodiscard]] VkPipeline CreatePipeline(const GraphicsPipelineDesc& graphicsDesc, VkPipelineLayout pipelineLayout) const;
	[[nodiscard]] VkPipeline CreatePipeline(const ComputePipelineDesc& computeDesc, VkPipelineLayout pipelineLayout) const;
	VkPipelineLayout GetOrCreatePipelineLayout(
		const std::vector<VkDescriptorSetLayout>& Layouts,
		const std::vector<VkPushConstantRange>& PushConstantRanges);
};

#define vulkan(result) \
	check(result == VK_SUCCESS, "%s", VulkanDevice::GetErrorString(result));