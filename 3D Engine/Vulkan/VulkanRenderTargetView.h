#pragma once
#include "VulkanImage.h"

class VulkanDevice;

class VulkanRenderTargetView : public drm::RenderTargetView
{
public:
	VulkanRenderTargetView(VulkanDevice& Device
		, drm::ImageRef Image
		, ELoadAction LoadAction
		, EStoreAction StoreAction
		, const ClearColorValue& ClearValue
		, EImageLayout FinalLayout)
		: Device(Device), drm::RenderTargetView(Image, LoadAction, StoreAction, ClearValue, FinalLayout)
	{
	}

	VulkanRenderTargetView(VulkanDevice& Device
		, drm::ImageRef Image
		, ELoadAction LoadAction
		, EStoreAction StoreAction
		, const ClearDepthStencilValue& DepthStencil
		, EImageLayout FinalLayout)
		: Device(Device), drm::RenderTargetView(Image, LoadAction, StoreAction, DepthStencil, FinalLayout)
	{
	}

	VkAttachmentLoadOp GetVulkanLoadOp() const;

	VkAttachmentStoreOp GetVulkanStoreOp() const;

private:
	VulkanDevice& Device;
};

CLASS(VulkanRenderTargetView);