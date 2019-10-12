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
	virtual void EndFrame();

	virtual void SubmitCommands(RenderCommandListRef CmdList);
	virtual RenderCommandListRef CreateCommandList();
	virtual drm::DescriptorSetRef CreateDescriptorSet();
	virtual drm::IndexBufferRef CreateIndexBuffer(EFormat Format, uint32 NumIndices, EBufferUsage Usage, const void* Data = nullptr);
	virtual drm::VertexBufferRef CreateVertexBuffer(EFormat Format, uint32 NumElements, EBufferUsage Usage, const void* Data = nullptr);
	virtual drm::UniformBufferRef CreateUniformBuffer(uint32 Size, const void* Data, EUniformUpdate Usage);
	virtual drm::StorageBufferRef CreateStorageBuffer(uint32 Size, const void* Data, EBufferUsage Usage);
	virtual drm::ImageRef CreateImage(uint32 Width, uint32 Height, EFormat Format, EImageUsage UsageFlags, const uint8* Data);
	virtual drm::ImageRef CreateCubemap(uint32 Width, uint32 Height, EFormat Format, EImageUsage UsageFlags, const CubemapCreateInfo& CubemapCreateInfo);
	virtual drm::RenderTargetViewRef CreateRenderTargetView(drm::ImageRef Image, ELoadAction LoadAction, EStoreAction StoreAction, const ClearColorValue& ClearValue, EImageLayout FinalLayout);
	virtual drm::RenderTargetViewRef CreateRenderTargetView(drm::ImageRef Image, ELoadAction LoadAction, EStoreAction StoreAction, const ClearDepthStencilValue& DepthStencil, EImageLayout FinalLayout);
	virtual drm::ImageRef GetSurface();
	virtual drm::RenderTargetViewRef GetSurfaceView(ELoadAction LoadAction, EStoreAction StoreAction, const ClearColorValue& ClearValue);
	virtual void* LockBuffer(drm::VertexBufferRef VertexBuffer);
	virtual void UnlockBuffer(drm::VertexBufferRef VertexBuffer);
	virtual void* LockBuffer(drm::IndexBufferRef IndexBuffer);
	virtual void UnlockBuffer(drm::IndexBufferRef IndexBuffer);
	virtual void* LockBuffer(drm::StorageBufferRef StorageBuffer);
	virtual void UnlockBuffer(drm::StorageBufferRef StorageBuffer);
	virtual std::string GetDRMName() { return "Vulkan"; }
	virtual ShaderCompilationInfo CompileShader(const ShaderCompilerWorker& Worker, const ShaderMetadata& Meta);
	virtual void RecompileShaders();

private:
	VulkanDevice Device;
	VulkanSurface Swapchain;
	VulkanAllocator Allocator;
	VulkanDescriptorPool DescriptorPool;
	uint32 SwapchainIndex = -1;
	VkSemaphore ImageAvailableSem = VK_NULL_HANDLE;
	VkSemaphore RenderEndSem = VK_NULL_HANDLE;

	void TransitionImageLayout(VulkanImageRef Image, VkAccessFlags SrcAccessMask, VkAccessFlags DstAccessMask, EImageLayout NewLayout, VkPipelineStageFlags DestinationStage);
	void CreateImage(VkImage& Image, VkDeviceMemory& Memory, EImageLayout& Layout, uint32 Width, uint32 Height, EFormat& Format, EImageUsage UsageFlags, bool bTransferDstBit);
};

CLASS(VulkanDRM);

extern HashTable<VkFormat, uint32> SizeOfVulkanFormat;
extern HashTable<EFormat, uint32> ImageFormatToGLSLSize;