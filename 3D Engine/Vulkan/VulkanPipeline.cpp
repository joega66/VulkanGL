#include "VulkanDevice.h"
#include "VulkanDRM.h"

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

VkPipeline VulkanDevice::GetPipeline(
	const PipelineStateInitializer& PSOInit,
	VkPipelineLayout PipelineLayout,
	VkRenderPass RenderPass,
	uint32 NumRenderTargets)
{
	// Find or create the pipeline.
	for (const auto&[CachedPSO, CachedPipelineLayout, CachedRenderPass, CachedNumRenderTargets, CachedPipeline] : PipelineCache)
	{
		if (PSOInit == CachedPSO && 
			PipelineLayout == CachedPipelineLayout &&
			RenderPass == CachedRenderPass &&
			NumRenderTargets == CachedNumRenderTargets)
		{
			return CachedPipeline;
		}
	}

	VkPipeline Pipeline = CreatePipeline(PSOInit, PipelineLayout, RenderPass, NumRenderTargets);
	PipelineCache.push_back({ PSOInit, PipelineLayout, RenderPass, NumRenderTargets, Pipeline });
	return Pipeline;
}

VkPipeline VulkanDevice::CreatePipeline(
	const PipelineStateInitializer& PSOInit,
	VkPipelineLayout PipelineLayout,
	VkRenderPass RenderPass,
	uint32 NumRenderTargets)
{
	VkPipelineDepthStencilStateCreateInfo DepthStencilState = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };

	{
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

	std::array<VkPipelineColorBlendAttachmentState, MaxRenderTargets> ColorBlendAttachmentStates;

	{
		for (uint32 RenderTargetIndex = 0; RenderTargetIndex < NumRenderTargets; RenderTargetIndex++)
		{
			const auto& In = PSOInit.ColorBlendAttachmentStates[RenderTargetIndex];
			auto& Out = ColorBlendAttachmentStates[RenderTargetIndex];

			Out = {};

			Out.blendEnable = In.BlendEnable;

			static const HashTable<EBlendOp, VkBlendOp> VulkanBlendOp =
			{
				ENTRY(EBlendOp::ADD, VK_BLEND_OP_ADD)
				ENTRY(EBlendOp::SUBTRACT, VK_BLEND_OP_SUBTRACT)
				ENTRY(EBlendOp::REVERSE_SUBTRACT, VK_BLEND_OP_REVERSE_SUBTRACT)
				ENTRY(EBlendOp::MIN, VK_BLEND_OP_MIN)
				ENTRY(EBlendOp::MAX,VK_BLEND_OP_MAX)
			};

			static const auto EngineToVulkanBlendOp = [&] (EBlendOp BlendOp) { return VulkanBlendOp.at(BlendOp); };

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

			static const auto EngineToVulkanBlendFactor = [&] (EBlendFactor BlendFactor) { return VulkanBlendFactor.at(BlendFactor); };

			Out.srcColorBlendFactor = EngineToVulkanBlendFactor(In.SrcColorBlendFactor);
			Out.dstColorBlendFactor = EngineToVulkanBlendFactor(In.DstColorBlendFactor);
			Out.colorBlendOp = EngineToVulkanBlendOp(In.ColorBlendOp);
			Out.srcAlphaBlendFactor = EngineToVulkanBlendFactor(In.SrcAlphaBlendFactor);
			Out.dstAlphaBlendFactor = EngineToVulkanBlendFactor(In.DstAlphaBlendFactor);
			Out.alphaBlendOp = EngineToVulkanBlendOp(In.AlphaBlendOp);

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
		const VulkanShader& VulkanShader = ShaderCache[Shader->ResourceTable.Type];
		VkPipelineShaderStageCreateInfo& ShaderStage = ShaderStages[i];
		ShaderStage.stage = VulkanShader::GetVulkanStage(Shader->ResourceTable.Stage);
		ShaderStage.module = VulkanShader.ShaderModule;
		ShaderStage.pName = Shader->ResourceTable.Entrypoint.data();
	}

	VkPipelineVertexInputStateCreateInfo VertexInputState = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	const std::vector<VkVertexInputAttributeDescription>& AttributeDescriptions = ShaderCache[GraphicsPipeline.Vertex->ResourceTable.Type].Attributes;
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
		Viewport.x = (float)In.X;
		Viewport.y = (float)In.Y;
		Viewport.width = (float)In.Width;
		Viewport.height = (float)In.Height;
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

VkPipelineLayout VulkanDevice::GetPipelineLayout(const std::vector<VkDescriptorSetLayout>& DescriptorSetLayouts)
{
	for (const auto&[CachedDescriptorSetLayouts, CachedPipelineLayout] : PipelineLayoutCache)
	{
		if (std::equal(
			CachedDescriptorSetLayouts.begin(), CachedDescriptorSetLayouts.end(), 
			DescriptorSetLayouts.begin(), DescriptorSetLayouts.end())
			)
		{
			return CachedPipelineLayout;
		}
	}

	VkPipelineLayoutCreateInfo PipelineLayoutInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	PipelineLayoutInfo.setLayoutCount = DescriptorSetLayouts.size();
	PipelineLayoutInfo.pSetLayouts = DescriptorSetLayouts.data();

	VkPipelineLayout PipelineLayout;
	vulkan(vkCreatePipelineLayout(Device, &PipelineLayoutInfo, nullptr, &PipelineLayout));

	PipelineLayoutCache.push_back({ DescriptorSetLayouts, PipelineLayout });

	return PipelineLayout;
}