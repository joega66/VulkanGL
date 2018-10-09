#include "VulkanCommands.h"
#include "VulkanDevice.h"

VulkanScopedCommandBuffer::VulkanScopedCommandBuffer(VulkanDevice & Device) 
	: Device(Device) 
{
	VkCommandBufferAllocateInfo Info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	Info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	Info.commandPool = Device.CommandPool;
	Info.commandBufferCount = 1;

	vkAllocateCommandBuffers(Device, &Info, &CommandBuffer);

	VkCommandBufferBeginInfo BeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(CommandBuffer, &BeginInfo);
}

VulkanScopedCommandBuffer::~VulkanScopedCommandBuffer()
{
	vkEndCommandBuffer(CommandBuffer);

	VkSubmitInfo SubmitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	SubmitInfo.commandBufferCount = 1;
	SubmitInfo.pCommandBuffers = &CommandBuffer;

	vkQueueSubmit(Device.GraphicsQueue, 1, &SubmitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(Device.GraphicsQueue);
	vkFreeCommandBuffers(Device, Device.CommandPool, 1, &CommandBuffer);
}