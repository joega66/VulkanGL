#include "VulkanCache.h"
#include "VulkanDevice.h"
#include "VulkanPipeline.h"
#include <GPU/GPUShader.h>

gpu::Pipeline VulkanCache::GetPipeline(const PipelineStateDesc& psoDesc)
{
	if (auto iter = GraphicsPipelineCache.find(psoDesc); iter != GraphicsPipelineCache.end())
	{
		return iter->second;
	}

	std::map<uint32, VkDescriptorSetLayout> layoutsMap;
	
	auto getLayouts = [&] (const gpu::Shader* shader)
	{
		if (shader)
		{
			for (const auto& [set, layout] : shader->compilationInfo.layouts)
			{
				layoutsMap.insert({ set, layout });
			}
		}
	};

	getLayouts(psoDesc.shaderStages.vertex);
	getLayouts(psoDesc.shaderStages.tessControl);
	getLayouts(psoDesc.shaderStages.tessEval);
	getLayouts(psoDesc.shaderStages.geometry);
	getLayouts(psoDesc.shaderStages.fragment);

	std::vector<VkDescriptorSetLayout> layouts;
	layouts.reserve(layoutsMap.size());

	for (const auto& [set, layout] : layoutsMap)
	{
		layouts.push_back(layout);
	}

	std::vector<VkPushConstantRange> pushConstantRanges;

	auto getPushConstantRange = [&] (const gpu::Shader* shader)
	{
		if (shader && shader->compilationInfo.pushConstantRange.size > 0)
		{
			pushConstantRanges.push_back(shader->compilationInfo.pushConstantRange);
		}
	};

	getPushConstantRange(psoDesc.shaderStages.vertex);
	getPushConstantRange(psoDesc.shaderStages.tessControl);
	getPushConstantRange(psoDesc.shaderStages.tessEval);
	getPushConstantRange(psoDesc.shaderStages.geometry);
	getPushConstantRange(psoDesc.shaderStages.fragment);

	const VkPipelineLayout pipelineLayout = GetPipelineLayout(layouts, pushConstantRanges);
	auto pipeline = std::make_shared<VulkanPipeline>(Device, CreatePipeline(psoDesc, pipelineLayout), pipelineLayout, VK_PIPELINE_BIND_POINT_GRAPHICS);
	GraphicsPipelineCache[psoDesc] = pipeline;
	return pipeline;
}

gpu::Pipeline VulkanCache::GetPipeline(const ComputePipelineDesc& computeDesc)
{
	auto& mapEntries = computeDesc.specInfo.GetMapEntries();
	auto& data = computeDesc.specInfo.GetData();

	struct ComputePipelineHash
	{
		Crc computeShaderCrc;
		Crc mapEntriesCrc;
		Crc mapDataCrc;
	};

	std::vector<VkDescriptorSetLayout> layouts;
	layouts.reserve(computeDesc.computeShader->compilationInfo.layouts.size());

	for (auto& [set, layout] : computeDesc.computeShader->compilationInfo.layouts)
	{
		layouts.push_back(layout);
	}

	const ComputePipelineHash computeHash =
	{
		.computeShaderCrc = Platform::CalculateCrc(computeDesc.computeShader, sizeof(computeDesc.computeShader)),
		.mapEntriesCrc = Platform::CalculateCrc(mapEntries.data(), mapEntries.size() * sizeof(SpecializationInfo::SpecializationMapEntry)),
		.mapDataCrc = Platform::CalculateCrc(data.data(), data.size()),
	};
	
	const Crc crc = Platform::CalculateCrc(&computeHash, sizeof(computeHash));

	if (auto iter = ComputePipelineCache.find(crc); iter != ComputePipelineCache.end())
	{
		return iter->second;
	}

	const auto& pushConstantRange = computeDesc.computeShader->compilationInfo.pushConstantRange;

	const auto pushConstantRanges = pushConstantRange.size > 0 ? std::vector{ pushConstantRange } : std::vector<VkPushConstantRange>{};

	const VkPipelineLayout pipelineLayout = GetPipelineLayout(layouts, pushConstantRanges);

	auto pipeline = std::make_shared<VulkanPipeline>(Device, CreatePipeline(computeDesc, pipelineLayout), pipelineLayout, VK_PIPELINE_BIND_POINT_COMPUTE);

	ComputePipelineCache[crc] = pipeline;

	CrcToComputeDesc[crc] = computeDesc;

	return pipeline;
}

static void CreateDepthStencilState(const PipelineStateDesc& psoDesc, VkPipelineDepthStencilStateCreateInfo& depthStencilState)
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

	depthStencilState.depthTestEnable	= psoDesc.depthStencilState.depthTestEnable;
	depthStencilState.depthWriteEnable	= psoDesc.depthStencilState.depthWriteEnable;
	depthStencilState.depthCompareOp	= vulkanCompareOp[static_cast<uint32>(psoDesc.depthStencilState.depthCompareTest)];
	depthStencilState.stencilTestEnable = psoDesc.depthStencilState.stencilTestEnable;

	depthStencilState.back.failOp		= vulkanStencilOp[static_cast<uint32>(psoDesc.depthStencilState.back.failOp)];
	depthStencilState.back.passOp		= vulkanStencilOp[static_cast<uint32>(psoDesc.depthStencilState.back.passOp)];
	depthStencilState.back.depthFailOp	= vulkanStencilOp[static_cast<uint32>(psoDesc.depthStencilState.back.depthFailOp)];
	depthStencilState.back.compareOp	= vulkanCompareOp[static_cast<uint32>(psoDesc.depthStencilState.back.compareOp)];
	depthStencilState.back.compareMask	= psoDesc.depthStencilState.back.compareMask;
	depthStencilState.back.writeMask	= psoDesc.depthStencilState.back.writeMask;
	depthStencilState.back.reference	= psoDesc.depthStencilState.back.reference;
	depthStencilState.front				= depthStencilState.back;
}

static void CreateRasterizationState(const PipelineStateDesc& psoDesc, VkPipelineRasterizationStateCreateInfo& rasterizationState)
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

	rasterizationState.depthClampEnable			= psoDesc.rasterizationState.depthClampEnable;
	rasterizationState.rasterizerDiscardEnable	= psoDesc.rasterizationState.rasterizerDiscardEnable;
	rasterizationState.polygonMode				= vulkanPolygonMode[static_cast<uint32>(psoDesc.rasterizationState.polygonMode)];
	rasterizationState.cullMode					= static_cast<VkCullModeFlags>(psoDesc.rasterizationState.cullMode);
	rasterizationState.frontFace				= vulkanFrontFace[static_cast<uint32>(psoDesc.rasterizationState.frontFace)];
	rasterizationState.depthBiasEnable			= psoDesc.rasterizationState.depthBiasEnable;
	rasterizationState.depthBiasConstantFactor	= psoDesc.rasterizationState.depthBiasConstantFactor;
	rasterizationState.depthBiasClamp			= psoDesc.rasterizationState.depthBiasClamp;
	rasterizationState.depthBiasSlopeFactor		= psoDesc.rasterizationState.depthBiasSlopeFactor;
	rasterizationState.lineWidth				= psoDesc.rasterizationState.lineWidth;
}

static void CreateMultisampleState(const PipelineStateDesc& psoDesc, VkPipelineMultisampleStateCreateInfo& multisampleState)
{
	multisampleState.rasterizationSamples	= static_cast<VkSampleCountFlagBits>(psoDesc.multisampleState.rasterizationSamples);
	multisampleState.sampleShadingEnable	= psoDesc.multisampleState.sampleShadingEnable;
	multisampleState.minSampleShading		= psoDesc.multisampleState.minSampleShading;
	multisampleState.alphaToCoverageEnable	= psoDesc.multisampleState.alphaToCoverageEnable;
	multisampleState.alphaToOneEnable		= psoDesc.multisampleState.alphaToOneEnable;
}

static void CreateColorBlendState(
	const PipelineStateDesc& psoDesc,
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

	colorBlendAttachmentStates.reserve(psoDesc.renderPass.GetNumAttachments());

	convertColorBlendAttachmentStates(
		psoDesc.colorBlendAttachmentStates.empty() ? 
		std::vector(psoDesc.renderPass.GetNumAttachments(), ColorBlendAttachmentState{}) :
		psoDesc.colorBlendAttachmentStates,
		colorBlendAttachmentStates);
	
	colorBlendState.blendConstants[0]	= 0.0f;
	colorBlendState.blendConstants[1]	= 0.0f;
	colorBlendState.blendConstants[2]	= 0.0f;
	colorBlendState.blendConstants[3]	= 0.0f;
	colorBlendState.logicOp				= VK_LOGIC_OP_COPY;
	colorBlendState.logicOpEnable		= false;
	colorBlendState.pAttachments		= colorBlendAttachmentStates.data();
	colorBlendState.attachmentCount		= psoDesc.renderPass.GetNumAttachments();
}

static void CreateShaderStages(const PipelineStateDesc& psoDesc, std::vector<VkPipelineShaderStageCreateInfo>& shaderStages)
{
	const auto addShaderStage = [&] (const gpu::Shader* shader)
	{
		if (shader == nullptr)
		{
			return;
		}

		shaderStages.push_back({
			.sType	= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage	= static_cast<VkShaderStageFlagBits>(shader->compilationInfo.stage),
			.module = shader->compilationInfo.shaderModule,
			.pName	= shader->compilationInfo.entrypoint.data(),
		});
	};

	addShaderStage(psoDesc.shaderStages.vertex);
	addShaderStage(psoDesc.shaderStages.tessControl);
	addShaderStage(psoDesc.shaderStages.tessEval);
	addShaderStage(psoDesc.shaderStages.geometry);
	addShaderStage(psoDesc.shaderStages.fragment);
}

static void CreateSpecializationInfo(
	const PipelineStateDesc& psoDesc, 
	VkSpecializationInfo& specializationInfo, 
	std::vector<VkPipelineShaderStageCreateInfo>& shaderStages)
{
	const std::vector<SpecializationInfo::SpecializationMapEntry>& mapEntries = psoDesc.specInfo.GetMapEntries();

	if (mapEntries.size() > 0)
	{
		static_assert(sizeof(VkSpecializationMapEntry) == sizeof(SpecializationInfo::SpecializationMapEntry));

		const std::vector<uint8>& data = psoDesc.specInfo.GetData();
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
	const PipelineStateDesc& psoDesc,
	VkPipelineVertexInputStateCreateInfo& outVertexInputState,
	std::vector<VkVertexInputAttributeDescription>& outVertexAttributes,
	std::vector<VkVertexInputBindingDescription>& outVertexBindings)
{
	// If no vertex attributes were provided in the PSO desc, use the ones from shader reflection.
	const std::vector<VertexAttributeDescription>& vertexAttributes =
		psoDesc.vertexAttributes.empty() ? 
		psoDesc.shaderStages.vertex->compilationInfo.vertexAttributeDescriptions : 
		psoDesc.vertexAttributes;

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

	if (psoDesc.vertexBindings.empty())
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
		for (const auto& vertexBinding : psoDesc.vertexBindings)
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

static void CreateInputAssemblyState(const PipelineStateDesc& psoDesc, VkPipelineInputAssemblyStateCreateInfo& inputAssemblyState)
{
	inputAssemblyState.topology					= static_cast<VkPrimitiveTopology>(psoDesc.inputAssemblyState.topology);
	inputAssemblyState.primitiveRestartEnable	= psoDesc.inputAssemblyState.primitiveRestartEnable;
}

static void CreateViewportState(const PipelineStateDesc& psoDesc, VkViewport& viewport, VkRect2D& scissor, VkPipelineViewportStateCreateInfo& viewportState)
{
	viewport.x			= static_cast<float>(psoDesc.viewport.x);
	viewport.y			= static_cast<float>(psoDesc.viewport.y);
	viewport.width		= static_cast<float>(psoDesc.viewport.width);
	viewport.height		= static_cast<float>(psoDesc.viewport.height);
	viewport.minDepth	= psoDesc.viewport.minDepth;
	viewport.maxDepth	= psoDesc.viewport.maxDepth;

	if (psoDesc.scissor == Scissor{})
	{
		scissor.extent.width	= static_cast<uint32>(viewport.width);
		scissor.extent.height	= static_cast<uint32>(viewport.height);
		scissor.offset			= { 0, 0 };
	}
	else
	{
		scissor.extent.width	= psoDesc.scissor.extent.x;
		scissor.extent.height	= psoDesc.scissor.extent.y;
		scissor.offset.x		= psoDesc.scissor.offset.x;
		scissor.offset.y		= psoDesc.scissor.offset.y;
	}

	viewportState.pViewports	= &viewport;
	viewportState.viewportCount = 1;
	viewportState.pScissors		= &scissor;
	viewportState.scissorCount	= 1;
}

VkPipeline VulkanCache::CreatePipeline(const PipelineStateDesc& psoDesc, VkPipelineLayout pipelineLayout) const
{
	VkPipelineDepthStencilStateCreateInfo depthStencilState = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	CreateDepthStencilState(psoDesc, depthStencilState);

	VkPipelineRasterizationStateCreateInfo rasterizationState = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	CreateRasterizationState(psoDesc, rasterizationState);

	VkPipelineMultisampleStateCreateInfo multisampleState = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	CreateMultisampleState(psoDesc, multisampleState);

	std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachmentStates;
	VkPipelineColorBlendStateCreateInfo colorBlendState = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	CreateColorBlendState(psoDesc, colorBlendState, colorBlendAttachmentStates);

	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	CreateShaderStages(psoDesc, shaderStages);

	VkSpecializationInfo specializationInfo;
	CreateSpecializationInfo(psoDesc, specializationInfo, shaderStages);

	std::vector<VkVertexInputAttributeDescription> vertexAttributes;
	std::vector<VkVertexInputBindingDescription> vertexBindings;
	VkPipelineVertexInputStateCreateInfo vertexInputState = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	CreateVertexInputState(psoDesc, vertexInputState, vertexAttributes, vertexBindings);
	
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	CreateInputAssemblyState(psoDesc, inputAssemblyState);

	VkViewport viewport;
	VkRect2D scissor;
	VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	CreateViewportState(psoDesc, viewport, scissor, viewportState);

	VkPipelineDynamicStateCreateInfo dynamicState = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	dynamicState.dynamicStateCount	= static_cast<uint32>(psoDesc.dynamicStates.size());
	dynamicState.pDynamicStates		= reinterpret_cast<const VkDynamicState*>(psoDesc.dynamicStates.data());

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
		.renderPass				= psoDesc.renderPass.GetRenderPass(),
		.subpass				= 0,
		.basePipelineHandle		= VK_NULL_HANDLE,
		.basePipelineIndex		= -1,
	};

	VkPipeline pipeline;
	vulkan(vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline));

	return pipeline;
}

VkPipeline VulkanCache::CreatePipeline(const ComputePipelineDesc& computeDesc, VkPipelineLayout pipelineLayout) const
{
	VkComputePipelineCreateInfo pipelineInfo = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
	pipelineInfo.layout				= pipelineLayout;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	VkPipelineShaderStageCreateInfo& stage = pipelineInfo.stage;
	stage			= { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	stage.stage		= VK_SHADER_STAGE_COMPUTE_BIT;
	stage.module	= computeDesc.computeShader->compilationInfo.shaderModule;
	stage.pName		= computeDesc.computeShader->compilationInfo.entrypoint.data();

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
	vulkan(vkCreateComputePipelines(Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline));

	return pipeline;
}

VkPipelineLayout VulkanCache::GetPipelineLayout(
	const std::vector<VkDescriptorSetLayout>& layouts, 
	const std::vector<VkPushConstantRange>& pushConstantRanges)
{
	const Crc crc0 = Platform::CalculateCrc(layouts.data(), layouts.size() * sizeof(layouts.front()));
	const Crc crc1 = Platform::CalculateCrc(pushConstantRanges.data(), pushConstantRanges.size() * sizeof(pushConstantRanges[0]));

	Crc crc = 0;
	HashCombine(crc, crc0);
	HashCombine(crc, crc1);

	if (auto iter = PipelineLayoutCache.find(crc); iter != PipelineLayoutCache.end())
	{
		const auto& [cachedCrc, cachedPipelineLayout] = *iter;
		return cachedPipelineLayout;
	}

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipelineLayoutInfo.setLayoutCount	= static_cast<uint32>(layouts.size());
	pipelineLayoutInfo.pSetLayouts		= layouts.data();

	if (pushConstantRanges.size() > 0)
	{
		pipelineLayoutInfo.pushConstantRangeCount	= static_cast<uint32>(pushConstantRanges.size());
		pipelineLayoutInfo.pPushConstantRanges		= pushConstantRanges.data();
	}

	VkPipelineLayout pipelineLayout;
	vulkan(vkCreatePipelineLayout(Device, &pipelineLayoutInfo, nullptr, &pipelineLayout));

	PipelineLayoutCache[crc] = pipelineLayout;

	return pipelineLayout;
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
	_Device.GetCache().Destroy(_Pipeline);
}