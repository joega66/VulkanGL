#include "VulkanDevice.h"
#include "VulkanSwapchain.h"
#include <Platform/Platform.h>
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

static VkSurfaceKHR CreateSurface(Platform& Platform, VkInstance Instance)
{
	VkSurfaceKHR Surface;
	vulkan(glfwCreateWindowSurface(Instance, Platform.Window, nullptr, &Surface));
	return Surface;
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

static bool IsDeviceSuitable(VkPhysicalDevice Device, VkSurfaceKHR Surface, const std::vector<const char*>& DeviceExtensions)
{
	VulkanQueues Queues(Device, Surface);

	if (!Queues.IsComplete())
	{
		return false;
	}

	if (!AllDeviceExtensionsSupported(Device, DeviceExtensions))
	{
		return false;
	}

	SwapchainSupportDetails SwapchainSupport(Device, Surface);

	if (SwapchainSupport.Formats.empty() || SwapchainSupport.PresentModes.empty())
	{
		return false;
	}

	return true;
}

/** Select a Vulkan-capable physical device. */
static VkPhysicalDevice SelectPhysicalDevice(VkInstance Instance, VkSurfaceKHR Surface, const std::vector<const char*>& DeviceExtensions)
{
	uint32 DeviceCount = 0;
	vkEnumeratePhysicalDevices(Instance, &DeviceCount, nullptr);

	check(DeviceCount != 0, "Failed to find GPUs with Vulkan support.");

	std::vector<VkPhysicalDevice> Devices(DeviceCount);
	vkEnumeratePhysicalDevices(Instance, &DeviceCount, Devices.data());

	VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;

	for (const auto& Device : Devices)
	{
		if (IsDeviceSuitable(Device, Surface, DeviceExtensions))
		{
			PhysicalDevice = Device;
			break;
		}
	}

	check(PhysicalDevice != VK_NULL_HANDLE, "Failed to find a suitable GPU.");

	return PhysicalDevice;
}

static VkDevice CreateLogicalDevice(VkPhysicalDevice PhysicalDevice,
	VkSurfaceKHR Surface,
	const VulkanQueues& Queues,
	const std::vector<const char*>& DeviceExtensions,
	const std::vector<const char*>& ValidationLayers,
	bool bUseValidationLayers
)
{
	const std::unordered_set<int32> UniqueQueueFamilies = Queues.GetUniqueFamilies();
	const float QueuePriority = 1.0f;
	std::vector<VkDeviceQueueCreateInfo> QueueCreateInfos;

	for (int32 QueueFamily : UniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo QueueCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
		QueueCreateInfo.queueFamilyIndex = QueueFamily;
		QueueCreateInfo.queueCount = 1;
		QueueCreateInfo.pQueuePriorities = &QueuePriority;
		QueueCreateInfos.push_back(QueueCreateInfo);
	}

	VkPhysicalDeviceFeatures DeviceFeatures = {};
	DeviceFeatures.samplerAnisotropy = VK_TRUE;
	DeviceFeatures.geometryShader = VK_TRUE;
	DeviceFeatures.fragmentStoresAndAtomics = VK_TRUE;
	DeviceFeatures.vertexPipelineStoresAndAtomics = VK_TRUE;

	VkDeviceCreateInfo CreateInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	CreateInfo.queueCreateInfoCount = static_cast<uint32>(QueueCreateInfos.size());
	CreateInfo.pQueueCreateInfos = QueueCreateInfos.data();
	CreateInfo.pEnabledFeatures = &DeviceFeatures;
	CreateInfo.enabledExtensionCount = static_cast<uint32>(DeviceExtensions.size());
	CreateInfo.ppEnabledExtensionNames = DeviceExtensions.data();

	if (bUseValidationLayers)
	{
		CreateInfo.enabledLayerCount = static_cast<uint32>(ValidationLayers.size());
		CreateInfo.ppEnabledLayerNames = ValidationLayers.data();
	}
	else
	{
		CreateInfo.enabledLayerCount = 0;
	}

	VkDevice Device;
	vulkan(vkCreateDevice(PhysicalDevice, &CreateInfo, nullptr, &Device));

	return Device;
}

static const std::vector<const char*> ValidationLayers =
{
	"VK_LAYER_LUNARG_standard_validation"
};

static const std::vector<const char*> DeviceExtensions =
{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_MAINTENANCE1_EXTENSION_NAME,
};

VulkanDevice::VulkanDevice(Platform& Platform, bool bUseValidationLayers)
	: Instance(CreateInstance(ValidationLayers, bUseValidationLayers))
	, DebugReportCallback(CreateDebugReportCallback(Instance, bUseValidationLayers))
	, Surface(CreateSurface(Platform, Instance))
	, PhysicalDevice(SelectPhysicalDevice(Instance, Surface, DeviceExtensions))
	, Queues(PhysicalDevice, Surface)
	, Device(CreateLogicalDevice(PhysicalDevice, Surface, Queues, DeviceExtensions, ValidationLayers, bUseValidationLayers))
{
	// Create queues and command pools.
	Queues.Create(Device);

	// Get physical device properties/features
	vkGetPhysicalDeviceProperties(PhysicalDevice, &Properties);
	vkGetPhysicalDeviceFeatures(PhysicalDevice, &Features);
}

VulkanDevice::~VulkanDevice()
{
	for (const auto&[DescriptorSetLayouts, PipelineLayout] : PipelineLayoutCache)
	{
		vkDestroyPipelineLayout(Device, PipelineLayout, nullptr);
	}

	for (const auto&[PipelineHash, Pipeline] : PipelineCache)
	{
		vkDestroyPipeline(Device, Pipeline, nullptr);
	}

	for (const auto&[CacheInfo, RenderPass, Framebuffer] : RenderPassCache)
	{
		vkDestroyFramebuffer(Device, Framebuffer, nullptr);
		vkDestroyRenderPass(Device, RenderPass, nullptr);
	}
}

const char* VulkanDevice::GetVulkanErrorString(VkResult Result)
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
