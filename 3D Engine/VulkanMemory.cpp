#include "VulkanMemory.h"
#include "VulkanDevice.h"
#include "Platform.h"

VulkanAllocator::VulkanAllocator(VulkanDevice& Device) 
	: Device(Device) 
{
}

uint32 VulkanAllocator::FindMemoryType(uint32 MemoryTypeBitsRequirement, VkMemoryPropertyFlags RequiredProperties)
{
	VkPhysicalDeviceMemoryProperties MemProperties;
	vkGetPhysicalDeviceMemoryProperties(Device, &MemProperties);

	const uint32_t memoryCount = MemProperties.memoryTypeCount;
	for (uint32_t memoryIndex = 0; memoryIndex < memoryCount; ++memoryIndex)
	{
		const uint32_t memoryTypeBits = (1 << memoryIndex);
		const bool isRequiredMemoryType = MemoryTypeBitsRequirement & memoryTypeBits;

		const VkMemoryPropertyFlags properties =
			MemProperties.memoryTypes[memoryIndex].propertyFlags;
		const bool hasRequiredProperties =
			(properties & RequiredProperties) == RequiredProperties;

		if (isRequiredMemoryType && hasRequiredProperties)
			return static_cast<int32_t>(memoryIndex);
	}

	fail("Suitable memory not found.");

}
