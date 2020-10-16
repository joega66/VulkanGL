#include "VulkanInstance.h"
#include "VulkanDevice.h"

static bool CheckValidationLayerSupport(const std::vector<const char*>& validationLayers)
{
	uint32 layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : validationLayers)
	{
		if (![] (const char* layerName, const std::vector<VkLayerProperties>& availableLayers)
		{
			for (const auto& layerProperties : availableLayers)
			{
				if (strcmp(layerName, layerProperties.layerName) == 0)
				{
					return true;
				}
			}

			return false;
		} (layerName, availableLayers))
		{
			return false;
		}
	}

	return true;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType,
	uint64 obj, size_t location, int32 code, const char* layerPrefix, const char* msg, void* userData)
{
	LOG("Validation layer: %s\nLayer prefix: %s", msg, layerPrefix);
	return false;
}

VulkanInstance::VulkanInstance(bool useValidationLayers)
{
	if (useValidationLayers)
	{
		_ValidationLayers =
		{
			"VK_LAYER_LUNARG_standard_validation"
		};
	}
	
	if (UseValidationLayers() && !CheckValidationLayerSupport(_ValidationLayers))
	{
		fail("Validation layers requested, but are unavailable.");
	}

	VkApplicationInfo applicationInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
	applicationInfo.pApplicationName = "Vulkan Renderer";
	applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	applicationInfo.pEngineName = "No Engine";
	applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	applicationInfo.apiVersion = VK_API_VERSION_1_2;

	std::vector<const char*> extensions;

	if (UseValidationLayers())
	{
		extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}

	extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

#if _WIN32
	extensions.push_back("VK_KHR_win32_surface");
#endif

	VkInstanceCreateInfo instanceInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	instanceInfo.pApplicationInfo = &applicationInfo;
	instanceInfo.enabledExtensionCount = static_cast<uint32>(extensions.size());
	instanceInfo.ppEnabledExtensionNames = extensions.data();

	if (UseValidationLayers())
	{
		instanceInfo.enabledLayerCount = static_cast<uint32>(_ValidationLayers.size());
		instanceInfo.ppEnabledLayerNames = _ValidationLayers.data();
	}
	else
	{
		instanceInfo.enabledLayerCount = 0;
	}

	vulkan(vkCreateInstance(&instanceInfo, nullptr, &_Instance));

	if (UseValidationLayers())
	{
		const VkDebugReportCallbackCreateInfoEXT debugReportCallbackInfo = 
		{ 
			.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
			.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT,
			.pfnCallback = DebugCallback
		};

		auto vkCreateDebugReportCallbackEXT = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(_Instance, "vkCreateDebugReportCallbackEXT"));
		check(vkCreateDebugReportCallbackEXT, "Failed to get proc address vkCreateDebugReportCallbackEXT");
		vkCreateDebugReportCallbackEXT(_Instance, &debugReportCallbackInfo, nullptr, &_DebugReportCallback);
	}
}