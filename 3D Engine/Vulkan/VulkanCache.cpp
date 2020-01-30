#include "VulkanCache.h"
#include "VulkanDevice.h"

VulkanCache::VulkanCache(VulkanDevice& Device)
	: Device(Device)
{
}

VulkanCache::~VulkanCache()
{
	for (const auto&[DescriptorSetLayouts, PipelineLayout] : PipelineLayoutCache)
	{
		vkDestroyPipelineLayout(Device, PipelineLayout, nullptr);
	}

	for (const auto&[PipelineHash, Pipeline] : PipelineCache)
	{
		vkDestroyPipeline(Device, Pipeline, nullptr);
	}

	for (const auto&[CacheInfo, RenderPass] : RenderPassCache)
	{
		vkDestroyRenderPass(Device, RenderPass, nullptr);
	}
}

const char* VulkanCache::GetVulkanErrorString(VkResult Result)
{
	const char* ErrorString = nullptr;
	switch (Result)
	{
	case VK_NOT_READY:
		ErrorString = "VK_NOT_READY";
		break;
	case VK_TIMEOUT:
		ErrorString = "VK_TIMEOUT";
		break;
	case VK_EVENT_SET:
		ErrorString = "VK_EVENT_SET";
		break;
	case VK_EVENT_RESET:
		ErrorString = "VK_EVENT_RESET";
		break;
	case VK_INCOMPLETE:
		ErrorString = "VK_INCOMPLETE";
		break;
	case VK_ERROR_OUT_OF_HOST_MEMORY:
		ErrorString = "VK_ERROR_OUT_OF_HOST_MEMORY";
		break;
	case VK_ERROR_OUT_OF_DEVICE_MEMORY:
		ErrorString = "VK_ERROR_OUT_OF_DEVICE_MEMORY";
		break;
	case VK_ERROR_INITIALIZATION_FAILED:
		ErrorString = "VK_ERROR_INITIALIZATION_FAILED";
		break;
	case VK_ERROR_DEVICE_LOST:
		ErrorString = "VK_ERROR_DEVICE_LOST";
		break;
	case VK_ERROR_MEMORY_MAP_FAILED:
		ErrorString = "VK_ERROR_MEMORY_MAP_FAILED";
		break;
	case VK_ERROR_LAYER_NOT_PRESENT:
		ErrorString = "VK_ERROR_LAYER_NOT_PRESENT";
		break;
	case VK_ERROR_EXTENSION_NOT_PRESENT:
		ErrorString = "VK_ERROR_EXTENSION_NOT_PRESENT";
		break;
	case VK_ERROR_FEATURE_NOT_PRESENT:
		ErrorString = "VK_ERROR_FEATURE_NOT_PRESENT";
		break;
	case VK_ERROR_INCOMPATIBLE_DRIVER:
		ErrorString = "VK_ERROR_INCOMPATIBLE_DRIVER";
		break;
	case VK_ERROR_TOO_MANY_OBJECTS:
		ErrorString = "VK_ERROR_TOO_MANY_OBJECTS";
		break;
	case VK_ERROR_FORMAT_NOT_SUPPORTED:
		ErrorString = "VK_ERROR_FORMAT_NOT_SUPPORTED";
		break;
	case VK_ERROR_FRAGMENTED_POOL:
		ErrorString = "VK_ERROR_FRAGMENTED_POOL";
		break;
	case VK_ERROR_OUT_OF_POOL_MEMORY:
		ErrorString = "VK_ERROR_OUT_OF_POOL_MEMORY";
		break;
	case VK_ERROR_INVALID_EXTERNAL_HANDLE:
		ErrorString = "VK_ERROR_INVALID_EXTERNAL_HANDLE";
		break;
	case VK_ERROR_SURFACE_LOST_KHR:
		ErrorString = "VK_ERROR_SURFACE_LOST_KHR";
		break;
	case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
		ErrorString = "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
		break;
	case VK_SUBOPTIMAL_KHR:
		ErrorString = "VK_SUBOPTIMAL_KHR";
		break;
	case VK_ERROR_OUT_OF_DATE_KHR:
		ErrorString = "VK_ERROR_OUT_OF_DATE_KHR";
		break;
	case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
		ErrorString = "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
		break;
	}
	return ErrorString;
}
