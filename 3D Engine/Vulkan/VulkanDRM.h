#pragma once
#include "../DRM.h"
#include "VulkanDevice.h"
#include "VulkanSurface.h"
#include "VulkanMemory.h"
#include "VulkanDescriptors.h"
#include "VulkanShader.h"
#include "VulkanCommandList.h"

class VulkanDRM final : public DRM
{
public:
	VulkanDRM();

	virtual void BeginFrame();
	virtual void EndFrame();
	virtual void SubmitCommands(drm::CommandListRef CmdList);
	virtual drm::CommandListRef CreateCommandList();
	virtual drm::DescriptorSetRef CreateDescriptorSet();
	virtual drm::BufferRef CreateBuffer(EBufferUsage Usage, uint32 Size, const void* Data = nullptr);
	virtual drm::ImageRef CreateImage(uint32 Width, uint32 Height, uint32 Depth, EFormat Format, EImageUsage UsageFlags, const uint8* Data);
	virtual drm::ImageRef CreateCubemap(uint32 Width, uint32 Height, EFormat Format, EImageUsage UsageFlags, const CubemapCreateInfo& CubemapCreateInfo);
	virtual drm::ImageRef GetSurface();
	virtual void* LockBuffer(drm::BufferRef Buffer);
	virtual void UnlockBuffer(drm::BufferRef Buffer);
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

	void CreateImage(VkImage& Image, VkDeviceMemory& Memory, EImageLayout& Layout, uint32 Width, uint32 Height, uint32 Depth, EFormat& Format, EImageUsage UsageFlags, bool bTransferDstBit);
};

CLASS(VulkanDRM);

namespace
{
	CAST(drm::Buffer, VulkanBuffer);
	CAST(drm::Image, VulkanImage);
	CAST(drm::CommandList, VulkanCommandList);
	CAST(drm::DescriptorSet, VulkanDescriptorSet);
}