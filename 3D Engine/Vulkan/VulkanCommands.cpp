#include "VulkanCommands.h"
#include "VulkanDevice.h"

VulkanScopedCommandBuffer::VulkanScopedCommandBuffer(VulkanDevice& Device, VkQueueFlags QueueFlags)
	: Device(Device)
	, Queue(Device.Queues.GetQueue(QueueFlags))
	, CommandPool(Device.Queues.GetCommandPool(QueueFlags))
{
	VkCommandBufferAllocateInfo Info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	Info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	Info.commandPool = CommandPool;
	Info.commandBufferCount = 1;

	vulkan(vkAllocateCommandBuffers(Device, &Info, &CommandBuffer));

	VkCommandBufferBeginInfo BeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vulkan(vkBeginCommandBuffer(CommandBuffer, &BeginInfo));
}

VulkanScopedCommandBuffer::~VulkanScopedCommandBuffer()
{
	vkEndCommandBuffer(CommandBuffer);

	VkSubmitInfo SubmitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	SubmitInfo.commandBufferCount = 1;
	SubmitInfo.pCommandBuffers = &CommandBuffer;

	vulkan(vkQueueSubmit(Queue, 1, &SubmitInfo, VK_NULL_HANDLE));
	vulkan(vkQueueWaitIdle(Queue));
	vkFreeCommandBuffers(Device, CommandPool, 1, &CommandBuffer);
}