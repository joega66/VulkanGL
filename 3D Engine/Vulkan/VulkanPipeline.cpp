#include "VulkanDevice.h"
#include "VulkanPipeline.h"
#include <GPU/GPUShader.h>

static void CreateDepthStencilState(const GraphicsPipelineDesc& graphicsDesc, VkPipelineDepthStencilStateCreateInfo& depthStencilState)
{
	static const VkStencilOp vulkanStencilOp[] =
	{
		VK_STENCIL_OP_KEEP,
		VK_STENCIL_OP_ZERO,
		VK_STENCIL_OP_REPLACE,
	};

	static const VkCompareOp vulkanCompareOp[] =
	{
		VK_COMPARE_OP_NEVER,
		VK_COMPARE_OP_LESS,
		VK_COMPARE_OP_EQUAL,
		VK_COMPARE_OP_LESS_OR_EQUAL,
		VK_COMPARE_OP_GREATER,
		VK_COMPARE_OP_NOT_EQUAL,
		VK_COMPARE_OP_GREATER_OR_EQUAL,
		VK_COMPARE_OP_ALWAYS,
	};

	depthStencilState.depthTestEnable	= graphicsDesc.depthStencilState.depthTestEnable;
	depthStencilState.depthWriteEnable	= graphicsDesc.depthStencilState.depthWriteEnable;
	depthStencilState.depthCompareOp	= vulkanCompareOp[static_cast<uint32>(graphicsDesc.depthStencilState.depthCompareTest)];
	depthStencilState.stencilTestEnable = graphicsDesc.depthStencilState.stencilTestEnable;

	depthStencilState.back.failOp		= vulkanStencilOp[static_cast<uint32>(graphicsDesc.depthStencilState.back.failOp)];
	depthStencilState.back.passOp		= vulkanStencilOp[static_cast<uint32>(graphicsDesc.depthStencilState.back.passOp)];
	depthStencilState.back.depthFailOp	= vulkanStencilOp[static_cast<uint32>(graphicsDesc.depthStencilState.back.depthFailOp)];
	depthStencilState.back.compareOp	= vulkanCompareOp[static_cast<uint32>(graphicsDesc.depthStencilState.back.compareOp)];
	depthStencilState.back.compareMask	= graphicsDesc.depthStencilState.back.compareMask;
	depthStencilState.back.writeMask	= graphicsDesc.depthStencilState.back.writeMask;
	depthStencilState.back.reference	= graphicsDesc.depthStencilState.back.reference;
	depthStencilState.front				= depthStencilState.back;
}

static void CreateRasterizationState(const GraphicsPipelineDesc& graphicsDesc, VkPipelineRasterizationStateCreateInfo& rasterizationState)
{
	static const VkFrontFace vulkanFrontFace[] =
	{
		VK_FRONT_FACE_COUNTER_CLOCKWISE,
		VK_FRONT_FACE_CLOCKWISE,
	};

	static const VkPolygonMode vulkanPolygonMode[] =
	{
		VK_POLYGON_MODE_FILL,
		VK_POLYGON_MODE_LINE,
		VK_POLYGON_MODE_POINT,
	};

	rasterizationState.depthClampEnable			= graphicsDesc.rasterizationState.depthClampEnable;
	rasterizationState.rasterizerDiscardEnable	= graphicsDesc.rasterizationState.rasterizerDiscardEnable;
	rasterizationState.polygonMode				= vulkanPolygonMode[static_cast<uint32>(graphicsDesc.rasterizationState.polygonMode)];
	rasterizationState.cullMode					= static_cast<VkCullModeFlags>(graphicsDesc.rasterizationState.cullMode);
	rasterizationState.frontFace				= vulkanFrontFace[static_cast<uint32>(graphicsDesc.rasterizationState.frontFace)];
	rasterizationState.depthBiasEnable			= graphicsDesc.rasterizationState.depthBiasEnable;
	rasterizationState.depthBiasConstantFactor	= graphicsDesc.rasterizationState.depthBiasConstantFactor;
	rasterizationState.depthBiasClamp			= graphicsDesc.rasterizationState.depthBiasClamp;
	rasterizationState.depthBiasSlopeFactor		= graphicsDesc.rasterizationState.depthBiasSlopeFactor;
	rasterizationState.lineWidth				= graphicsDesc.rasterizationState.lineWidth;
}

static void CreateMultisampleState(const GraphicsPipelineDesc& graphicsDesc, VkPipelineMultisampleStateCreateInfo& multisampleState)
{
	multisampleState.rasterizationSamples	= static_cast<VkSampleCountFlagBits>(graphicsDesc.multisampleState.rasterizationSamples);
	multisampleState.sampleShadingEnable	= graphicsDesc.multisampleState.sampleShadingEnable;
	multisampleState.minSampleShading		= graphicsDesc.multisampleState.minSampleShading;
	multisampleState.alphaToCoverageEnable	= graphicsDesc.multisampleState.alphaToCoverageEnable;
	multisampleState.alphaToOneEnable		= graphicsDesc.multisampleState.alphaToOneEnable;
}

static void CreateColorBlendState(
	const GraphicsPipelineDesc& graphicsDesc,
	VkPipelineColorBlendStateCreateInfo& colorBlendState,
	std::vector<VkPipelineColorBlendAttachmentState>& colorBlendAttachmentStates)
{
	static auto convertColorBlendAttachmentStates = [] (
		const std::vector<ColorBlendAttachmentState>& inColorBlendAttachmentStates,
		std::vector<VkPipelineColorBlendAttachmentState>& outColorBlendAttachmentStates
	)
	{
		static const VkBlendOp vulkanBlendOp[] =
		{
			VK_BLEND_OP_ADD,
			VK_BLEND_OP_SUBTRACT,
			VK_BLEND_OP_REVERSE_SUBTRACT,
			VK_BLEND_OP_MIN,
			VK_BLEND_OP_MAX,
		};

		static const VkBlendFactor vulkanBlendFactor[] =
		{
			VK_BLEND_FACTOR_ZERO,
			VK_BLEND_FACTOR_ONE,
			VK_BLEND_FACTOR_SRC_COLOR,
			VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
			VK_BLEND_FACTOR_DST_COLOR,
			VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
			VK_BLEND_FACTOR_SRC_ALPHA,
			VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			VK_BLEND_FACTOR_DST_ALPHA,
			VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
			VK_BLEND_FACTOR_CONSTANT_COLOR,
			VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR,
			VK_BLEND_FACTOR_CONSTANT_ALPHA,
			VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA,
			VK_BLEND_FACTOR_SRC_ALPHA_SATURATE,
			VK_BLEND_FACTOR_SRC1_COLOR,
			VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR,
			VK_BLEND_FACTOR_SRC1_ALPHA,
			VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA,
		};

		for (const auto& in : inColorBlendAttachmentStates)
		{
			outColorBlendAttachmentStates.push_back({
				.blendEnable			= in.blendEnable,
				.srcColorBlendFactor	= vulkanBlendFactor[static_cast<uint32>(in.srcColorBlendFactor)],
				.dstColorBlendFactor	= vulkanBlendFactor[static_cast<uint32>(in.dstColorBlendFactor)],
				.colorBlendOp			= vulkanBlendOp[static_cast<uint32>(in.colorBlendOp)],
				.srcAlphaBlendFactor	= vulkanBlendFactor[static_cast<uint32>(in.srcAlphaBlendFactor)],
				.dstAlphaBlendFactor	= vulkanBlendFactor[static_cast<uint32>(in.dstAlphaBlendFactor)],
				.alphaBlendOp			= vulkanBlendOp[static_cast<uint32>(in.alphaBlendOp)],
				.colorWriteMask			= (VkColorComponentFlags)
					(Any(in.colorWriteMask & EColorChannel::R) ? VK_COLOR_COMPONENT_R_BIT : 0) |
					(Any(in.colorWriteMask & EColorChannel::G) ? VK_COLOR_COMPONENT_G_BIT : 0) |
					(Any(in.colorWriteMask & EColorChannel::B) ? VK_COLOR_COMPONENT_B_BIT : 0) |
					(Any(in.colorWriteMask & EColorChannel::A) ? VK_COLOR_COMPONENT_A_BIT : 0)
			});
		}
	};

	colorBlendAttachmentStates.reserve(graphicsDesc.renderPass.GetNumAttachments());

	convertColorBlendAttachmentStates(
		graphicsDesc.colorBlendAttachmentStates.empty() ? 
		std::vector(graphicsDesc.renderPass.GetNumAttachments(), ColorBlendAttachmentState{}) :
		graphicsDesc.colorBlendAttachmentStates,
		colorBlendAttachmentStates);
	
	colorBlendState.blendConstants[0]	= 0.0f;
	colorBlendState.blendConstants[1]	= 0.0f;
	colorBlendState.blendConstants[2]	= 0.0f;
	colorBlendState.blendConstants[3]	= 0.0f;
	colorBlendState.logicOp				= VK_LOGIC_OP_COPY;
	colorBlendState.logicOpEnable		= false;
	colorBlendState.pAttachments		= colorBlendAttachmentStates.data();
	colorBlendState.attachmentCount		= graphicsDesc.renderPass.GetNumAttachments();
}

static void CreateShaderStages(const GraphicsPipelineDesc& graphicsDesc, std::vector<VkPipelineShaderStageCreateInfo>& shaderStages)
{
	const auto addShaderStage = [&] (const gpu::Shader* shader)
	{
		if (shader == nullptr)
		{
			return;
		}

		shaderStages.push_back({
			.sType	= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage	= static_cast<VkShaderStageFlagBits>(shader->compilationResult.stage),
			.module = shader->compilationResult.shaderModule,
			.pName	= shader->compilationResult.entrypoint.data(),
		});
	};

	addShaderStage(graphicsDesc.shaderStages.vertex);
	addShaderStage(graphicsDesc.shaderStages.tessControl);
	addShaderStage(graphicsDesc.shaderStages.tessEval);
	addShaderStage(graphicsDesc.shaderStages.geometry);
	addShaderStage(graphicsDesc.shaderStages.fragment);
}

static void CreateSpecializationInfo(
	const GraphicsPipelineDesc& graphicsDesc, 
	VkSpecializationInfo& specializationInfo, 
	std::vector<VkPipelineShaderStageCreateInfo>& shaderStages)
{
	const std::vector<SpecializationInfo::SpecializationMapEntry>& mapEntries = graphicsDesc.specInfo.GetMapEntries();

	if (mapEntries.size() > 0)
	{
		static_assert(sizeof(VkSpecializationMapEntry) == sizeof(SpecializationInfo::SpecializationMapEntry));

		const std::vector<uint8>& data = graphicsDesc.specInfo.GetData();
		specializationInfo.mapEntryCount	= static_cast<uint32>(mapEntries.size());
		specializationInfo.pMapEntries		= reinterpret_cast<const VkSpecializationMapEntry*>(mapEntries.data());
		specializationInfo.dataSize			= data.size();
		specializationInfo.pData			= data.data();

		std::for_each(shaderStages.begin(), shaderStages.end(), [&] (VkPipelineShaderStageCreateInfo& shaderStageInfo)
		{
			shaderStageInfo.pSpecializationInfo = &specializationInfo;
		});
	}
}

static void CreateVertexInputState(
	const GraphicsPipelineDesc& graphicsDesc,
	VkPipelineVertexInputStateCreateInfo& outVertexInputState,
	std::vector<VkVertexInputAttributeDescription>& outVertexAttributes,
	std::vector<VkVertexInputBindingDescription>& outVertexBindings)
{
	// If no vertex attributes were provided in the PSO desc, use the ones from shader reflection.
	const std::vector<VertexAttributeDescription>& vertexAttributes =
		graphicsDesc.vertexAttributes.empty() ? 
		graphicsDesc.shaderStages.vertex->compilationResult.vertexAttributeDescriptions : 
		graphicsDesc.vertexAttributes;

	outVertexAttributes.reserve(vertexAttributes.size());

	for (const auto& vertexAttribute : vertexAttributes)
	{
		outVertexAttributes.push_back({
			.location	= static_cast<uint32>(vertexAttribute.location),
			.binding	= vertexAttribute.binding,
			.format		= gpu::Image::GetVulkanFormat(vertexAttribute.format),
			.offset		= vertexAttribute.offset
		});
	}

	outVertexBindings.reserve(vertexAttributes.size());

	if (graphicsDesc.vertexBindings.empty())
	{
		for (const auto& vertexAttribute : vertexAttributes)
		{
			outVertexBindings.push_back({
				.binding = vertexAttribute.binding,
				.stride = gpu::Image::GetSize(vertexAttribute.format),
				.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
			});
		}
	}
	else
	{
		for (const auto& vertexBinding : graphicsDesc.vertexBindings)
		{
			outVertexBindings.push_back({
				.binding = vertexBinding.binding,
				.stride = vertexBinding.stride,
				.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
			});
		}
	}

	outVertexInputState.vertexBindingDescriptionCount	= static_cast<uint32>(outVertexBindings.size());
	outVertexInputState.pVertexBindingDescriptions		= outVertexBindings.data();
	outVertexInputState.vertexAttributeDescriptionCount = static_cast<uint32>(outVertexAttributes.size());
	outVertexInputState.pVertexAttributeDescriptions	= outVertexAttributes.data();
}

static void CreateInputAssemblyState(const GraphicsPipelineDesc& graphicsDesc, VkPipelineInputAssemblyStateCreateInfo& inputAssemblyState)
{
	inputAssemblyState.topology					= static_cast<VkPrimitiveTopology>(graphicsDesc.inputAssemblyState.topology);
	inputAssemblyState.primitiveRestartEnable	= graphicsDesc.inputAssemblyState.primitiveRestartEnable;
}

static void CreateViewportState(const GraphicsPipelineDesc& graphicsDesc, VkPipelineViewportStateCreateInfo& viewportState)
{
	viewportState.pViewports	= nullptr;
	viewportState.viewportCount = 1;
	viewportState.pScissors		= nullptr;
	viewportState.scissorCount	= 1;
}

VkPipeline VulkanDevice::CreatePipeline(const GraphicsPipelineDesc& graphicsDesc, VkPipelineLayout pipelineLayout) const
{
	VkPipelineDepthStencilStateCreateInfo depthStencilState = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	CreateDepthStencilState(graphicsDesc, depthStencilState);

	VkPipelineRasterizationStateCreateInfo rasterizationState = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	CreateRasterizationState(graphicsDesc, rasterizationState);

	VkPipelineMultisampleStateCreateInfo multisampleState = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	CreateMultisampleState(graphicsDesc, multisampleState);

	std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachmentStates;
	VkPipelineColorBlendStateCreateInfo colorBlendState = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	CreateColorBlendState(graphicsDesc, colorBlendState, colorBlendAttachmentStates);

	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	CreateShaderStages(graphicsDesc, shaderStages);

	VkSpecializationInfo specializationInfo;
	CreateSpecializationInfo(graphicsDesc, specializationInfo, shaderStages);

	std::vector<VkVertexInputAttributeDescription> vertexAttributes;
	std::vector<VkVertexInputBindingDescription> vertexBindings;
	VkPipelineVertexInputStateCreateInfo vertexInputState = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	CreateVertexInputState(graphicsDesc, vertexInputState, vertexAttributes, vertexBindings);
	
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	CreateInputAssemblyState(graphicsDesc, inputAssemblyState);

	VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	CreateViewportState(graphicsDesc, viewportState);

	const VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	const VkPipelineDynamicStateCreateInfo dynamicState =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = static_cast<uint32>(std::size(dynamicStates)),
		.pDynamicStates = dynamicStates,
	};

	const VkGraphicsPipelineCreateInfo pipelineInfo = 
	{ 
		.sType					= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount				= static_cast<uint32>(shaderStages.size()),
		.pStages				= shaderStages.data(),
		.pVertexInputState		= &vertexInputState,
		.pInputAssemblyState	= &inputAssemblyState,
		.pViewportState			= &viewportState,
		.pRasterizationState	= &rasterizationState,
		.pMultisampleState		= &multisampleState,
		.pDepthStencilState		= &depthStencilState,
		.pColorBlendState		= &colorBlendState,
		.pDynamicState			= &dynamicState,
		.layout					= pipelineLayout,
		.renderPass				= graphicsDesc.renderPass.GetRenderPass(),
		.subpass				= 0,
		.basePipelineHandle		= VK_NULL_HANDLE,
		.basePipelineIndex		= -1,
	};

	VkPipeline pipeline;
	vulkan(vkCreateGraphicsPipelines(_Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline));
	
	return pipeline;
}

VkPipeline VulkanDevice::CreatePipeline(const ComputePipelineDesc& computeDesc, VkPipelineLayout pipelineLayout) const
{
	VkComputePipelineCreateInfo pipelineInfo = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
	pipelineInfo.layout				= pipelineLayout;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	VkPipelineShaderStageCreateInfo& stage = pipelineInfo.stage;
	stage			= { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	stage.stage		= VK_SHADER_STAGE_COMPUTE_BIT;
	stage.module	= computeDesc.shader->compilationResult.shaderModule;
	stage.pName		= computeDesc.shader->compilationResult.entrypoint.data();

	VkSpecializationInfo specializationInfo;

	if (computeDesc.specInfo.GetMapEntries().size() > 0)
	{
		static_assert(sizeof(VkSpecializationMapEntry) == sizeof(SpecializationInfo::SpecializationMapEntry));

		const std::vector<uint8>& data = computeDesc.specInfo.GetData();
		specializationInfo.mapEntryCount	= static_cast<uint32>(computeDesc.specInfo.GetMapEntries().size());
		specializationInfo.pMapEntries		= reinterpret_cast<const VkSpecializationMapEntry*>(computeDesc.specInfo.GetMapEntries().data());
		specializationInfo.dataSize			= data.size();
		specializationInfo.pData			= data.data();

		stage.pSpecializationInfo = &specializationInfo;
	}

	VkPipeline pipeline;
	vulkan(vkCreateComputePipelines(_Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline));

	return pipeline;
}

VkPipelineLayout VulkanDevice::GetOrCreatePipelineLayout(
	const std::vector<VkDescriptorSetLayout>& layouts,
	const std::vector<VkPushConstantRange>& pushConstantRanges)
{
	Crc crc = 0;
	Platform::crc32_u32(crc, layouts.data(), layouts.size() * sizeof(layouts[0]));
	Platform::crc32_u32(crc, pushConstantRanges.data(), pushConstantRanges.size() * sizeof(pushConstantRanges[0]));

	if (auto iter = _PipelineLayoutCache.find(crc); iter == _PipelineLayoutCache.end())
	{
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
		pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32>(layouts.size());
		pipelineLayoutCreateInfo.pSetLayouts = layouts.data();

		if (pushConstantRanges.size() > 0)
		{
			pipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<uint32>(pushConstantRanges.size());
			pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges.data();
		}

		VkPipelineLayout pipelineLayout;
		vulkan(vkCreatePipelineLayout(_Device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

		_PipelineLayoutCache[crc] = pipelineLayout;

		return pipelineLayout;
	}
	else
	{
		return iter->second;
	}
}

VulkanPipeline::VulkanPipeline(
	VulkanDevice& device,
	VkPipeline pipeline, 
	VkPipelineLayout pipelineLayout, 
	VkPipelineBindPoint pipelineBindPoint)
	: _Device(device)
	, _Pipeline(pipeline)
	, _PipelineLayout(pipelineLayout)
	, _PipelineBindPoint(pipelineBindPoint)
{
}

VulkanPipeline::~VulkanPipeline()
{
	_Device.Destroy(_Pipeline);
}