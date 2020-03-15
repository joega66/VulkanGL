#include "VulkanCache.h"
#include "VulkanDevice.h"
#include <DRMShader.h>
#include "VulkanPipeline.h"

std::pair<VkPipeline, VkPipelineLayout> VulkanCache::GetPipeline(const PipelineStateDesc& PSODesc)
{
	const VkPipelineLayout PipelineLayout = GetPipelineLayout(PSODesc.Layouts);
	 
	for (const auto& [OtherPSODesc, Pipeline] : GraphicsPipelineCache)
	{
		if (PSODesc == OtherPSODesc)
		{
			return { Pipeline, PipelineLayout };
		}
	}

	const VkPipeline Pipeline = CreatePipeline(PSODesc, PipelineLayout);
	GraphicsPipelineCache.push_back({ PSODesc, Pipeline });
	return { Pipeline, PipelineLayout };

	/*if (const auto Iter = GraphicsPipelineCache.find(PSODesc); Iter != GraphicsPipelineCache.end())
	{
		return { Iter->second, PipelineLayout };
	}
	
	const VkPipeline Pipeline = CreatePipeline(PSODesc, PipelineLayout);
	GraphicsPipelineCache.emplace(PSODesc, Pipeline);
	return { Pipeline, PipelineLayout };*/
}

std::pair<VkPipeline, VkPipelineLayout> VulkanCache::GetPipeline(const ComputePipelineDesc& ComputeDesc)
{
	auto& MapEntries = ComputeDesc.SpecializationInfo.GetMapEntries();
	auto& Data = ComputeDesc.SpecializationInfo.GetData();
	auto& SetLayouts = ComputeDesc.Layouts;

	struct ComputePipelineHash
	{
		Crc ComputeShaderCrc;
		Crc MapEntriesCrc;
		Crc MapDataCrc;
		Crc SetLayoutsCrc;
	};

	ComputePipelineHash ComputeHash;
	ComputeHash.ComputeShaderCrc = CalculateCrc(ComputeDesc.ComputeShader, sizeof(ComputeDesc.ComputeShader));
	ComputeHash.MapEntriesCrc = CalculateCrc(MapEntries.data(), MapEntries.size() * sizeof(SpecializationInfo::SpecializationMapEntry));
	ComputeHash.MapDataCrc = CalculateCrc(Data.data(), Data.size());
	ComputeHash.SetLayoutsCrc = CalculateCrc(SetLayouts.data(), SetLayouts.size() * sizeof(VkDescriptorSetLayout));

	const Crc Crc = CalculateCrc(&ComputeHash, sizeof(ComputeHash));

	const VkPipelineLayout PipelineLayout = GetPipelineLayout(ComputeDesc.Layouts);

	if (auto Iter = ComputePipelineCache.find(Crc); Iter != ComputePipelineCache.end())
	{
		return { Iter->second, PipelineLayout };
	}

	const VkPipeline Pipeline = CreatePipeline(ComputeDesc, PipelineLayout);
	ComputePipelineCache[Crc] = Pipeline;
	return { Pipeline, PipelineLayout };
}

static void CreateDepthStencilState(const PipelineStateDesc& PSODesc, VkPipelineDepthStencilStateCreateInfo& DepthStencilState)
{
	static const HashTable<EStencilOp, VkStencilOp> VulkanStencilOp =
	{
		ENTRY(EStencilOp::Keep, VK_STENCIL_OP_KEEP)
		ENTRY(EStencilOp::Replace, VK_STENCIL_OP_REPLACE)
		ENTRY(EStencilOp::Zero, VK_STENCIL_OP_ZERO)
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

	const auto& In = PSODesc.DepthStencilState;
	DepthStencilState.depthTestEnable = In.DepthTestEnable;
	DepthStencilState.depthWriteEnable = In.DepthWriteEnable;
	DepthStencilState.depthCompareOp = VulkanDepthCompare.at(In.DepthCompareTest);
	DepthStencilState.stencilTestEnable = In.StencilTestEnable;

	const auto& Back = In.Back;
	DepthStencilState.back.failOp = VulkanStencilOp.at(Back.FailOp);
	DepthStencilState.back.passOp = VulkanStencilOp.at(Back.PassOp);
	DepthStencilState.back.depthFailOp = VulkanStencilOp.at(Back.DepthFailOp);
	DepthStencilState.back.compareOp = VulkanCompareOp.at(Back.CompareOp);
	DepthStencilState.back.compareMask = Back.CompareMask;
	DepthStencilState.back.writeMask = Back.WriteMask;
	DepthStencilState.back.reference = Back.Reference;
	DepthStencilState.front = DepthStencilState.back;
}

static void CreateRasterizationState(const PipelineStateDesc& PSODesc, VkPipelineRasterizationStateCreateInfo& RasterizationState)
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

	const auto& In = PSODesc.RasterizationState;
	RasterizationState.depthClampEnable = In.DepthClampEnable;
	RasterizationState.rasterizerDiscardEnable = In.RasterizerDiscardEnable;
	RasterizationState.polygonMode = VulkanPolygonMode.at(In.PolygonMode);
	RasterizationState.cullMode = VulkanCullMode.at(In.CullMode);
	RasterizationState.frontFace = VulkanFrontFace.at(In.FrontFace);
	RasterizationState.depthBiasEnable = In.DepthBiasEnable;
	RasterizationState.depthBiasConstantFactor = In.DepthBiasConstantFactor;
	RasterizationState.depthBiasClamp = In.DepthBiasClamp;
	RasterizationState.depthBiasSlopeFactor = In.DepthBiasSlopeFactor;
	RasterizationState.lineWidth = In.LineWidth;
}

static void CreateMultisampleState(const PipelineStateDesc& PSODesc, VkPipelineMultisampleStateCreateInfo& MultisampleState)
{
	const auto& In = PSODesc.MultisampleState;
	MultisampleState.rasterizationSamples = (VkSampleCountFlagBits)In.RasterizationSamples;
	MultisampleState.sampleShadingEnable = In.SampleShadingEnable;
	MultisampleState.minSampleShading = In.MinSampleShading;
	MultisampleState.alphaToCoverageEnable = In.AlphaToCoverageEnable;
	MultisampleState.alphaToOneEnable = In.AlphaToOneEnable;
}

static void CreateColorBlendState(
	const PipelineStateDesc& PSODesc,
	VkPipelineColorBlendStateCreateInfo& ColorBlendState,
	std::vector<VkPipelineColorBlendAttachmentState>& ColorBlendAttachmentStates)
{
	static const HashTable<EBlendOp, VkBlendOp> VulkanBlendOp =
	{
		ENTRY(EBlendOp::ADD, VK_BLEND_OP_ADD)
		ENTRY(EBlendOp::SUBTRACT, VK_BLEND_OP_SUBTRACT)
		ENTRY(EBlendOp::REVERSE_SUBTRACT, VK_BLEND_OP_REVERSE_SUBTRACT)
		ENTRY(EBlendOp::MIN, VK_BLEND_OP_MIN)
		ENTRY(EBlendOp::MAX,VK_BLEND_OP_MAX)
	};

	static const HashTable<EBlendFactor, VkBlendFactor> VulkanBlendFactor =
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
			Out.blendEnable = In.BlendEnable;
			Out.srcColorBlendFactor = VulkanBlendFactor.at(In.SrcColorBlendFactor);
			Out.dstColorBlendFactor = VulkanBlendFactor.at(In.DstColorBlendFactor);
			Out.colorBlendOp = VulkanBlendOp.at(In.ColorBlendOp);
			Out.srcAlphaBlendFactor = VulkanBlendFactor.at(In.SrcAlphaBlendFactor);
			Out.dstAlphaBlendFactor = VulkanBlendFactor.at(In.DstAlphaBlendFactor);
			Out.alphaBlendOp = VulkanBlendOp.at(In.AlphaBlendOp);
			Out.colorWriteMask |= Any(In.ColorWriteMask & EColorChannel::R) ? VK_COLOR_COMPONENT_R_BIT : 0;
			Out.colorWriteMask |= Any(In.ColorWriteMask & EColorChannel::G) ? VK_COLOR_COMPONENT_G_BIT : 0;
			Out.colorWriteMask |= Any(In.ColorWriteMask & EColorChannel::B) ? VK_COLOR_COMPONENT_B_BIT : 0;
			Out.colorWriteMask |= Any(In.ColorWriteMask & EColorChannel::A) ? VK_COLOR_COMPONENT_A_BIT : 0;

			ColorBlendAttachmentStates.push_back(Out);
		}
	};

	ColorBlendAttachmentStates.reserve(PSODesc.RenderPass.GetNumAttachments());
	if (PSODesc.ColorBlendAttachmentStates.empty())
	{
		std::vector<ColorBlendAttachmentState> DefaultColorBlendAttachmentStates(PSODesc.RenderPass.GetNumAttachments(), ColorBlendAttachmentState{});
		ConvertColorBlendAttachmentStates(DefaultColorBlendAttachmentStates, ColorBlendAttachmentStates);
	}
	else
	{
		ConvertColorBlendAttachmentStates(PSODesc.ColorBlendAttachmentStates, ColorBlendAttachmentStates);
	}
	
	ColorBlendState.blendConstants[0] = 0.0f;
	ColorBlendState.blendConstants[1] = 0.0f;
	ColorBlendState.blendConstants[2] = 0.0f;
	ColorBlendState.blendConstants[3] = 0.0f;
	ColorBlendState.logicOp = VK_LOGIC_OP_COPY;
	ColorBlendState.logicOpEnable = false;
	ColorBlendState.pAttachments = ColorBlendAttachmentStates.data();
	ColorBlendState.attachmentCount = PSODesc.RenderPass.GetNumAttachments();
}

static void CreateShaderStageInfos(const PipelineStateDesc& PSODesc, std::vector<VkPipelineShaderStageCreateInfo>& ShaderStageInfos)
{
	std::vector<const drm::Shader*> ShaderStages;

	ShaderStages.push_back(PSODesc.ShaderStages.Vertex);

	if (PSODesc.ShaderStages.TessControl)
	{
		ShaderStages.push_back(PSODesc.ShaderStages.TessControl);
	}
	if (PSODesc.ShaderStages.TessEval)
	{
		ShaderStages.push_back(PSODesc.ShaderStages.TessEval);
	}
	if (PSODesc.ShaderStages.Geometry)
	{
		ShaderStages.push_back(PSODesc.ShaderStages.Geometry);
	}
	if (PSODesc.ShaderStages.Fragment)
	{
		ShaderStages.push_back(PSODesc.ShaderStages.Fragment);
	}

	static const HashTable<EShaderStage, VkShaderStageFlagBits> VulkanStages =
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
		const drm::Shader* Shader = ShaderStages[StageIndex];
		VkPipelineShaderStageCreateInfo& ShaderStage = ShaderStageInfos[StageIndex];
		ShaderStage.stage = VulkanStages.at(Shader->CompilationInfo.Stage);
		ShaderStage.module = static_cast<VkShaderModule>(Shader->CompilationInfo.Module);
		ShaderStage.pName = Shader->CompilationInfo.Entrypoint.data();
	}
}

static void CreateSpecializationInfo(
	const PipelineStateDesc& PSODesc, 
	VkSpecializationInfo& SpecializationInfo, 
	std::vector<VkPipelineShaderStageCreateInfo>& ShaderStageInfos)
{
	const std::vector<SpecializationInfo::SpecializationMapEntry>& MapEntries = PSODesc.SpecializationInfo.GetMapEntries();

	if (MapEntries.size() > 0)
	{
		static_assert(sizeof(VkSpecializationMapEntry) == sizeof(SpecializationInfo::SpecializationMapEntry));

		const std::vector<uint8>& Data = PSODesc.SpecializationInfo.GetData();
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
		PSODesc.VertexAttributes.empty() ? PSODesc.ShaderStages.Vertex->CompilationInfo.VertexAttributeDescriptions : PSODesc.VertexAttributes;

	VulkanVertexAttributes.reserve(VertexAttributes.size());

	for (uint32 VertexAttributeIndex = 0; VertexAttributeIndex < VertexAttributes.size(); VertexAttributeIndex++)
	{
		const VertexAttributeDescription& VertexAttributeDescription = VertexAttributes[VertexAttributeIndex];
		const VkVertexInputAttributeDescription VulkanVertexAttributeDescription =
		{
			static_cast<uint32>(VertexAttributeDescription.Location),
			VertexAttributeDescription.Binding,
			VulkanImage::GetVulkanFormat(VertexAttributeDescription.Format),
			VertexAttributeDescription.Offset
		};
		VulkanVertexAttributes.push_back(VulkanVertexAttributeDescription);
	}

	VertexBindings.reserve(VertexAttributes.size());

	if (PSODesc.VertexBindings.empty())
	{
		for (uint32 VertexBindingIndex = 0; VertexBindingIndex < VertexAttributes.size(); VertexBindingIndex++)
		{
			const VkVertexInputBindingDescription VertexBinding =
			{
				VertexAttributes[VertexBindingIndex].Binding,
				VulkanImage::GetSize(VertexAttributes[VertexBindingIndex].Format),
				VK_VERTEX_INPUT_RATE_VERTEX
			};
			VertexBindings.push_back(VertexBinding);
		}
	}
	else
	{
		for (uint32 VertexBindingIndex = 0; VertexBindingIndex < PSODesc.VertexBindings.size(); VertexBindingIndex++)
		{
			const VkVertexInputBindingDescription VertexBinding =
			{
				PSODesc.VertexBindings[VertexBindingIndex].Binding,
				PSODesc.VertexBindings[VertexBindingIndex].Stride,
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
			if (static_cast<uint32>(PSODesc.InputAssemblyState.Topology) == VulkanTopology)
			{
				return VulkanTopology;
			}
			VulkanTopology = static_cast<VkPrimitiveTopology>(VulkanTopology + 1);
		}
		fail("VkPrimitiveTopology not found.");
	}();
	InputAssemblyState.primitiveRestartEnable = PSODesc.InputAssemblyState.PrimitiveRestartEnable;
}

static void CreateViewportState(const PipelineStateDesc& PSODesc, VkViewport& Viewport, VkRect2D& Scissor, VkPipelineViewportStateCreateInfo& ViewportState)
{
	Viewport.x = static_cast<float>(PSODesc.Viewport.X);
	Viewport.y = static_cast<float>(PSODesc.Viewport.Y);
	Viewport.width = static_cast<float>(PSODesc.Viewport.Width);
	Viewport.height = static_cast<float>(PSODesc.Viewport.Height);
	Viewport.minDepth = PSODesc.Viewport.MinDepth;
	Viewport.maxDepth = PSODesc.Viewport.MaxDepth;

	if (PSODesc.Scissor == ScissorDesc{})
	{
		Scissor.extent.width = static_cast<uint32>(Viewport.width);
		Scissor.extent.height = static_cast<uint32>(Viewport.height);
		Scissor.offset = { 0, 0 };
	}
	else
	{
		Scissor.extent.width = PSODesc.Scissor.Extent.x;
		Scissor.extent.height = PSODesc.Scissor.Extent.y;
		Scissor.offset.x = PSODesc.Scissor.Offset.x;
		Scissor.offset.y = PSODesc.Scissor.Offset.y;
	}

	ViewportState.pViewports = &Viewport;
	ViewportState.viewportCount = 1;
	ViewportState.pScissors = &Scissor;
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
	DynamicState.dynamicStateCount = static_cast<uint32>(PSODesc.DynamicStates.size());
	DynamicState.pDynamicStates = reinterpret_cast<const VkDynamicState*>(PSODesc.DynamicStates.data());

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
	PipelineInfo.renderPass = PSODesc.RenderPass.GetRenderPass();
	PipelineInfo.subpass = 0;
	PipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	VkPipeline Pipeline;
	vulkan(vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &Pipeline));

	return Pipeline;
}

VkPipeline VulkanCache::CreatePipeline(const ComputePipelineDesc& ComputePipelineDesc, VkPipelineLayout PipelineLayout) const
{
	VkComputePipelineCreateInfo ComputePipelineCreateInfo = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
	ComputePipelineCreateInfo.layout = PipelineLayout;
	ComputePipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;

	const drm::Shader* ComputeShader = ComputePipelineDesc.ComputeShader;

	VkPipelineShaderStageCreateInfo& PipelineShaderStageCreateInfo = ComputePipelineCreateInfo.stage;
	PipelineShaderStageCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	PipelineShaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	PipelineShaderStageCreateInfo.module = static_cast<VkShaderModule>(ComputeShader->CompilationInfo.Module);
	PipelineShaderStageCreateInfo.pName = ComputeShader->CompilationInfo.Entrypoint.data();

	VkSpecializationInfo SpecializationInfo;
	const std::vector<SpecializationInfo::SpecializationMapEntry>& MapEntries = ComputePipelineDesc.SpecializationInfo.GetMapEntries();
	
	if (MapEntries.size() > 0)
	{
		static_assert(sizeof(VkSpecializationMapEntry) == sizeof(SpecializationInfo::SpecializationMapEntry));
		const std::vector<uint8>& Data = ComputePipelineDesc.SpecializationInfo.GetData();
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

VkPipelineLayout VulkanCache::GetPipelineLayout(const std::vector<VkDescriptorSetLayout>& Layouts)
{
	const Crc Crc = CalculateCrc(Layouts.data(), Layouts.size() * sizeof(Layouts.front()));

	if (auto Iter = PipelineLayoutCache.find(Crc); Iter != PipelineLayoutCache.end())
	{
		const auto& [CachedCrc, CachedPipelineLayout] = *Iter;
		return CachedPipelineLayout;
	}

	VkPipelineLayoutCreateInfo PipelineLayoutInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	PipelineLayoutInfo.setLayoutCount = static_cast<uint32>(Layouts.size());
	PipelineLayoutInfo.pSetLayouts = Layouts.data();

	VkPipelineLayout PipelineLayout;
	vulkan(vkCreatePipelineLayout(Device, &PipelineLayoutInfo, nullptr, &PipelineLayout));

	PipelineLayoutCache[Crc] = PipelineLayout;

	return PipelineLayout;
}

void VulkanCache::DestroyPipelinesWithShader(const drm::Shader* Shader)
{
	/*if (Shader->CompilationInfo.Stage == EShaderStage::Compute)
	{
		ComputePipelineCache.erase(std::remove_if(
			ComputePipelineCache.begin(),
			ComputePipelineCache.end(),
			[&] (const auto& Iter)
		{
			const auto&[ComputeDesc, CachedObjects] = Iter;
			if (ComputeDesc.ComputeShader == Shader)
			{
				vkDestroyPipeline(Device, CachedObjects.first, nullptr);
				return true;
			}
			return false;
		}), ComputePipelineCache.end());
	}
	else
	{*/

	auto Iter = std::find_if(GraphicsPipelineCache.begin(), GraphicsPipelineCache.end(), [&] (const auto& Iter)
	{
		const auto& [PSODesc, Pipeline] = Iter;
		if (PSODesc.HasShader(Shader))
		{
			vkDestroyPipeline(Device, Pipeline, nullptr);
			return true;
		}
		return false;
	});

	if (Iter != GraphicsPipelineCache.end())
	{
		GraphicsPipelineCache.erase(Iter);
	}
}

VulkanPipeline::VulkanPipeline(VkPipeline Pipeline, VkPipelineLayout PipelineLayout, VkPipelineBindPoint PipelineBindPoint)
	: Pipeline(Pipeline)
	, PipelineLayout(PipelineLayout)
	, PipelineBindPoint(PipelineBindPoint)
{
}