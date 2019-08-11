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
		, const ClearColorValue& ClearValue)
		: Device(Device), drm::RenderTargetView(Image, LoadAction, StoreAction, ClearValue) 
	{
	}

	VulkanRenderTargetView(VulkanDevice& Device
		, drm::ImageRef Image
		, ELoadAction LoadAction
		, EStoreAction StoreAction
		, const ClearDepthStencilValue& DepthStencil)
		: Device(Device), drm::RenderTargetView(Image, LoadAction, StoreAction, DepthStencil)
	{
	}

	VkAttachmentLoadOp GetVulkanLoadOp() const;

	VkAttachmentStoreOp GetVulkanStoreOp() const;

private:
	VulkanDevice& Device;
};

CLASS(VulkanRenderTargetView);