#include "VulkanDevice.h"
#include "VulkanSurface.h"
#include "VulkanDRM.h"
#include <Platform/WindowsPlatform.h> // Yuck
#include <GLFW/glfw3.h>

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugReportFlagsEXT Flags, VkDebugReportObjectTypeEXT ObjType,
	uint64 Obj, size_t Location, int32 Code, const char* LayerPrefix, const char* Msg, void* UserData)
{
	print("Validation layer: %s\nLayer prefix: %s", Msg, LayerPrefix);
	return VK_FALSE;
}

static std::vector<const char*> GetRequiredExtensions()
{
	uint32 GLFWExtensionCount = 0;
	const char** GLFWExtensions;
	GLFWExtensions = glfwGetRequiredInstanceExtensions(&GLFWExtensionCount);
	std::vector<const char*> Extensions(GLFWExtensions, GLFWExtensions + GLFWExtensionCount);

#ifdef DEBUG_BUILD
	Extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif

	return Extensions;
}

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

static bool CheckDeviceExtensionSupport(VkPhysicalDevice Device, const std::vector<const char*> &DeviceExtensions)
{
	uint32 ExtensionCount;
	vkEnumerateDeviceExtensionProperties(Device, nullptr, &ExtensionCount, nullptr);

	std::vector<VkExtensionProperties> AvailableExtensions(ExtensionCount);
	vkEnumerateDeviceExtensionProperties(Device, nullptr, &ExtensionCount, AvailableExtensions.data());

	std::unordered_set<std::string> RequiredExtensions(DeviceExtensions.begin(), DeviceExtensions.end());

	for (const auto &Extension : AvailableExtensions)
	{
		RequiredExtensions.erase(Extension.extensionName);
	}

	return RequiredExtensions.empty();
}

static bool IsDeviceSuitable(VkPhysicalDevice Device, VkSurfaceKHR Surface, const std::vector<const char*>& DeviceExtensions)
{
	QueueFamilyIndices Indices = {};
	Indices.FindQueueFamilies(Device, Surface);

	bool ExtensionsSupported = CheckDeviceExtensionSupport(Device, DeviceExtensions);
	bool SwapChainAdequate = false;

	if (ExtensionsSupported)
	{
		SwapchainSupportDetails SwapchainSupport = {};
		SwapchainSupport.QuerySwapchainSupport(Device, Surface);
		SwapChainAdequate = !SwapchainSupport.Formats.empty() && !SwapchainSupport.PresentModes.empty();
	}

	VkPhysicalDeviceFeatures SupportedFeatures;
	vkGetPhysicalDeviceFeatures(Device, &SupportedFeatures);

	return Indices.IsComplete() && ExtensionsSupported && SwapChainAdequate && SupportedFeatures.samplerAnisotropy;
}

static VkPhysicalDevice SelectPhysicalDevice(VkInstance Instance, VkSurfaceKHR Surface, const std::vector<const char*>& DeviceExtensions)
{
	uint32 DeviceCount = 0;
	vkEnumeratePhysicalDevices(Instance, &DeviceCount, nullptr);

	if (DeviceCount == 0)
	{
		fail("Failed to find GPUs with Vulkan support.");
	}

	std::vector<VkPhysicalDevice> Devices(DeviceCount);
	vkEnumeratePhysicalDevices(Instance, &DeviceCount, Devices.data());

	VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;

	for (const auto &Device : Devices)
	{
		if (IsDeviceSuitable(Device, Surface, DeviceExtensions))
		{
			PhysicalDevice = Device;
			break;
		}
	}

	if (PhysicalDevice == VK_NULL_HANDLE)
	{
		fail("Failed to find a suitable GPU.");
	}

	return PhysicalDevice;
}

static VkDevice CreateLogicalDevice(VkPhysicalDevice PhysicalDevice,
	VkSurfaceKHR Surface,
	const std::vector<const char*>& DeviceExtensions,
	const std::vector<const char*>& ValidationLayers,
	VkQueue& GraphicsQueue,
	VkQueue& PresentQueue)
{
	QueueFamilyIndices Indices = {};
	Indices.FindQueueFamilies(PhysicalDevice, Surface);

	std::vector<VkDeviceQueueCreateInfo> QueueCreateInfos;
	std::unordered_set<int> UniqueQueueFamilies = { Indices.GraphicsFamily, Indices.PresentFamily };

	float QueuePriority = 1.0f;
	for (int QueueFamily : UniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo QueueCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
		QueueCreateInfo.queueFamilyIndex = QueueFamily;
		QueueCreateInfo.queueCount = 1;
		QueueCreateInfo.pQueuePriorities = &QueuePriority;
		QueueCreateInfos.push_back(QueueCreateInfo);
	}

	VkPhysicalDeviceFeatures DeviceFeatures = {};
	DeviceFeatures.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo CreateInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	CreateInfo.queueCreateInfoCount = static_cast<uint32>(QueueCreateInfos.size());
	CreateInfo.pQueueCreateInfos = QueueCreateInfos.data();
	CreateInfo.pEnabledFeatures = &DeviceFeatures;
	CreateInfo.enabledExtensionCount = static_cast<uint32>(DeviceExtensions.size());
	CreateInfo.ppEnabledExtensionNames = DeviceExtensions.data();

#ifdef DEBUG_BUILD
	CreateInfo.enabledLayerCount = static_cast<uint32>(ValidationLayers.size());
	CreateInfo.ppEnabledLayerNames = ValidationLayers.data();
#else
	CreateInfo.enabledLayerCount = 0;
#endif

	VkDevice Device;
	vulkan(vkCreateDevice(PhysicalDevice, &CreateInfo, nullptr, &Device));

	vkGetDeviceQueue(Device, Indices.GraphicsFamily, 0, &GraphicsQueue);
	vkGetDeviceQueue(Device, Indices.PresentFamily, 0, &PresentQueue);

	return Device;
}

static VkCommandPool CreateCommandPool(VkDevice Device,
	VkPhysicalDevice PhysicalDevice,
	VkSurfaceKHR Surface)
{
	QueueFamilyIndices QueueFamilyIndices = {};
	QueueFamilyIndices.FindQueueFamilies(PhysicalDevice, Surface);

	VkCommandPoolCreateInfo PoolInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	PoolInfo.queueFamilyIndex = QueueFamilyIndices.GraphicsFamily;
	PoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	VkCommandPool CommandPool;
	vulkan(vkCreateCommandPool(Device, &PoolInfo, nullptr, &CommandPool));

	return CommandPool;
}

VulkanDevice::VulkanDevice()
{
	const std::vector<const char*> ValidationLayers =
	{
		"VK_LAYER_LUNARG_standard_validation"
	};

	const std::vector<const char*> DeviceExtensions =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_MAINTENANCE1_EXTENSION_NAME
	};

#ifdef DEBUG_BUILD
	if (!CheckValidationLayerSupport(ValidationLayers))
	{
		fail("Validation layers requested, but are unavailable.");
	}
#endif

	// Create instance
	VkApplicationInfo ApplicationInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
	ApplicationInfo.pApplicationName = "Vulkan Engine";
	ApplicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	ApplicationInfo.pEngineName = "No Engine";
	ApplicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	ApplicationInfo.apiVersion = VK_API_VERSION_1_0;

	auto Extensions = GetRequiredExtensions();

	VkInstanceCreateInfo InstanceInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	InstanceInfo.pApplicationInfo = &ApplicationInfo;
	InstanceInfo.enabledExtensionCount = static_cast<uint32>(Extensions.size());
	InstanceInfo.ppEnabledExtensionNames = Extensions.data();

#ifdef DEBUG_BUILD
	InstanceInfo.enabledLayerCount = static_cast<uint32>(ValidationLayers.size());
	InstanceInfo.ppEnabledLayerNames = ValidationLayers.data();
#else
	InstanceInfo.enabledLayerCount = 0;
#endif

	vulkan(vkCreateInstance(&InstanceInfo, nullptr, &Instance));

#ifdef DEBUG_BUILD
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

	vulkan(CreateDebugReportCallbackEXT(Instance, &DebugCallbackInfo, nullptr, &DebugReportCallback));
#endif

#ifdef _WIN32
	WindowsPlatformRef Windows = std::static_pointer_cast<WindowsPlatform>(GPlatform);
	vulkan(glfwCreateWindowSurface(Instance, Windows->Window, nullptr, &Surface));
#elif
	signal_unimplemented();
#endif

	// Select Vulkan-capable physical device
	PhysicalDevice = SelectPhysicalDevice(Instance, Surface, DeviceExtensions);

	// Create logical device 
	Device = CreateLogicalDevice(PhysicalDevice, Surface, DeviceExtensions, ValidationLayers, GraphicsQueue, PresentQueue);

	// Create command pool
	CommandPool = CreateCommandPool(Device, PhysicalDevice, Surface);

	// Get physical device properties/features
	vkGetPhysicalDeviceProperties(PhysicalDevice, &Properties);
	vkGetPhysicalDeviceFeatures(PhysicalDevice, &Features);
}

VulkanDevice::~VulkanDevice()
{
	for (const auto&[GraphicsPipelineState, PipelineLayout, DescriptorSetLayout] : PipelineLayoutCache)
	{
		vkDestroyDescriptorSetLayout(Device, DescriptorSetLayout, nullptr);
		vkDestroyPipelineLayout(Device, PipelineLayout, nullptr);
	}

	for (const auto&[PSOInit, Pipeline] : PipelineCache)
	{
		vkDestroyPipeline(Device, Pipeline, nullptr);
	}
}

VkPipeline VulkanDevice::CreatePipeline(
	const PipelineStateInitializer& PSOInit,
	VkPipelineLayout PipelineLayout,
	VkRenderPass RenderPass,
	uint32 NumRenderTargets)
{
	VkPipelineDepthStencilStateCreateInfo DepthStencilState = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };

	{
		static const HashTable<EDepthCompareTest, VkCompareOp> VulkanDepthCompare =
		{
			ENTRY(EDepthCompareTest::Never, VK_COMPARE_OP_NEVER)
			ENTRY(EDepthCompareTest::Less, VK_COMPARE_OP_LESS)
			ENTRY(EDepthCompareTest::Equal, VK_COMPARE_OP_EQUAL)
			ENTRY(EDepthCompareTest::LEqual, VK_COMPARE_OP_LESS_OR_EQUAL)
			ENTRY(EDepthCompareTest::Greater, VK_COMPARE_OP_GREATER)
			ENTRY(EDepthCompareTest::NEqual, VK_COMPARE_OP_NOT_EQUAL)
			ENTRY(EDepthCompareTest::GEqual, VK_COMPARE_OP_GREATER_OR_EQUAL)
			ENTRY(EDepthCompareTest::Always, VK_COMPARE_OP_ALWAYS)
		};

		static const HashTable<ECompareOp, VkCompareOp> VulkanCompareOp =
		{
			ENTRY(ECompareOp::Never, VK_COMPARE_OP_NEVER)
			ENTRY(ECompareOp::Less, VK_COMPARE_OP_LESS)
			ENTRY(ECompareOp::Equal, VK_COMPARE_OP_EQUAL)
			ENTRY(ECompareOp::LessOrEqual, VK_COMPARE_OP_LESS_OR_EQUAL)
			ENTRY(ECompareOp::Greater, VK_COMPARE_OP_GREATER)
			ENTRY(ECompareOp::NotEqual, VK_COMPARE_OP_NOT_EQUAL)
			ENTRY(ECompareOp::GreaterOrEqual, VK_COMPARE_OP_GREATER_OR_EQUAL)
			ENTRY(ECompareOp::Always, VK_COMPARE_OP_ALWAYS)
		};

		static const HashTable<EStencilOp, VkStencilOp> VulkanStencilOp =
		{
			ENTRY(EStencilOp::Keep, VK_STENCIL_OP_KEEP)
			ENTRY(EStencilOp::Replace, VK_STENCIL_OP_REPLACE)
			ENTRY(EStencilOp::Zero, VK_STENCIL_OP_ZERO)
		};

		const auto& In = PSOInit.DepthStencilState;

		DepthStencilState.depthTestEnable = In.DepthTestEnable;
		DepthStencilState.depthWriteEnable = In.DepthWriteEnable;
		DepthStencilState.depthCompareOp = GetValue(VulkanDepthCompare, In.DepthCompareTest);
		DepthStencilState.stencilTestEnable = In.StencilTestEnable;

		const auto& Back = In.Back;

		DepthStencilState.back.failOp = GetValue(VulkanStencilOp, Back.FailOp);
		DepthStencilState.back.passOp = GetValue(VulkanStencilOp, Back.PassOp);
		DepthStencilState.back.depthFailOp = GetValue(VulkanStencilOp, Back.DepthFailOp);
		DepthStencilState.back.compareOp = GetValue(VulkanCompareOp, Back.CompareOp);
		DepthStencilState.back.compareMask = Back.CompareMask;
		DepthStencilState.back.writeMask = Back.WriteMask;
		DepthStencilState.back.reference = Back.Reference;
		DepthStencilState.front = DepthStencilState.back;
	}

	VkPipelineRasterizationStateCreateInfo RasterizationState = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };

	{
		static const HashTable<ECullMode, VkCullModeFlags> VulkanCullMode =
		{
			ENTRY(ECullMode::None, VK_CULL_MODE_NONE)
			ENTRY(ECullMode::Back, VK_CULL_MODE_BACK_BIT)
			ENTRY(ECullMode::Front, VK_CULL_MODE_FRONT_BIT)
			ENTRY(ECullMode::FrontAndBack, VK_CULL_MODE_FRONT_AND_BACK)
		};

		static const HashTable<EFrontFace, VkFrontFace> VulkanFrontFace =
		{
			ENTRY(EFrontFace::CW, VK_FRONT_FACE_CLOCKWISE)
			ENTRY(EFrontFace::CCW, VK_FRONT_FACE_COUNTER_CLOCKWISE)
		};

		static const HashTable<EPolygonMode, VkPolygonMode> VulkanPolygonMode =
		{
			ENTRY(EPolygonMode::Fill, VK_POLYGON_MODE_FILL)
			ENTRY(EPolygonMode::Line, VK_POLYGON_MODE_LINE)
			ENTRY(EPolygonMode::Point, VK_POLYGON_MODE_POINT)
		};

		const auto& In = PSOInit.RasterizationState;

		RasterizationState.depthClampEnable = In.DepthClampEnable;
		RasterizationState.rasterizerDiscardEnable = In.RasterizerDiscardEnable;
		RasterizationState.polygonMode = GetValue(VulkanPolygonMode, In.PolygonMode);
		RasterizationState.cullMode = GetValue(VulkanCullMode, In.CullMode);
		RasterizationState.frontFace = GetValue(VulkanFrontFace, In.FrontFace);
		RasterizationState.depthBiasEnable = In.DepthBiasEnable;
		RasterizationState.depthBiasConstantFactor = In.DepthBiasConstantFactor;
		RasterizationState.depthBiasClamp = In.DepthBiasClamp;
		RasterizationState.depthBiasSlopeFactor = In.DepthBiasSlopeFactor;
		RasterizationState.lineWidth = In.LineWidth;
	}

	VkPipelineMultisampleStateCreateInfo MultisampleState = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };

	{
		const auto& In = PSOInit.MultisampleState;

		MultisampleState.rasterizationSamples = (VkSampleCountFlagBits)In.RasterizationSamples;
		MultisampleState.sampleShadingEnable = In.SampleShadingEnable;
		MultisampleState.minSampleShading = In.MinSampleShading;
		MultisampleState.alphaToCoverageEnable = In.AlphaToCoverageEnable;
		MultisampleState.alphaToOneEnable = In.AlphaToOneEnable;
	}

	std::array<VkPipelineColorBlendAttachmentState, RenderPassInitializer::MaxRenderTargets> ColorBlendAttachmentStates;

	{
		for (uint32 RenderTargetIndex = 0; RenderTargetIndex < NumRenderTargets; RenderTargetIndex++)
		{
			const auto& In = PSOInit.ColorBlendAttachmentStates[RenderTargetIndex];
			auto& Out = ColorBlendAttachmentStates[RenderTargetIndex];

			Out = {};

			Out.blendEnable = In.BlendEnable;

			// @todo-joe Blend factors
			/*Out.srcColorBlendFactor = In.SrcColorBlendFactor;
			Out.dstColorBlendFactor = In.DstColorBlendFactor;
			Out.colorBlendOp = In.ColorBlendOp;
			Out.srcAlphaBlendFactor = In.SrcAlphaBlendFactor;
			Out.dstAlphaBlendFactor = In.DstAlphaBlendFactor;
			Out.alphaBlendOp = In.AlphaBlendOp;*/

			Out.colorWriteMask |= Any(In.ColorWriteMask & EColorChannel::R) ? VK_COLOR_COMPONENT_R_BIT : 0;
			Out.colorWriteMask |= Any(In.ColorWriteMask & EColorChannel::G) ? VK_COLOR_COMPONENT_G_BIT : 0;
			Out.colorWriteMask |= Any(In.ColorWriteMask & EColorChannel::B) ? VK_COLOR_COMPONENT_B_BIT : 0;
			Out.colorWriteMask |= Any(In.ColorWriteMask & EColorChannel::A) ? VK_COLOR_COMPONENT_A_BIT : 0;
		}
	}

	const auto& GraphicsPipeline = PSOInit.GraphicsPipelineState;

	check(GraphicsPipeline.Vertex, "No vertex shader bound...");

	std::vector<drm::ShaderRef> Shaders;

	Shaders.push_back(GraphicsPipeline.Vertex);

	if (GraphicsPipeline.TessControl)
	{
		Shaders.push_back(GraphicsPipeline.TessControl);
	}
	if (GraphicsPipeline.TessEval)
	{
		Shaders.push_back(GraphicsPipeline.TessEval);
	}
	if (GraphicsPipeline.Geometry)
	{
		Shaders.push_back(GraphicsPipeline.Geometry);
	}
	if (GraphicsPipeline.Fragment)
	{
		Shaders.push_back(GraphicsPipeline.Fragment);
	}

	std::vector<VkPipelineShaderStageCreateInfo> ShaderStages(Shaders.size(), { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO });

	for (uint32 i = 0; i < ShaderStages.size(); i++)
	{
		drm::ShaderRef Shader = Shaders[i];
		const VulkanShader& VulkanShader = ShaderCache[Shader->Type];
		VkPipelineShaderStageCreateInfo& ShaderStage = ShaderStages[i];
		ShaderStage.stage = VulkanShader::GetVulkanStage(Shader->Stage);
		ShaderStage.module = VulkanShader.ShaderModule;
		ShaderStage.pName = Shader->Entrypoint.data();
	}

	VkPipelineVertexInputStateCreateInfo VertexInputState = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	const std::vector<VkVertexInputAttributeDescription>& AttributeDescriptions = ShaderCache[GraphicsPipeline.Vertex->Type].Attributes;
	std::vector<VkVertexInputBindingDescription> Bindings(AttributeDescriptions.size());

	for (uint32 i = 0; i < Bindings.size(); i++)
	{
		VkVertexInputBindingDescription& Binding = Bindings[i];
		Binding.binding = AttributeDescriptions[i].binding;
		Binding.stride = GetValue(SizeOfVulkanFormat, AttributeDescriptions[i].format);
		Binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	}

	VertexInputState.pVertexBindingDescriptions = Bindings.data();
	VertexInputState.vertexBindingDescriptionCount = Bindings.size();
	VertexInputState.pVertexAttributeDescriptions = AttributeDescriptions.data();
	VertexInputState.vertexAttributeDescriptionCount = AttributeDescriptions.size();

	VkPipelineInputAssemblyStateCreateInfo InputAssemblyState = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	{
		const auto& In = PSOInit.InputAssemblyState;
		InputAssemblyState.topology = [&] ()
		{
			for (VkPrimitiveTopology VulkanTopology = VK_PRIMITIVE_TOPOLOGY_BEGIN_RANGE; VulkanTopology < VK_PRIMITIVE_TOPOLOGY_RANGE_SIZE;)
			{
				if ((uint32)In.Topology == VulkanTopology)
				{
					return VulkanTopology;
				}
				VulkanTopology = VkPrimitiveTopology(VulkanTopology + 1);
			}
			fail("VkPrimitiveTopology not found.");
		}();
		InputAssemblyState.primitiveRestartEnable = In.PrimitiveRestartEnable;
	}

	VkViewport Viewport = {};
	{
		const auto& In = PSOInit.Viewport;
		Viewport.x = In.X;
		Viewport.y = In.Y;
		Viewport.width = In.Width;
		Viewport.height = In.Height;
		Viewport.minDepth = In.MinDepth;
		Viewport.maxDepth = In.MaxDepth;
	}

	VkRect2D Scissor = {};
	{
		Scissor.extent.width = (uint32)Viewport.width;
		Scissor.extent.height = (uint32)Viewport.height;
		Scissor.offset = { 0, 0 };
	}

	VkPipelineViewportStateCreateInfo ViewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	ViewportState.pViewports = &Viewport;
	ViewportState.viewportCount = 1;
	ViewportState.pScissors = &Scissor;
	ViewportState.scissorCount = 1;

	VkPipelineColorBlendStateCreateInfo ColorBlendState = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	ColorBlendState.blendConstants[0] = 0.0f;
	ColorBlendState.blendConstants[1] = 0.0f;
	ColorBlendState.blendConstants[2] = 0.0f;
	ColorBlendState.blendConstants[3] = 0.0f;
	ColorBlendState.logicOp = VK_LOGIC_OP_COPY;
	ColorBlendState.logicOpEnable = false;
	ColorBlendState.pAttachments = ColorBlendAttachmentStates.data();
	ColorBlendState.attachmentCount = NumRenderTargets;

	VkGraphicsPipelineCreateInfo PipelineInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	PipelineInfo.stageCount = ShaderStages.size();
	PipelineInfo.pStages = ShaderStages.data();
	PipelineInfo.pVertexInputState = &VertexInputState;
	PipelineInfo.pInputAssemblyState = &InputAssemblyState;
	PipelineInfo.pViewportState = &ViewportState;
	PipelineInfo.pRasterizationState = &RasterizationState;
	PipelineInfo.pMultisampleState = &MultisampleState;
	PipelineInfo.pDepthStencilState = &DepthStencilState;
	PipelineInfo.pColorBlendState = &ColorBlendState;
	PipelineInfo.layout = PipelineLayout;
	PipelineInfo.renderPass = RenderPass;
	PipelineInfo.subpass = 0;
	PipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	VkPipeline Pipeline;
	vulkan(vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &Pipeline));

	return Pipeline;
}

std::tuple<VkPipeline, VkPipelineLayout, VkDescriptorSetLayout> VulkanDevice::GetPipeline(const PipelineStateInitializer& PSOInit, VkRenderPass RenderPass, uint32 NumRenderTargets)
{
	// Find or create the layouts for the bound shaders. (@todo Do this at shader load time?)

	auto [PipelineLayout, DescriptorSetLayout] = GetPipelineLayout(PSOInit.GraphicsPipelineState);

	// Find or create the pipeline.

	VkPipeline Pipeline = VK_NULL_HANDLE;

	for (const auto& [CachedPSO, CachedPipeline] : PipelineCache)
	{
		if (PSOInit == CachedPSO)
		{
			Pipeline = CachedPipeline;
			break;
		}
	}

	if (Pipeline == VK_NULL_HANDLE)
	{
		Pipeline = CreatePipeline(PSOInit, PipelineLayout, RenderPass, NumRenderTargets);
		PipelineCache.push_back({ PSOInit, Pipeline });
	}

	return { Pipeline, PipelineLayout, DescriptorSetLayout };
}

std::pair<VkPipelineLayout, VkDescriptorSetLayout> VulkanDevice::GetPipelineLayout(const GraphicsPipelineState& GraphicsPipelineState)
{
	VkPipelineLayout PipelineLayout = VK_NULL_HANDLE;
	VkDescriptorSetLayout DescriptorSetLayout = VK_NULL_HANDLE;

	for (const auto&[CachedGraphicsPipelineState, CachedPipelineLayout, CachedDescriptorSetLayout] : PipelineLayoutCache)
	{
		if (GraphicsPipelineState == CachedGraphicsPipelineState)
		{
			PipelineLayout = CachedPipelineLayout;
			DescriptorSetLayout = CachedDescriptorSetLayout;
			break;
		}
	}

	if (PipelineLayout == VK_NULL_HANDLE)
	{
		std::vector<VkDescriptorSetLayoutBinding> AllBindings;

		auto AddBindings = [&] (const drm::ShaderRef Shader)
		{
			if (Shader)
			{
				const auto& VulkanShader = ShaderCache[Shader->Type];
				const auto& Bindings = VulkanShader.Bindings;
				if (Bindings.size() > 0)
				{
					AllBindings.insert(AllBindings.end(), Bindings.begin(), Bindings.end());
				}
			}
		};

		{
			AddBindings(GraphicsPipelineState.Vertex);
			AddBindings(GraphicsPipelineState.TessControl);
			AddBindings(GraphicsPipelineState.TessEval);
			AddBindings(GraphicsPipelineState.Geometry);
			AddBindings(GraphicsPipelineState.Fragment);
		}

		VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		DescriptorSetLayoutInfo.bindingCount = static_cast<uint32>(AllBindings.size());
		DescriptorSetLayoutInfo.pBindings = AllBindings.data();

		vulkan(vkCreateDescriptorSetLayout(Device, &DescriptorSetLayoutInfo, nullptr, &DescriptorSetLayout));

		VkPipelineLayoutCreateInfo PipelineLayoutInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
		PipelineLayoutInfo.setLayoutCount = 1;
		PipelineLayoutInfo.pSetLayouts = &DescriptorSetLayout;

		vulkan(vkCreatePipelineLayout(Device, &PipelineLayoutInfo, nullptr, &PipelineLayout));

		PipelineLayoutCache.push_back({ GraphicsPipelineState, PipelineLayout, DescriptorSetLayout });
	}

	return { PipelineLayout, DescriptorSetLayout };
}

inline bool QueueFamilyIndices::IsComplete() const
{
	return GraphicsFamily >= 0 && PresentFamily >= 0;
}

inline void QueueFamilyIndices::FindQueueFamilies(VkPhysicalDevice Device, VkSurfaceKHR Surface)
{
	QueueFamilyIndices Indices;

	uint32 QueueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(Device, &QueueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> QueueFamilies(QueueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(Device, &QueueFamilyCount, QueueFamilies.data());

	for (int i = 0; i < static_cast<int>(QueueFamilies.size()); i++)
	{
		if (QueueFamilies[i].queueCount > 0 && QueueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			GraphicsFamily = i;
		}

		VkBool32 PresentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(Device, i, Surface, &PresentSupport);

		if (QueueFamilies[i].queueCount > 0 && PresentSupport)
		{
			PresentFamily = i;
		}

		if (Indices.IsComplete())
		{
			break;
		}
	}
}