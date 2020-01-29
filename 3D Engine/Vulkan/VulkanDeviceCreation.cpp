#include "VulkanDevice.h"
#include <GLFW/glfw3.h>

static bool CheckValidationLayerSupport(const std::vector<const char*>& ValidationLayers)
{
	uint32 LayerCount;
	vkEnumerateInstanceLayerProperties(&LayerCount, nullptr);
	std::vector<VkLayerProperties> AvailableLayers(LayerCount);
	vkEnumerateInstanceLayerProperties(&LayerCount, AvailableLayers.data());

	for (const char* LayerName : ValidationLayers)
	{
		if (![] (const char* LayerName, const std::vector<VkLayerProperties>& AvailableLayers)
		{
			for (const auto& LayerProperties : AvailableLayers)
			{
				if (strcmp(LayerName, LayerProperties.layerName) == 0)
				{
					return true;
				}
			}

			return false;
		} (LayerName, AvailableLayers))
		{
			return false;
		}
	}

	return true;
}

static std::vector<const char*> GetRequiredExtensions(bool bUseValidationLayers)
{
	uint32 GLFWExtensionCount = 0;
	const char** GLFWExtensions;
	GLFWExtensions = glfwGetRequiredInstanceExtensions(&GLFWExtensionCount);
	std::vector<const char*> Extensions(GLFWExtensions, GLFWExtensions + GLFWExtensionCount);

	if (bUseValidationLayers)
	{
		Extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}

	return Extensions;
}

static VkInstance CreateInstance(const std::vector<const char*>& ValidationLayers, bool bUseValidationLayers)
{
	if (bUseValidationLayers && !CheckValidationLayerSupport(ValidationLayers))
	{
		fail("Validation layers requested, but are unavailable.");
	}

	VkApplicationInfo ApplicationInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
	ApplicationInfo.pApplicationName = "Vulkan Engine";
	ApplicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	ApplicationInfo.pEngineName = "No Engine";
	ApplicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	ApplicationInfo.apiVersion = VK_API_VERSION_1_0;

	auto Extensions = GetRequiredExtensions(bUseValidationLayers);

	VkInstanceCreateInfo InstanceInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	InstanceInfo.pApplicationInfo = &ApplicationInfo;
	InstanceInfo.enabledExtensionCount = static_cast<uint32>(Extensions.size());
	InstanceInfo.ppEnabledExtensionNames = Extensions.data();

	if (bUseValidationLayers)
	{
		InstanceInfo.enabledLayerCount = static_cast<uint32>(ValidationLayers.size());
		InstanceInfo.ppEnabledLayerNames = ValidationLayers.data();
	}
	else
	{
		InstanceInfo.enabledLayerCount = 0;
	}

	VkInstance Instance;
	vulkan(vkCreateInstance(&InstanceInfo, nullptr, &Instance));

	return Instance;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugReportFlagsEXT Flags, VkDebugReportObjectTypeEXT ObjType,
	uint64 Obj, size_t Location, int32 Code, const char* LayerPrefix, const char* Msg, void* UserData)
{
	LOG("Validation layer: %s\nLayer prefix: %s", Msg, LayerPrefix);
	return VK_FALSE;
}

static VkDebugReportCallbackEXT CreateDebugReportCallback(VkInstance Instance, bool bUseValidationLayers)
{
	if (bUseValidationLayers)
	{
		// Create Vulkan debug callback
		VkDebugReportCallbackCreateInfoEXT DebugCallbackInfo = { VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT };
		DebugCallbackInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
		DebugCallbackInfo.pfnCallback = DebugCallback;

		auto CreateDebugReportCallbackEXT = [] (VkInstance Instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
			const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback)
		{
			auto Func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(Instance, "vkCreateDebugReportCallbackEXT");
			if (Func)
			{
				return Func(Instance, pCreateInfo, pAllocator, pCallback);
			}
			else
			{
				return VK_ERROR_EXTENSION_NOT_PRESENT;
			}
		};

		VkDebugReportCallbackEXT DebugReportCallback;
		vulkan(CreateDebugReportCallbackEXT(Instance, &DebugCallbackInfo, nullptr, &DebugReportCallback));
	}

	return 0;
}

static bool AllDeviceExtensionsSupported(VkPhysicalDevice Device, const std::vector<const char*>& DeviceExtensions)
{
	uint32 ExtensionCount;
	vkEnumerateDeviceExtensionProperties(Device, nullptr, &ExtensionCount, nullptr);

	std::vector<VkExtensionProperties> AvailableExtensions(ExtensionCount);
	vkEnumerateDeviceExtensionProperties(Device, nullptr, &ExtensionCount, AvailableExtensions.data());

	std::unordered_set<std::string> RequiredExtensions(DeviceExtensions.begin(), DeviceExtensions.end());

	for (const auto& Extension : AvailableExtensions)
	{
		RequiredExtensions.erase(Extension.extensionName);
	}

	return RequiredExtensions.empty();
}

/** Select a Vulkan-capable physical device. */
static VkPhysicalDevice SelectPhysicalDevice(VkInstance Instance, const std::vector<const char*>& DeviceExtensions)
{
	uint32 DeviceCount = 0;
	vkEnumeratePhysicalDevices(Instance, &DeviceCount, nullptr);

	check(DeviceCount != 0, "Failed to find GPUs with Vulkan support.");

	std::vector<VkPhysicalDevice> Devices(DeviceCount);
	vkEnumeratePhysicalDevices(Instance, &DeviceCount, Devices.data());

	VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;

	for (const auto& Device : Devices)
	{
		if (AllDeviceExtensionsSupported(Device, DeviceExtensions))
		{
			PhysicalDevice = Device;
			break;
		}
	}

	check(PhysicalDevice != VK_NULL_HANDLE, "Failed to find a suitable GPU.");

	return PhysicalDevice;
}

static VkPhysicalDeviceProperties GetPhysicalDeviceProperties(VkPhysicalDevice PhysicalDevice)
{
	VkPhysicalDeviceProperties Properties;
	vkGetPhysicalDeviceProperties(PhysicalDevice, &Properties);
	return Properties;
}

static VkPhysicalDeviceFeatures GetPhysicalDeviceFeatures(VkPhysicalDevice PhysicalDevice)
{
	VkPhysicalDeviceFeatures Features;
	vkGetPhysicalDeviceFeatures(PhysicalDevice, &Features);
	return Features;
}

VulkanDevice::VulkanDevice(Platform& Platform)
	: Instance(CreateInstance(ValidationLayers, Platform::GetBool("Engine.ini", "Renderer", "UseValidationLayers", false)))
	, DebugReportCallback(CreateDebugReportCallback(Instance, Platform::GetBool("Engine.ini", "Renderer", "UseValidationLayers", false)))
	, PhysicalDevice(SelectPhysicalDevice(Instance, DeviceExtensions))
	, Queues(PhysicalDevice)
	, Allocator(*this)
	, Properties(GetPhysicalDeviceProperties(PhysicalDevice))
	, Features(GetPhysicalDeviceFeatures(PhysicalDevice))
	, VulkanCache(*this)
{
}