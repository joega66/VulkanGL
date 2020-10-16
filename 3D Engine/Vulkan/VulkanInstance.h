#pragma once
#include <Vulkan/vulkan.h>
#include <vector>

class VulkanInstance
{
public:
	VulkanInstance(bool useValidationLayers);

	inline operator VkInstance& () { return _Instance; }
	inline bool UseValidationLayers() const { return _ValidationLayers.size() > 0; }
	inline const std::vector<const char*>& GetValidationLayers() const { return _ValidationLayers; }

private:
	VkInstance _Instance;

	VkDebugReportCallbackEXT _DebugReportCallback;

	std::vector<const char*> _ValidationLayers;
};