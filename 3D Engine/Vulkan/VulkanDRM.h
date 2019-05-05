#pragma once
#include "../DRM.h"
#include "VulkanDevice.h"
#include "VulkanSurface.h"
#include "VulkanMemory.h"
#include "VulkanDescriptors.h"
#include "VulkanShader.h"

class VulkanDRM final : public DRM
{
public:
	VulkanDRM();

	virtual void Init();
	virtual void Release();

	virtual void BeginFrame();
	virtual void EndFrame(RenderCommandListRef CmdList);

	virtual RenderCommandListRef CreateCommandList();
	virtual GLIndexBufferRef CreateIndexBuffer(EImageFormat Format, uint32 NumIndices, EResourceUsage Usage, const void* Data = nullptr);
	virtual GLVertexBufferRef CreateVertexBuffer(EImageFormat Format, uint32 NumElements, EResourceUsage Usage, const void* Data = nullptr);
	virtual GLUniformBufferRef CreateUniformBuffer(uint32 Size, const void* Data, EUniformUpdate Usage);
	virtual GLStorageBufferRef CreateStorageBuffer(uint32 Size, const void* Data, EResourceUsage Usage);
	virtual GLImageRef CreateImage(uint32 Width, uint32 Height, EImageFormat Format, EResourceUsage UsageFlags, const uint8* Data);
	virtual GLImageRef CreateCubemap(uint32 Width, uint32 Height, EImageFormat Format, EResourceUsage UsageFlags, const CubemapCreateInfo& CubemapCreateInfo);
	virtual GLRenderTargetViewRef CreateRenderTargetView(GLImageRef Image, ELoadAction LoadAction, EStoreAction StoreAction, const std::array<float, 4>& ClearValue);
	virtual GLRenderTargetViewRef CreateRenderTargetView(GLImageRef Image, ELoadAction LoadAction, EStoreAction StoreAction, const ClearDepthStencilValue& DepthStencil);
	virtual GLImageRef GetSurface();
	virtual GLRenderTargetViewRef GetSurfaceView(ELoadAction LoadAction, EStoreAction StoreAction, const std::array<float, 4>& ClearValue);
	virtual void* LockBuffer(GLVertexBufferRef VertexBuffer, uint32 Size, uint32 Offset);
	virtual void UnlockBuffer(GLVertexBufferRef VertexBuffer);
	virtual void* LockBuffer(GLIndexBufferRef IndexBuffer, uint32 Size, uint32 Offset);
	virtual void UnlockBuffer(GLIndexBufferRef IndexBuffer);
	virtual void RebuildResolutionDependents();
	virtual std::string GetDRMName() { return "Vulkan"; }

	VulkanDevice& GetDevice() { return Device; }

private:
	VulkanDevice Device;
	VulkanSurface Swapchain;
	VulkanAllocator Allocator;
	VulkanDescriptorPool DescriptorPool;
	uint32 SwapchainIndex = -1;
	VkSemaphore ImageAvailableSem;
	VkSemaphore RenderEndSem;

	void TransitionImageLayout(VulkanImageRef Image, VkImageLayout NewLayout, VkPipelineStageFlags DestinationStage);
	VkFormat FindSupportedDepthFormat(EImageFormat Format);
	void CreateImage(VkImage& Image, VkDeviceMemory& Memory, VkImageLayout& Layout, uint32 Width, uint32 Height, EImageFormat& Format, EResourceUsage UsageFlags, bool bTransferDstBit);
	VkSampler CreateSampler(const SamplerState& Sampler);
};

CLASS(VulkanDRM);

extern HashTable<VkFormat, uint32> SizeOfVulkanFormat;