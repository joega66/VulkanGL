#pragma once
#include <DRM.h>
#include "VulkanDevice.h"
#include "VulkanRenderPass.h"
#include "VulkanSwapchain.h"
#include "VulkanMemory.h"
#include "VulkanDescriptors.h"
#include "VulkanCommandList.h"

class VulkanDRM final : public DRM
{
public:
	VulkanDRM(Platform& Platform, class Screen& Screen);

	virtual ~VulkanDRM() override {}

	virtual void BeginFrame() override;
	virtual void EndFrame() override;
	virtual void SubmitCommands(drm::CommandListRef CmdList) override;
	virtual drm::CommandListRef CreateCommandList() override;
	virtual drm::DescriptorSetRef CreateDescriptorSet() override;
	virtual drm::BufferRef CreateBuffer(EBufferUsage Usage, uint32 Size, const void* Data = nullptr) override;
	virtual drm::ImageRef CreateImage(uint32 Width, uint32 Height, uint32 Depth, EFormat Format, EImageUsage UsageFlags) override;
	virtual drm::ImageRef GetSurface() override;
	virtual void* LockBuffer(drm::BufferRef Buffer) override;
	virtual void UnlockBuffer(drm::BufferRef Buffer) override;
	virtual drm::RenderPassRef CreateRenderPass(RenderPassInitializer& RPInit) override;

	VulkanDevice Device;

private:
	VulkanSwapchain Swapchain;
	VulkanAllocator Allocator;
	VulkanDescriptorPool DescriptorPool;
	uint32 SwapchainIndex = -1;
	VkSemaphore ImageAvailableSem = VK_NULL_HANDLE;
	VkSemaphore RenderEndSem = VK_NULL_HANDLE;
};

CLASS(VulkanDRM);

namespace
{
	CAST(drm::Buffer, VulkanBuffer);
	CAST(drm::Image, VulkanImage);
	CAST(drm::CommandList, VulkanCommandList);
	CAST(drm::DescriptorSet, VulkanDescriptorSet);
	CAST(drm::RenderPass, VulkanRenderPass);
}