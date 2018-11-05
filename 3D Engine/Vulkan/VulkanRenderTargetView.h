#pragma once
#include "VulkanImage.h"

class VulkanDevice;

class VulkanRenderTargetView : public GLRenderTargetView
{
public:
	VulkanRenderTargetView(VulkanDevice& Device
		, GLImageRef Image
		, ELoadAction LoadAction
		, EStoreAction StoreAction
		, const std::array<float, 4>& ClearValue) 
		: Device(Device), GLRenderTargetView(Image, LoadAction, StoreAction, ClearValue) 
	{
	}

	VulkanRenderTargetView(VulkanDevice& Device
		, GLImageRef Image
		, ELoadAction LoadAction
		, EStoreAction StoreAction
		, float DepthClear
		, uint32 StencilClear)
		: Device(Device), GLRenderTargetView(Image, LoadAction, StoreAction, DepthClear, StencilClear)
	{
	}

	VkAttachmentLoadOp GetVulkanLoadOp() const;
	VkAttachmentStoreOp GetVulkanStoreOp() const;

private:
	VulkanDevice& Device;
};

CLASS(VulkanRenderTargetView);