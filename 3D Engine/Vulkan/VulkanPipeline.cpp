#include "VulkanCache.h"
#include "VulkanDevice.h"
#include <GPU/GPUShader.h>
#include "VulkanPipeline.h"

gpu::Pipeline VulkanCache::GetPipeline(const PipelineStateDesc& PSODesc)
{
	if (auto Iter = GraphicsPipelineCache.find(PSODesc); Iter != GraphicsPipelineCache.end())
	{
		return Iter->second;
	}

	const VkPipelineLayout PipelineLayout = GetPipelineLayout(PSODesc.layouts, PSODesc.pushConstantRanges);
	auto Pipeline = std::make_shared<VulkanPipeline>(Device, CreatePipeline(PSODesc, PipelineLayout), PipelineLayout, VK_PIPELINE_BIND_POINT_GRAPHICS);
	GraphicsPipelineCache[PSODesc] = Pipeline;
	return Pipeline;
}

gpu::Pipeline VulkanCache::GetPipeline(const ComputePipelineDesc& ComputeDesc)
{
	auto& MapEntries = ComputeDesc.specInfo.GetMapEntries();
	auto& Data = ComputeDesc.specInfo.GetData();
	auto& SetLayouts = ComputeDesc.Layouts;

	struct ComputePipelineHash
	{
		Crc ComputeShaderCrc;
		Crc MapEntriesCrc;
		Crc MapDataCrc;
		Crc SetLayoutsCrc;
	};

	ComputePipelineHash ComputeHash;
	ComputeHash.ComputeShaderCrc = Platform::CalculateCrc(ComputeDesc.computeShader, sizeof(ComputeDesc.computeShader));
	ComputeHash.MapEntriesCrc = Platform::CalculateCrc(MapEntries.data(), MapEntries.size() * sizeof(SpecializationInfo::SpecializationMapEntry));
	ComputeHash.MapDataCrc = Platform::CalculateCrc(Data.data(), Data.size());
	ComputeHash.SetLayoutsCrc = Platform::CalculateCrc(SetLayouts.data(), SetLayouts.size() * sizeof(VkDescriptorSetLayout));

	const Crc Crc = Platform::CalculateCrc(&ComputeHash, sizeof(ComputeHash));

	if (auto Iter = ComputePipelineCache.find(Crc); Iter != ComputePipelineCache.end())
	{
		return Iter->second;
	}

	const auto& InPushConstantRange = ComputeDesc.computeShader->compilationInfo.pushConstantRange;

	const auto PushConstantRanges = InPushConstantRange.size > 0 ? std::vector{ InPushConstantRange } : std::vector<PushConstantRange>{};

	const VkPipelineLayout PipelineLayout = GetPipelineLayout(ComputeDesc.Layouts, PushConstantRanges);

	auto Pipeline = std::make_shared<VulkanPipeline>(Device, CreatePipeline(ComputeDesc, PipelineLayout), PipelineLayout, VK_PIPELINE_BIND_POINT_COMPUTE);

	ComputePipelineCache[Crc] = Pipeline;

	CrcToComputeDesc[Crc] = ComputeDesc;

	return Pipeline;
}

static void CreateDepthStencilState(const PipelineStateDesc& PSODesc, VkPipelineDepthStencilStateCreateInfo& DepthStencilState)
{
	static const std::unordered_map<EStencilOp, VkStencilOp> VulkanStencilOp =
	{
		ENTRY(EStencilOp::Keep, VK_STENCIL_OP_KEEP)
		ENTRY(EStencilOp::Replace, VK_STENCIL_OP_REPLACE)
		ENTRY(EStencilOp::Zero, VK_STENCIL_OP_ZERO)
	};

	static const std::unordered_map<ECompareOp, VkCompareOp> VulkanCompareOp =
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

	static const std::unordered_map<EDepthCompareTest, VkCompareOp> VulkanDepthCompare =
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

	const auto& In = PSODesc.depthStencilState;
	DepthStencilState.depthTestEnable = In.depthTestEnable;
	DepthStencilState.depthWriteEnable = In.depthWriteEnable;
	DepthStencilState.depthCompareOp = VulkanDepthCompare.at(In.depthCompareTest);
	DepthStencilState.stencilTestEnable = In.stencilTestEnable;

	const auto& Back = In.back;
	DepthStencilState.back.failOp = VulkanStencilOp.at(Back.failOp);
	DepthStencilState.back.passOp = VulkanStencilOp.at(Back.passOp);
	DepthStencilState.back.depthFailOp = VulkanStencilOp.at(Back.depthFailOp);
	DepthStencilState.back.compareOp = VulkanCompareOp.at(Back.compareOp);
	DepthStencilState.back.compareMask = Back.compareMask;
	DepthStencilState.back.writeMask = Back.writeMask;
	DepthStencilState.back.reference = Back.reference;
	DepthStencilState.front = DepthStencilState.back;
}

static void CreateRasterizationState(const PipelineStateDesc& PSODesc, VkPipelineRasterizationStateCreateInfo& RasterizationState)
{
	static const std::unordered_map<ECullMode, VkCullModeFlags> VulkanCullMode =
	{
		ENTRY(ECullMode::None, VK_CULL_MODE_NONE)
		ENTRY(ECullMode::Back, VK_CULL_MODE_BACK_BIT)
		ENTRY(ECullMode::Front, VK_CULL_MODE_FRONT_BIT)
		ENTRY(ECullMode::FrontAndBack, VK_CULL_MODE_FRONT_AND_BACK)
	};

	static const std::unordered_map<EFrontFace, VkFrontFace> VulkanFrontFace =
	{
		ENTRY(EFrontFace::CW, VK_FRONT_FACE_CLOCKWISE)
		ENTRY(EFrontFace::CCW, VK_FRONT_FACE_COUNTER_CLOCKWISE)
	};

	static const std::unordered_map<EPolygonMode, VkPolygonMode> VulkanPolygonMode =
	{
		ENTRY(EPolygonMode::Fill, VK_POLYGON_MODE_FILL)
		ENTRY(EPolygonMode::Line, VK_POLYGON_MODE_LINE)
		ENTRY(EPolygonMode::Point, VK_POLYGON_MODE_POINT)
	};

	const auto& In = PSODesc.rasterizationState;
	RasterizationState.depthClampEnable = In.depthClampEnable;
	RasterizationState.rasterizerDiscardEnable = In.rasterizerDiscardEnable;
	RasterizationState.polygonMode = VulkanPolygonMode.at(In.polygonMode);
	RasterizationState.cullMode = VulkanCullMode.at(In.cullMode);
	RasterizationState.frontFace = VulkanFrontFace.at(In.frontFace);
	RasterizationState.depthBiasEnable = In.depthBiasEnable;
	RasterizationState.depthBiasConstantFactor = In.depthBiasConstantFactor;
	RasterizationState.depthBiasClamp = In.depthBiasClamp;
	RasterizationState.depthBiasSlopeFactor = In.depthBiasSlopeFactor;
	RasterizationState.lineWidth = In.lineWidth;
}

static void CreateMultisampleState(const PipelineStateDesc& PSODesc, VkPipelineMultisampleStateCreateInfo& MultisampleState)
{
	const auto& In = PSODesc.multisampleState;
	MultisampleState.rasterizationSamples = (VkSampleCountFlagBits)In.rasterizationSamples;
	MultisampleState.sampleShadingEnable = In.sampleShadingEnable;
	MultisampleState.minSampleShading = In.minSampleShading;
	MultisampleState.alphaToCoverageEnable = In.alphaToCoverageEnable;
	MultisampleState.alphaToOneEnable = In.alphaToOneEnable;
}

static void CreateColorBlendState(
	const PipelineStateDesc& PSODesc,
	VkPipelineColorBlendStateCreateInfo& ColorBlendState,
	std::vector<VkPipelineColorBlendAttachmentState>& ColorBlendAttachmentStates)
{
	static const std::unordered_map<EBlendOp, VkBlendOp> VulkanBlendOp =
	{
		ENTRY(EBlendOp::ADD, VK_BLEND_OP_ADD)
		ENTRY(EBlendOp::SUBTRACT, VK_BLEND_OP_SUBTRACT)
		ENTRY(EBlendOp::REVERSE_SUBTRACT, VK_BLEND_OP_REVERSE_SUBTRACT)
		ENTRY(EBlendOp::MIN, VK_BLEND_OP_MIN)
		ENTRY(EBlendOp::MAX,VK_BLEND_OP_MAX)
	};

	static const std::unordered_map<EBlendFactor, VkBlendFactor> VulkanBlendFactor =
	{
		ENTRY(EBlendFactor::ZERO, VK_BLEND_FACTOR_ZERO)
		ENTRY(EBlendFactor::ONE, VK_BLEND_FACTOR_ONE)
		ENTRY(EBlendFactor::SRC_COLOR, VK_BLEND_FACTOR_SRC_COLOR)
		ENTRY(EBlendFactor::ONE_MINUS_SRC_COLOR, VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR)
		ENTRY(EBlendFactor::DST_COLOR, VK_BLEND_FACTOR_DST_COLOR)
		ENTRY(EBlendFactor::ONE_MINUS_DST_COLOR, VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR)
		ENTRY(EBlendFactor::SRC_ALPHA, VK_BLEND_FACTOR_SRC_ALPHA)
		ENTRY(EBlendFactor::ONE_MINUS_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA)
		ENTRY(EBlendFactor::DST_ALPHA, VK_BLEND_FACTOR_DST_ALPHA)
		ENTRY(EBlendFactor::ONE_MINUS_DST_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA)
		ENTRY(EBlendFactor::CONSTANT_COLOR, VK_BLEND_FACTOR_CONSTANT_COLOR)
		ENTRY(EBlendFactor::ONE_MINUS_CONSTANT_COLOR, VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR)
		ENTRY(EBlendFactor::CONSTANT_ALPHA, VK_BLEND_FACTOR_CONSTANT_ALPHA)
		ENTRY(EBlendFactor::ONE_MINUS_CONSTANT_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA)
		ENTRY(EBlendFactor::SRC_ALPHA_SATURATE, VK_BLEND_FACTOR_SRC_ALPHA_SATURATE)
		ENTRY(EBlendFactor::SRC1_COLOR, VK_BLEND_FACTOR_SRC1_COLOR)
		ENTRY(EBlendFactor::ONE_MINUS_SRC1_COLOR, VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR)
		ENTRY(EBlendFactor::SRC1_ALPHA, VK_BLEND_FACTOR_SRC1_ALPHA)
		ENTRY(EBlendFactor::ONE_MINUS_SRC1_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA)
	};

	static auto ConvertColorBlendAttachmentStates = [] (
		const std::vector<ColorBlendAttachmentState>& InColorBlendAttachmentStates,
		std::vector<VkPipelineColorBlendAttachmentState>& ColorBlendAttachmentStates
	)
	{
		for (uint32 ColorAttachmentIndex = 0; ColorAttachmentIndex < InColorBlendAttachmentStates.size(); ColorAttachmentIndex++)
		{
			const ColorBlendAttachmentState& In = InColorBlendAttachmentStates[ColorAttachmentIndex];
			VkPipelineColorBlendAttachmentState Out;
			Out = {};
			Out.blendEnable = In.blendEnable;
			Out.srcColorBlendFactor = VulkanBlendFactor.at(In.srcColorBlendFactor);
			Out.dstColorBlendFactor = VulkanBlendFactor.at(In.dstColorBlendFactor);
			Out.colorBlendOp = VulkanBlendOp.at(In.colorBlendOp);
			Out.srcAlphaBlendFactor = VulkanBlendFactor.at(In.srcAlphaBlendFactor);
			Out.dstAlphaBlendFactor = VulkanBlendFactor.at(In.dstAlphaBlendFactor);
			Out.alphaBlendOp = VulkanBlendOp.at(In.alphaBlendOp);
			Out.colorWriteMask |= Any(In.colorWriteMask & EColorChannel::R) ? VK_COLOR_COMPONENT_R_BIT : 0;
			Out.colorWriteMask |= Any(In.colorWriteMask & EColorChannel::G) ? VK_COLOR_COMPONENT_G_BIT : 0;
			Out.colorWriteMask |= Any(In.colorWriteMask & EColorChannel::B) ? VK_COLOR_COMPONENT_B_BIT : 0;
			Out.colorWriteMask |= Any(In.colorWriteMask & EColorChannel::A) ? VK_COLOR_COMPONENT_A_BIT : 0;

			ColorBlendAttachmentStates.push_back(Out);
		}
	};

	ColorBlendAttachmentStates.reserve(PSODesc.renderPass.GetNumAttachments());
	if (PSODesc.colorBlendAttachmentStates.empty())
	{
		std::vector<ColorBlendAttachmentState> DefaultColorBlendAttachmentStates(PSODesc.renderPass.GetNumAttachments(), ColorBlendAttachmentState{});
		ConvertColorBlendAttachmentStates(DefaultColorBlendAttachmentStates, ColorBlendAttachmentStates);
	}
	else
	{
		ConvertColorBlendAttachmentStates(PSODesc.colorBlendAttachmentStates, ColorBlendAttachmentStates);
	}
	
	ColorBlendState.blendConstants[0] = 0.0f;
	ColorBlendState.blendConstants[1] = 0.0f;
	ColorBlendState.blendConstants[2] = 0.0f;
	ColorBlendState.blendConstants[3] = 0.0f;
	ColorBlendState.logicOp = VK_LOGIC_OP_COPY;
	ColorBlendState.logicOpEnable = false;
	ColorBlendState.pAttachments = ColorBlendAttachmentStates.data();
	ColorBlendState.attachmentCount = PSODesc.renderPass.GetNumAttachments();
}

static void CreateShaderStageInfos(const PipelineStateDesc& PSODesc, std::vector<VkPipelineShaderStageCreateInfo>& ShaderStageInfos)
{
	std::vector<const gpu::Shader*> ShaderStages;

	ShaderStages.push_back(PSODesc.shaderStages.vertex);

	if (PSODesc.shaderStages.tessControl)
	{
		ShaderStages.push_back(PSODesc.shaderStages.tessControl);
	}
	if (PSODesc.shaderStages.tessEval)
	{
		ShaderStages.push_back(PSODesc.shaderStages.tessEval);
	}
	if (PSODesc.shaderStages.geometry)
	{
		ShaderStages.push_back(PSODesc.shaderStages.geometry);
	}
	if (PSODesc.shaderStages.fragment)
	{
		ShaderStages.push_back(PSODesc.shaderStages.fragment);
	}

	static const std::unordered_map<EShaderStage, VkShaderStageFlagBits> VulkanStages =
	{
		ENTRY(EShaderStage::Vertex, VK_SHADER_STAGE_VERTEX_BIT)
		ENTRY(EShaderStage::TessControl, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT)
		ENTRY(EShaderStage::TessEvaluation, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)
		ENTRY(EShaderStage::Geometry, VK_SHADER_STAGE_GEOMETRY_BIT)
		ENTRY(EShaderStage::Fragment, VK_SHADER_STAGE_FRAGMENT_BIT)
		ENTRY(EShaderStage::Compute, VK_SHADER_STAGE_COMPUTE_BIT)
	};

	ShaderStageInfos.resize(ShaderStages.size(), { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO });

	for (std::size_t StageIndex = 0; StageIndex < ShaderStageInfos.size(); StageIndex++)
	{
		const gpu::Shader* Shader = ShaderStages[StageIndex];
		VkPipelineShaderStageCreateInfo& ShaderStage = ShaderStageInfos[StageIndex];
		ShaderStage.stage = VulkanStages.at(Shader->compilationInfo.stage);
		ShaderStage.module = static_cast<VkShaderModule>(Shader->compilationInfo.module);
		ShaderStage.pName = Shader->compilationInfo.entrypoint.data();
	}
}

static void CreateSpecializationInfo(
	const PipelineStateDesc& PSODesc, 
	VkSpecializationInfo& SpecializationInfo, 
	std::vector<VkPipelineShaderStageCreateInfo>& ShaderStageInfos)
{
	const std::vector<SpecializationInfo::SpecializationMapEntry>& MapEntries = PSODesc.specInfo.GetMapEntries();

	if (MapEntries.size() > 0)
	{
		static_assert(sizeof(VkSpecializationMapEntry) == sizeof(SpecializationInfo::SpecializationMapEntry));

		const std::vector<uint8>& Data = PSODesc.specInfo.GetData();
		SpecializationInfo.mapEntryCount = static_cast<uint32>(MapEntries.size());
		SpecializationInfo.pMapEntries = reinterpret_cast<const VkSpecializationMapEntry*>(MapEntries.data());
		SpecializationInfo.dataSize = Data.size();
		SpecializationInfo.pData = Data.data();

		std::for_each(ShaderStageInfos.begin(), ShaderStageInfos.end(), [&] (VkPipelineShaderStageCreateInfo& ShaderStageInfo)
		{
			ShaderStageInfo.pSpecializationInfo = &SpecializationInfo;
		});
	}
}

static void CreateVertexInputState(
	const PipelineStateDesc& PSODesc,
	VkPipelineVertexInputStateCreateInfo& VertexInputState,
	std::vector<VkVertexInputAttributeDescription>& VulkanVertexAttributes,
	std::vector<VkVertexInputBindingDescription>& VertexBindings)
{
	// If no vertex attributes were provided in the PSO desc, use the ones from shader reflection.
	const std::vector<VertexAttributeDescription>& VertexAttributes =
		PSODesc.vertexAttributes.empty() ? PSODesc.shaderStages.vertex->compilationInfo.vertexAttributeDescriptions : PSODesc.vertexAttributes;

	VulkanVertexAttributes.reserve(VertexAttributes.size());

	for (uint32 VertexAttributeIndex = 0; VertexAttributeIndex < VertexAttributes.size(); VertexAttributeIndex++)
	{
		const VertexAttributeDescription& VertexAttributeDescription = VertexAttributes[VertexAttributeIndex];
		const VkVertexInputAttributeDescription VulkanVertexAttributeDescription =
		{
			static_cast<uint32>(VertexAttributeDescription.location),
			VertexAttributeDescription.binding,
			VulkanImage::GetVulkanFormat(VertexAttributeDescription.format),
			VertexAttributeDescription.offset
		};
		VulkanVertexAttributes.push_back(VulkanVertexAttributeDescription);
	}

	VertexBindings.reserve(VertexAttributes.size());

	if (PSODesc.vertexBindings.empty())
	{
		for (uint32 VertexBindingIndex = 0; VertexBindingIndex < VertexAttributes.size(); VertexBindingIndex++)
		{
			const VkVertexInputBindingDescription VertexBinding =
			{
				VertexAttributes[VertexBindingIndex].binding,
				VulkanImage::GetSize(VertexAttributes[VertexBindingIndex].format),
				VK_VERTEX_INPUT_RATE_VERTEX
			};
			VertexBindings.push_back(VertexBinding);
		}
	}
	else
	{
		for (uint32 VertexBindingIndex = 0; VertexBindingIndex < PSODesc.vertexBindings.size(); VertexBindingIndex++)
		{
			const VkVertexInputBindingDescription VertexBinding =
			{
				PSODesc.vertexBindings[VertexBindingIndex].binding,
				PSODesc.vertexBindings[VertexBindingIndex].stride,
				VK_VERTEX_INPUT_RATE_VERTEX
			};
			VertexBindings.push_back(VertexBinding);
		}
	}

	VertexInputState.vertexBindingDescriptionCount = static_cast<uint32>(VertexBindings.size());
	VertexInputState.pVertexBindingDescriptions = VertexBindings.data();
	VertexInputState.vertexAttributeDescriptionCount = static_cast<uint32>(VulkanVertexAttributes.size());
	VertexInputState.pVertexAttributeDescriptions = VulkanVertexAttributes.data();
}

static void CreateInputAssemblyState(const PipelineStateDesc& PSODesc, VkPipelineInputAssemblyStateCreateInfo& InputAssemblyState)
{
	InputAssemblyState.topology = [&] ()
	{
		for (VkPrimitiveTopology VulkanTopology = VK_PRIMITIVE_TOPOLOGY_BEGIN_RANGE; VulkanTopology < VK_PRIMITIVE_TOPOLOGY_RANGE_SIZE;)
		{
			if (static_cast<uint32>(PSODesc.inputAssemblyState.topology) == VulkanTopology)
			{
				return VulkanTopology;
			}
			VulkanTopology = static_cast<VkPrimitiveTopology>(VulkanTopology + 1);
		}
		fail("VkPrimitiveTopology not found.");
	}();
	InputAssemblyState.primitiveRestartEnable = PSODesc.inputAssemblyState.primitiveRestartEnable;
}

static void CreateViewportState(const PipelineStateDesc& PSODesc, VkViewport& Viewport, VkRect2D& scissor, VkPipelineViewportStateCreateInfo& ViewportState)
{
	Viewport.x = static_cast<float>(PSODesc.viewport.x);
	Viewport.y = static_cast<float>(PSODesc.viewport.y);
	Viewport.width = static_cast<float>(PSODesc.viewport.width);
	Viewport.height = static_cast<float>(PSODesc.viewport.height);
	Viewport.minDepth = PSODesc.viewport.minDepth;
	Viewport.maxDepth = PSODesc.viewport.maxDepth;

	if (PSODesc.scissor == Scissor{})
	{
		scissor.extent.width = static_cast<uint32>(Viewport.width);
		scissor.extent.height = static_cast<uint32>(Viewport.height);
		scissor.offset = { 0, 0 };
	}
	else
	{
		scissor.extent.width = PSODesc.scissor.extent.x;
		scissor.extent.height = PSODesc.scissor.extent.y;
		scissor.offset.x = PSODesc.scissor.offset.x;
		scissor.offset.y = PSODesc.scissor.offset.y;
	}

	ViewportState.pViewports = &Viewport;
	ViewportState.viewportCount = 1;
	ViewportState.pScissors = &scissor;
	ViewportState.scissorCount = 1;
}

VkPipeline VulkanCache::CreatePipeline(const PipelineStateDesc& PSODesc, VkPipelineLayout PipelineLayout) const
{
	VkPipelineDepthStencilStateCreateInfo DepthStencilState = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	CreateDepthStencilState(PSODesc, DepthStencilState);

	VkPipelineRasterizationStateCreateInfo RasterizationState = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	CreateRasterizationState(PSODesc, RasterizationState);

	VkPipelineMultisampleStateCreateInfo MultisampleState = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	CreateMultisampleState(PSODesc, MultisampleState);

	std::vector<VkPipelineColorBlendAttachmentState> ColorBlendAttachmentStates;
	VkPipelineColorBlendStateCreateInfo ColorBlendState = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	CreateColorBlendState(PSODesc, ColorBlendState, ColorBlendAttachmentStates);

	std::vector<VkPipelineShaderStageCreateInfo> ShaderStageInfos;
	CreateShaderStageInfos(PSODesc, ShaderStageInfos);

	VkSpecializationInfo SpecializationInfo;
	CreateSpecializationInfo(PSODesc, SpecializationInfo, ShaderStageInfos);

	std::vector<VkVertexInputAttributeDescription> VulkanVertexAttributes;
	std::vector<VkVertexInputBindingDescription> VertexBindings;
	VkPipelineVertexInputStateCreateInfo VertexInputState = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	CreateVertexInputState(PSODesc, VertexInputState, VulkanVertexAttributes, VertexBindings);
	
	VkPipelineInputAssemblyStateCreateInfo InputAssemblyState = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	CreateInputAssemblyState(PSODesc, InputAssemblyState);

	VkViewport Viewport;
	VkRect2D Scissor;
	VkPipelineViewportStateCreateInfo ViewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	CreateViewportState(PSODesc, Viewport, Scissor, ViewportState);

	VkPipelineDynamicStateCreateInfo DynamicState = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	DynamicState.dynamicStateCount = static_cast<uint32>(PSODesc.dynamicStates.size());
	DynamicState.pDynamicStates = reinterpret_cast<const VkDynamicState*>(PSODesc.dynamicStates.data());

	VkGraphicsPipelineCreateInfo PipelineInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	PipelineInfo.stageCount = static_cast<uint32>(ShaderStageInfos.size());
	PipelineInfo.pStages = ShaderStageInfos.data();
	PipelineInfo.pVertexInputState = &VertexInputState;
	PipelineInfo.pInputAssemblyState = &InputAssemblyState;
	PipelineInfo.pViewportState = &ViewportState;
	PipelineInfo.pRasterizationState = &RasterizationState;
	PipelineInfo.pMultisampleState = &MultisampleState;
	PipelineInfo.pDepthStencilState = &DepthStencilState;
	PipelineInfo.pColorBlendState = &ColorBlendState;
	PipelineInfo.pDynamicState = &DynamicState;
	PipelineInfo.layout = PipelineLayout;
	PipelineInfo.renderPass = PSODesc.renderPass.GetRenderPass();
	PipelineInfo.subpass = 0;
	PipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	PipelineInfo.basePipelineIndex = -1;

	VkPipeline Pipeline;
	vulkan(vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &Pipeline));

	return Pipeline;
}

VkPipeline VulkanCache::CreatePipeline(const ComputePipelineDesc& ComputePipelineDesc, VkPipelineLayout PipelineLayout) const
{
	VkComputePipelineCreateInfo ComputePipelineCreateInfo = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
	ComputePipelineCreateInfo.layout = PipelineLayout;
	ComputePipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;

	const gpu::Shader* ComputeShader = ComputePipelineDesc.computeShader;

	VkPipelineShaderStageCreateInfo& PipelineShaderStageCreateInfo = ComputePipelineCreateInfo.stage;
	PipelineShaderStageCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	PipelineShaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	PipelineShaderStageCreateInfo.module = static_cast<VkShaderModule>(ComputeShader->compilationInfo.module);
	PipelineShaderStageCreateInfo.pName = ComputeShader->compilationInfo.entrypoint.data();

	VkSpecializationInfo SpecializationInfo;
	const std::vector<SpecializationInfo::SpecializationMapEntry>& MapEntries = ComputePipelineDesc.specInfo.GetMapEntries();
	
	if (MapEntries.size() > 0)
	{
		static_assert(sizeof(VkSpecializationMapEntry) == sizeof(SpecializationInfo::SpecializationMapEntry));
		const std::vector<uint8>& Data = ComputePipelineDesc.specInfo.GetData();
		SpecializationInfo.mapEntryCount = static_cast<uint32>(MapEntries.size());
		SpecializationInfo.pMapEntries = reinterpret_cast<const VkSpecializationMapEntry*>(MapEntries.data());
		SpecializationInfo.dataSize = Data.size();
		SpecializationInfo.pData = Data.data();
		PipelineShaderStageCreateInfo.pSpecializationInfo = &SpecializationInfo;
	}

	VkPipeline Pipeline;
	vulkan(vkCreateComputePipelines(Device, VK_NULL_HANDLE, 1, &ComputePipelineCreateInfo, nullptr, &Pipeline));

	return Pipeline;
}

static std::vector<VkPushConstantRange> CreatePushConstantRanges(const std::vector<PushConstantRange>& PushConstantRanges)
{
	std::vector<VkPushConstantRange> VulkanPushConstantRanges;
	VulkanPushConstantRanges.reserve(PushConstantRanges.size());

	for (const auto& PushConstantRange : PushConstantRanges)
	{
		VkPushConstantRange VulkanPushConstantRange = {};
		VulkanPushConstantRange.stageFlags |= Any(PushConstantRange.stageFlags & EShaderStage::Vertex) ? VK_SHADER_STAGE_VERTEX_BIT : 0;
		VulkanPushConstantRange.stageFlags |= Any(PushConstantRange.stageFlags & EShaderStage::TessControl) ? VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT : 0;
		VulkanPushConstantRange.stageFlags |= Any(PushConstantRange.stageFlags & EShaderStage::TessEvaluation) ? VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT : 0;
		VulkanPushConstantRange.stageFlags |= Any(PushConstantRange.stageFlags & EShaderStage::Geometry) ? VK_SHADER_STAGE_GEOMETRY_BIT : 0;
		VulkanPushConstantRange.stageFlags |= Any(PushConstantRange.stageFlags & EShaderStage::Fragment) ? VK_SHADER_STAGE_FRAGMENT_BIT : 0;
		VulkanPushConstantRange.stageFlags |= Any(PushConstantRange.stageFlags & EShaderStage::Compute) ? VK_SHADER_STAGE_COMPUTE_BIT : 0;
		VulkanPushConstantRange.offset = PushConstantRange.offset;
		VulkanPushConstantRange.size = PushConstantRange.size;
		VulkanPushConstantRanges.push_back(VulkanPushConstantRange);
	}
	
	return VulkanPushConstantRanges;
}

VkPipelineLayout VulkanCache::GetPipelineLayout(
	const std::vector<VkDescriptorSetLayout>& Layouts, 
	const std::vector<PushConstantRange>& PushConstantRanges
)
{
	const Crc Crc0 = Platform::CalculateCrc(Layouts.data(), Layouts.size() * sizeof(Layouts.front()));
	const Crc Crc1 = Platform::CalculateCrc(PushConstantRanges.data(), PushConstantRanges.size() * sizeof(PushConstantRange));

	Crc Crc = 0;
	HashCombine(Crc, Crc0);
	HashCombine(Crc, Crc1);

	const std::vector<VkPushConstantRange> VulkanPushConstantRanges = CreatePushConstantRanges(PushConstantRanges);

	if (auto Iter = PipelineLayoutCache.find(Crc); Iter != PipelineLayoutCache.end())
	{
		const auto& [CachedCrc, CachedPipelineLayout] = *Iter;
		return CachedPipelineLayout;
	}

	VkPipelineLayoutCreateInfo PipelineLayoutInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	PipelineLayoutInfo.setLayoutCount = static_cast<uint32>(Layouts.size());
	PipelineLayoutInfo.pSetLayouts = Layouts.data();

	if (VulkanPushConstantRanges.size() > 0)
	{
		PipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32>(VulkanPushConstantRanges.size());
		PipelineLayoutInfo.pPushConstantRanges = VulkanPushConstantRanges.data();
	}

	VkPipelineLayout PipelineLayout;
	vulkan(vkCreatePipelineLayout(Device, &PipelineLayoutInfo, nullptr, &PipelineLayout));

	PipelineLayoutCache[Crc] = PipelineLayout;

	return PipelineLayout;
}

VulkanPipeline::VulkanPipeline(VulkanDevice& Device, 
	VkPipeline Pipeline, 
	VkPipelineLayout PipelineLayout, 
	VkPipelineBindPoint PipelineBindPoint)
	: Device(Device)
	, Pipeline(Pipeline)
	, PipelineLayout(PipelineLayout)
	, PipelineBindPoint(PipelineBindPoint)
{
}

VulkanPipeline::~VulkanPipeline()
{
	Device.GetCache().Destroy(Pipeline);
}