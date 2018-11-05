#include "VulkanRenderTargetView.h"
#include "VulkanDevice.h"

VkAttachmentLoadOp VulkanRenderTargetView::GetVulkanLoadOp() const
{
	if (LoadAction == ELoadAction::Clear)
	{
		return VK_ATTACHMENT_LOAD_OP_CLEAR;
	}
	else if (LoadAction == ELoadAction::Load)
	{
		return VK_ATTACHMENT_LOAD_OP_LOAD;
	}
	else // ELoadAction::None
	{
		return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	}
}

VkAttachmentStoreOp VulkanRenderTargetView::GetVulkanStoreOp() const
{
	if (StoreAction == EStoreAction::Store)
	{
		return VK_ATTACHMENT_STORE_OP_STORE;
	}
	else // EStoreAction::None
	{
		return VK_ATTACHMENT_STORE_OP_DONT_CARE;
	}
}
