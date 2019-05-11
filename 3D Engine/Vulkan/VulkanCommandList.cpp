#include "VulkanCommandList.h"
#include "VulkanDRM.h"

static CAST(drm::RenderTargetView, VulkanRenderTargetView);
static CAST(drm::Image, VulkanImage);
static CAST(drm::VertexBuffer, VulkanVertexBuffer);
static CAST(drm::UniformBuffer, VulkanUniformBuffer);
static CAST(drm::StorageBuffer, VulkanStorageBuffer);
static CAST(drm::IndexBuffer, VulkanIndexBuffer);
static CAST(RenderCommandList, VulkanCommandList);

VulkanCommandList::VulkanCommandList(VulkanDevice& Device, VulkanAllocator& Allocator, VulkanDescriptorPool& DescriptorPool)
	: Device(Device)
	, DescriptorPool(DescriptorPool)
	, Allocator(Allocator)
	, RenderPasses([&] (VkRenderPass RenderPass) { vkDestroyRenderPass(Device, RenderPass, nullptr); })
	, Framebuffers([&] (VkFramebuffer Framebuffer) { vkDestroyFramebuffer(Device, Framebuffer, nullptr); })
	, DescriptorSetLayouts([&] (VkDescriptorSetLayout DescriptorSetLayout) { vkDestroyDescriptorSetLayout(Device, DescriptorSetLayout, nullptr); })
	, PipelineLayouts([&] (VkPipelineLayout PipelineLayout) { vkDestroyPipelineLayout(Device, PipelineLayout, nullptr); })
	, Pipelines([&] (VkPipeline Pipeline) { vkDestroyPipeline(Device, Pipeline, nullptr); })
	, Samplers([&] (VkSampler Sampler) { vkDestroySampler(Device, Sampler, nullptr); })
	, CommandBuffer([&] ()
{
	VkCommandBufferAllocateInfo CommandBufferInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	CommandBufferInfo.commandPool = Device.CommandPool;
	CommandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	CommandBufferInfo.commandBufferCount = 1;

	VkCommandBuffer CommandBuffer;
	vkAllocateCommandBuffers(Device, &CommandBufferInfo, &CommandBuffer);

	VkCommandBufferBeginInfo BeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vulkan(vkBeginCommandBuffer(CommandBuffer, &BeginInfo));

	return CommandBuffer;
}())
{
	Pending.VertexStreams.resize(Device.Properties.limits.maxVertexInputBindings, VulkanVertexBufferRef());
}

VulkanCommandList::~VulkanCommandList()
{
	RenderPasses.Destroy();
	Framebuffers.Destroy();
	DescriptorSetLayouts.Destroy();
	PipelineLayouts.Destroy();
	Pipelines.Destroy();
	Samplers.Destroy();

	vkFreeCommandBuffers(Device, Device.CommandPool, 1, &CommandBuffer);
}

void VulkanCommandList::SetRenderTargets(const RenderPassInitializer& RenderPassInit)
{
	if (Pending.RPInit == RenderPassInit)
	{
		return;
	}

	Pending.RPInit = RenderPassInit;

	bDirtyRenderPass = true;
}

void VulkanCommandList::SetVertexStream(uint32 Location, drm::VertexBufferRef VertexBuffer)
{
	check(Location < Device.Properties.limits.maxVertexInputBindings, "Invalid location.");

	VulkanVertexBufferRef VulkanVertexBuffer = ResourceCast(VertexBuffer);
	check(VulkanVertexBuffer, "Invalid vertex buffer.");

	Pending.VertexStreams[Location] = VulkanVertexBuffer;

	bDirtyVertexStreams = true;
}

void VulkanCommandList::SetUniformBuffer(drm::ShaderRef Shader, uint32 Location, drm::UniformBufferRef UniformBuffer)
{
	const auto& VulkanShader = Device.ShaderCache[Shader->Type];
	VulkanUniformBufferRef VulkanUniformBuffer = ResourceCast(UniformBuffer);
	auto& Bindings = VulkanShader.Bindings;
	auto SharedBuffer = VulkanUniformBuffer->Buffer;

	check(SharedBuffer->Shared->Usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, "Invalid buffer type.");

	if (VulkanUniformBuffer->bDirty)
	{
		Allocator.UploadBufferData(*VulkanUniformBuffer->Buffer, VulkanUniformBuffer->GetData());
		VulkanUniformBuffer->bDirty = false;
	}

	for (const auto& Binding : Bindings)
	{
		if (Binding.binding == Location)
		{
			check(Binding.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, "Shader resource at this location isn't a uniform buffer.");

			VkDescriptorBufferInfo BufferInfo = { SharedBuffer->GetVulkanHandle(), SharedBuffer->Offset, SharedBuffer->Size };
			DescriptorBuffers[Shader->Stage][Location] = std::make_unique<VulkanWriteDescriptorBuffer>(Binding, BufferInfo);
			bDirtyDescriptorSets = true;

			return;
		}
	}

	fail("A shader resource doesn't exist at this location.\nLocation: %d", Location);
}

void VulkanCommandList::SetShaderImage(drm::ShaderRef Shader, uint32 Location, drm::ImageRef Image, const SamplerState& Sampler)
{
	const auto& VulkanShader = Device.ShaderCache[Shader->Type];
	VulkanImageRef VulkanImage = ResourceCast(Image);
	auto& Bindings = VulkanShader.Bindings;

	check(VulkanImage->Layout != VK_IMAGE_LAYOUT_UNDEFINED, "Invalid Vulkan image layout for shader read.");

	for (const auto& Binding : Bindings)
	{
		if (Binding.binding == Location)
		{
			check(Binding.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, "Shader resource at this location isn't a combined image sampler.");

			VkSampler VulkanSampler = VulkanImage::CreateSampler(Device, Sampler);
			Samplers.Push(VulkanSampler);

			VkDescriptorImageInfo DescriptorImageInfo = { VulkanSampler, VulkanImage->ImageView, VulkanImage->Layout };
			DescriptorImages[Shader->Stage][Location] = std::make_unique<VulkanWriteDescriptorImage>(Binding, DescriptorImageInfo);
			bDirtyDescriptorSets = true;

			return;
		}
	}

	fail("A shader resource doesn't exist at this location.\nLocation: %d", Location);
}

void VulkanCommandList::SetStorageBuffer(drm::ShaderRef Shader, uint32 Location, drm::StorageBufferRef StorageBuffer)
{
	const auto& VulkanShader = Device.ShaderCache[Shader->Type];
	VulkanStorageBufferRef VulkanStorageBuffer = ResourceCast(StorageBuffer);
	auto& Bindings = VulkanShader.Bindings;
	auto SharedBuffer = VulkanStorageBuffer->Buffer;

	check(SharedBuffer->Shared->Usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, "Invalid buffer type.");

	for (const auto& Binding : Bindings)
	{
		if (Binding.binding == Location)
		{
			check(Binding.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, "Shader resource at this location isn't a storage buffer.");

			VkDescriptorBufferInfo BufferInfo = { SharedBuffer->GetVulkanHandle(), SharedBuffer->Offset, SharedBuffer->Size };
			DescriptorBuffers[Shader->Stage][Location] = std::make_unique<VulkanWriteDescriptorBuffer>(Binding, BufferInfo);
			bDirtyDescriptorSets = true;

			return;
		}
	}

	fail("A shader resource doesn't exist at this location.\nLocation: %d", Location);
}

void VulkanCommandList::DrawIndexed(drm::IndexBufferRef IndexBuffer, uint32 IndexCount, uint32 InstanceCount, uint32 FirstIndex, uint32 VertexOffset, uint32 FirstInstance)
{
	PrepareForDraw();

	VulkanIndexBufferRef VulkanIndexBuffer = ResourceCast(IndexBuffer);
	VkIndexType IndexType = VulkanIndexBuffer->IndexStride == sizeof(uint32) ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16;

	vkCmdBindIndexBuffer(CommandBuffer, VulkanIndexBuffer->Buffer->GetVulkanHandle(), VulkanIndexBuffer->Buffer->Offset, IndexType);
	vkCmdDrawIndexed(CommandBuffer, IndexCount, InstanceCount, FirstIndex, VertexOffset, FirstInstance);
}

void VulkanCommandList::Draw(uint32 VertexCount, uint32 InstanceCount, uint32 FirstVertex, uint32 FirstInstance)
{
	PrepareForDraw();

	vkCmdDraw(CommandBuffer, VertexCount, InstanceCount, FirstVertex, FirstInstance);
}

void VulkanCommandList::Finish()
{
	if (RenderPasses.Size())
	{
		vkCmdEndRenderPass(CommandBuffer);
	}

	vulkan(vkEndCommandBuffer(CommandBuffer));

	bFinished = true;
}

void VulkanCommandList::SetPipelineState(const PipelineStateInitializer& PSOInit)
{
	if (Pending.PSOInit.GraphicsPipelineState != PSOInit.GraphicsPipelineState)
	{
		// Clear descriptors and vertex streams.
		DescriptorImages.clear();
		DescriptorBuffers.clear();
		std::fill(Pending.VertexStreams.begin(), Pending.VertexStreams.end(), VulkanVertexBufferRef());
		bDirtyPipelineLayout = true;
	}

	Pending.PSOInit = PSOInit;

	bDirtyPipeline = true;
}

void VulkanCommandList::CleanPipelineLayout()
{
	std::vector<VkDescriptorSetLayoutBinding> AllBindings;

	auto AddBindings = [&] (const drm::ShaderRef Shader)
	{
		if (Shader)
		{
			const auto& VulkanShader = Device.ShaderCache[Shader->Type];
			const auto& Bindings = VulkanShader.Bindings;
			if (Bindings.size() > 0)
			{
				AllBindings.insert(AllBindings.end(), Bindings.begin(), Bindings.end());
			}
		}
	};

	{
		const auto& GraphicsPipelineState = Pending.PSOInit.GraphicsPipelineState;
		AddBindings(GraphicsPipelineState.Vertex);
		AddBindings(GraphicsPipelineState.TessControl);
		AddBindings(GraphicsPipelineState.TessEval);
		AddBindings(GraphicsPipelineState.Geometry);
		AddBindings(GraphicsPipelineState.Fragment);
	}

	VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	DescriptorSetLayoutInfo.bindingCount = static_cast<uint32>(AllBindings.size());
	DescriptorSetLayoutInfo.pBindings = AllBindings.data();

	VkDescriptorSetLayout DescriptorSetLayout;
	vulkan(vkCreateDescriptorSetLayout(Device, &DescriptorSetLayoutInfo, nullptr, &DescriptorSetLayout));
	DescriptorSetLayouts.Push(DescriptorSetLayout);

	VkPipelineLayoutCreateInfo PipelineLayoutInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	PipelineLayoutInfo.setLayoutCount = 1;
	PipelineLayoutInfo.pSetLayouts = &DescriptorSetLayout;

	VkPipelineLayout PipelineLayout;
	vulkan(vkCreatePipelineLayout(Device, &PipelineLayoutInfo, nullptr, &PipelineLayout));
	PipelineLayouts.Push(PipelineLayout);

	bDirtyPipelineLayout = false;
	bDirtyPipeline = true;
}

void VulkanCommandList::CleanDescriptorSets()
{
	VkDescriptorSet DescriptorSet = DescriptorPool.Spawn(DescriptorSetLayouts.Get());
	std::vector<VkWriteDescriptorSet> WriteDescriptors;

	for (auto& DescriptorImagesInShaderStage : DescriptorImages)
	{
		auto& ImageDescriptors = DescriptorImagesInShaderStage.second;
		for (auto& Descriptors : ImageDescriptors)
		{
			const auto& WriteDescriptorImage = Descriptors.second;
			const auto& Binding = WriteDescriptorImage->Binding;

			VkWriteDescriptorSet WriteDescriptor = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			WriteDescriptor.dstSet = DescriptorSet;
			WriteDescriptor.dstBinding = Binding.binding;
			WriteDescriptor.dstArrayElement = 0;
			WriteDescriptor.descriptorType = Binding.descriptorType;
			WriteDescriptor.descriptorCount = 1;
			WriteDescriptor.pImageInfo = &WriteDescriptorImage->DescriptorImage;

			WriteDescriptors.push_back(WriteDescriptor);
		}
	}

	for (auto& DescriptorBuffersInShaderStage : DescriptorBuffers)
	{
		auto& BufferDescriptors = DescriptorBuffersInShaderStage.second;
		for (auto& Descriptors : BufferDescriptors)
		{
			const auto& WriteDescriptorBuffer = Descriptors.second;
			const auto& Binding = WriteDescriptorBuffer->Binding;

			VkWriteDescriptorSet WriteDescriptor = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			WriteDescriptor.dstSet = DescriptorSet;
			WriteDescriptor.dstBinding = Binding.binding;
			WriteDescriptor.dstArrayElement = 0;
			WriteDescriptor.descriptorType = Binding.descriptorType;
			WriteDescriptor.descriptorCount = 1;
			WriteDescriptor.pBufferInfo = &WriteDescriptorBuffer->DescriptorBuffer;

			WriteDescriptors.push_back(WriteDescriptor);
		}
	}

	vkUpdateDescriptorSets(Device, WriteDescriptors.size(), WriteDescriptors.data(), 0, nullptr);
	vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayouts.Get(), 0, 1, &DescriptorSet, 0, nullptr);

	bDirtyDescriptorSets = false;
}

void VulkanCommandList::CleanVertexStreams()
{
	const std::vector<VulkanVertexBufferRef>& VertexStreams = Pending.VertexStreams;
	std::vector<VkDeviceSize> Offsets;
	std::vector<VkBuffer> Buffers;

	for (uint32 Location = 0; Location < VertexStreams.size(); Location++)
	{
		if (VertexStreams[Location])
		{
			VkDeviceSize Offset = VertexStreams[Location]->Buffer->Offset;
			VkBuffer Buffer = VertexStreams[Location]->Buffer->GetVulkanHandle();
			Offsets.push_back(Offset);
			Buffers.push_back(Buffer);
		}
	}

	vkCmdBindVertexBuffers(CommandBuffer, 0, Buffers.size(), Buffers.data(), Offsets.data());

	bDirtyVertexStreams = false;
}

void VulkanCommandList::PrepareForDraw()
{
	if (bDirtyRenderPass)
	{
		CleanRenderPass();
	}

	if (bDirtyPipelineLayout)
	{
		CleanPipelineLayout();
	}

	if (bDirtyPipeline)
	{
		CleanPipeline();
	}

	if (bDirtyDescriptorSets)
	{
		CleanDescriptorSets();
	}

	if (bDirtyVertexStreams)
	{
		CleanVertexStreams();
	}
}

void VulkanCommandList::CleanRenderPass()
{
	if (RenderPasses.Size() > 1)
	{
		vkCmdEndRenderPass(CommandBuffer);
	}

	{
		// Create the render pass.
		check(Pending.RPInit.NumRenderTargets < RenderPassInitializer::MaxSimultaneousRenderTargets, "Trying to set too many render targets.");

		std::vector<VkAttachmentDescription> Descriptions;
		std::vector<VkAttachmentReference> ColorRefs(Pending.RPInit.NumRenderTargets);
		VkAttachmentReference DepthRef = {};

		if (Pending.RPInit.DepthTarget)
		{
			Descriptions.resize(Pending.RPInit.NumRenderTargets + 1);
		}
		else
		{
			Descriptions.resize(Pending.RPInit.NumRenderTargets);
		}

		for (uint32 i = 0; i < Pending.RPInit.NumRenderTargets; i++)
		{
			check(Pending.RPInit.ColorTargets[i], "Color target is null.");

			VulkanRenderTargetViewRef ColorTarget = ResourceCast(Pending.RPInit.ColorTargets[i]);
			VulkanImageRef Image = ResourceCast(ColorTarget->Image);

			check(Image && Any(Image->Usage & EResourceUsage::RenderTargetable), "Color target is invalid.");
			check(Image->IsColor(), "Color target was not created in color format.");

			Image->Layout = [&] ()
			{
				if (Image == drm::GetSurface())
				{
					bTouchedSurface = true;
					return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
				}
				else if (Any(Image->Usage & EResourceUsage::ShaderResource))
				{
					return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				}
				else if (Any(Image->Usage & EResourceUsage::RenderTargetable))
				{
					return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				}
				else
				{
					fail("Underspecified Resource Create Flags.");
				}
			}();

			VkAttachmentDescription ColorDescription = {};
			ColorDescription.format = Image->GetVulkanFormat();
			ColorDescription.samples = VK_SAMPLE_COUNT_1_BIT;
			ColorDescription.loadOp = ColorTarget->GetVulkanLoadOp();
			ColorDescription.storeOp = ColorTarget->GetVulkanStoreOp();
			ColorDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			ColorDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			ColorDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			ColorDescription.finalLayout = Image->Layout;
			Descriptions[i] = ColorDescription;

			VkAttachmentReference ColorRef = {};
			ColorRef.attachment = i;
			ColorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			ColorRefs[i] = ColorRef;
		}

		if (Pending.RPInit.DepthStencilTransition != EDepthStencilTransition::None)
		{
			VulkanRenderTargetViewRef DepthTarget = ResourceCast(Pending.RPInit.DepthTarget);
			check(DepthTarget, "Depth target is invalid.");

			VulkanImageRef DepthImage = ResourceCast(DepthTarget->Image);
			check(DepthImage && Any(DepthImage->Usage & EResourceUsage::RenderTargetable), "Depth target is invalid.");
			check(DepthImage->IsDepth() || DepthImage->IsStencil(), "Depth target was not created in a depth layout.");

			VkImageLayout FinalLayout = [&] ()
			{
				if (Pending.RPInit.DepthStencilTransition == EDepthStencilTransition::DepthReadStencilRead)
				{
					check(Any(DepthImage->Usage & EResourceUsage::ShaderResource), "Depth Image must be created with EResourceUsage::ShaderResource.");
					return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
				}
				else if (Pending.RPInit.DepthStencilTransition == EDepthStencilTransition::DepthReadStencilWrite)
				{
					check(Any(DepthImage->Usage & EResourceUsage::ShaderResource), "Depth Image must be created with EResourceUsage::ShaderResource.");
					return VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
				}
				else if (Pending.RPInit.DepthStencilTransition == EDepthStencilTransition::DepthWriteStencilRead)
				{
					check(Any(DepthImage->Usage & EResourceUsage::ShaderResource), "Depth Image must be created with EResourceUsage::ShaderResource.");
					return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
				}
				else // DepthWriteStencilWrite
				{
					return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				}
			}();

			VkAttachmentLoadOp LoadOp = DepthTarget->GetVulkanLoadOp();
			VkAttachmentStoreOp StoreOp = DepthTarget->GetVulkanStoreOp();

			VkAttachmentDescription DepthDescription = {};
			DepthDescription.format = DepthImage->GetVulkanFormat();
			DepthDescription.samples = VK_SAMPLE_COUNT_1_BIT;
			DepthDescription.loadOp = DepthImage->IsDepth() ? LoadOp : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			DepthDescription.storeOp = DepthImage->IsDepth() ? StoreOp : VK_ATTACHMENT_STORE_OP_DONT_CARE;
			DepthDescription.stencilLoadOp = DepthImage->IsStencil() ? LoadOp : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			DepthDescription.stencilStoreOp = DepthImage->IsStencil() ? StoreOp : VK_ATTACHMENT_STORE_OP_DONT_CARE;
			DepthDescription.initialLayout = DepthImage->Layout;
			DepthDescription.finalLayout = FinalLayout;
			Descriptions[Pending.RPInit.NumRenderTargets] = DepthDescription;

			DepthRef.attachment = Pending.RPInit.NumRenderTargets;
			DepthRef.layout = FinalLayout;

			DepthImage->Layout = FinalLayout;
		}

		VkSubpassDescription Subpass = {};
		Subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		Subpass.pColorAttachments = ColorRefs.data();
		Subpass.colorAttachmentCount = static_cast<uint32>(ColorRefs.size());
		Subpass.pDepthStencilAttachment = !Pending.RPInit.DepthTarget ? nullptr : &DepthRef;

		std::array<VkSubpassDependency, 2> Dependencies;

		Dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		Dependencies[0].dstSubpass = 0;
		Dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		Dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		Dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		Dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		Dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		Dependencies[1].srcSubpass = 0;
		Dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		Dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		Dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		Dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		Dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		Dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		VkRenderPassCreateInfo RenderPassInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
		RenderPassInfo.pAttachments = Descriptions.data();
		RenderPassInfo.attachmentCount = static_cast<uint32>(Descriptions.size());
		RenderPassInfo.subpassCount = 1;
		RenderPassInfo.pSubpasses = &Subpass;
		RenderPassInfo.pDependencies = Dependencies.data();
		RenderPassInfo.dependencyCount = static_cast<uint32>(Dependencies.size());

		VkRenderPass RenderPass;
		vulkan(vkCreateRenderPass(Device, &RenderPassInfo, nullptr, &RenderPass));
		RenderPasses.Push(RenderPass);
	}

	const uint32 NumRTs = Pending.RPInit.NumRenderTargets;
	std::vector<VkClearValue> ClearValues;
	std::vector<VkImageView> AttachmentViews;

	if (Pending.RPInit.DepthTarget)
	{
		ClearValues.resize(NumRTs + 1);
		AttachmentViews.resize(NumRTs + 1);
	}
	else
	{
		ClearValues.resize(NumRTs);
		AttachmentViews.resize(NumRTs);
	}

	for (uint32 i = 0; i < NumRTs; i++)
	{
		VulkanRenderTargetViewRef ColorTarget = ResourceCast(Pending.RPInit.ColorTargets[i]);
		VulkanImageRef Image = ResourceCast(ColorTarget->Image);

		const auto& ClearValue = std::get<std::array<float, 4>>(ColorTarget->ClearValue);
		ClearValues[i].color.float32[0] = ClearValue[0];
		ClearValues[i].color.float32[1] = ClearValue[1];
		ClearValues[i].color.float32[2] = ClearValue[2];
		ClearValues[i].color.float32[3] = ClearValue[3];

		AttachmentViews[i] = Image->ImageView;
	}

	if (Pending.RPInit.DepthTarget)
	{
		VulkanRenderTargetViewRef DepthTarget = ResourceCast(Pending.RPInit.DepthTarget);
		VulkanImageRef Image = ResourceCast(DepthTarget->Image);

		ClearValues[NumRTs].depthStencil = { 0, 0 };

		if (Image->IsDepth())
		{
			ClearValues[NumRTs].depthStencil.depth = std::get<ClearDepthStencilValue>(DepthTarget->ClearValue).DepthClear;
		}

		if (Image->IsStencil())
		{
			ClearValues[NumRTs].depthStencil.stencil = std::get<ClearDepthStencilValue>(DepthTarget->ClearValue).StencilClear;
		}

		AttachmentViews[NumRTs] = Image->ImageView;
	}

	drm::ImageRef Image = Pending.RPInit.DepthTarget ? Pending.RPInit.DepthTarget->Image : Pending.RPInit.ColorTargets[0]->Image;

	VkRect2D RenderArea = {};
	RenderArea.extent = { Image->Width, Image->Height };
	RenderArea.offset = {};

	VkFramebufferCreateInfo FramebufferInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	FramebufferInfo.renderPass = RenderPasses.Get();
	FramebufferInfo.pAttachments = AttachmentViews.data();
	FramebufferInfo.attachmentCount = AttachmentViews.size();
	FramebufferInfo.width = RenderArea.extent.width;
	FramebufferInfo.height = RenderArea.extent.height;
	FramebufferInfo.layers = 1;

	VkFramebuffer Framebuffer;
	vulkan(vkCreateFramebuffer(Device, &FramebufferInfo, nullptr, &Framebuffer));
	Framebuffers.Push(Framebuffer);

	VkRenderPassBeginInfo BeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	BeginInfo.renderPass = RenderPasses.Get();
	BeginInfo.framebuffer = Framebuffers.Get();
	BeginInfo.renderArea = RenderArea;
	BeginInfo.pClearValues = ClearValues.data();
	BeginInfo.clearValueCount = ClearValues.size();

	vkCmdBeginRenderPass(CommandBuffer, &BeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	bDirtyRenderPass = false;
	bDirtyPipeline = true;
}

void VulkanCommandList::CleanPipeline()
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

		const auto& In = Pending.PSOInit.DepthStencilState;

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

		const auto& In = Pending.PSOInit.RasterizationState;

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
		const auto& In = Pending.PSOInit.MultisampleState;

		MultisampleState.rasterizationSamples = (VkSampleCountFlagBits)In.RasterizationSamples;
		MultisampleState.sampleShadingEnable = In.SampleShadingEnable;
		MultisampleState.minSampleShading = In.MinSampleShading;
		MultisampleState.pSampleMask = In.SampleMask;
		MultisampleState.alphaToCoverageEnable = In.AlphaToCoverageEnable;
		MultisampleState.alphaToOneEnable = In.AlphaToOneEnable;
	}

	std::array<VkPipelineColorBlendAttachmentState, RenderPassInitializer::MaxSimultaneousRenderTargets> ColorBlendAttachmentStates;

	{
		for (uint32 RenderTargetIndex = 0; RenderTargetIndex < Pending.RPInit.NumRenderTargets; RenderTargetIndex++)
		{
			const auto& In = Pending.PSOInit.ColorBlendAttachmentStates[RenderTargetIndex];
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

	const auto& GraphicsPipeline = Pending.PSOInit.GraphicsPipelineState;

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
		const VulkanShader& VulkanShader = Device.ShaderCache[Shader->Type];
		VkPipelineShaderStageCreateInfo& ShaderStage = ShaderStages[i];
		ShaderStage.stage = VulkanShader::GetVulkanStage(Shader->Stage);
		ShaderStage.module = VulkanShader.ShaderModule;
		ShaderStage.pName = Shader->Entrypoint.data();
	}

	VkPipelineVertexInputStateCreateInfo VertexInputState = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	const std::vector<VkVertexInputAttributeDescription>& AttributeDescriptions = Device.ShaderCache[GraphicsPipeline.Vertex->Type].Attributes;
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
		const auto& In = Pending.PSOInit.InputAssemblyState;
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
		const auto& In = Pending.PSOInit.Viewport;
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
	ColorBlendState.attachmentCount = Pending.RPInit.NumRenderTargets;

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
	PipelineInfo.layout = PipelineLayouts.Get();
	PipelineInfo.renderPass = RenderPasses.Get();
	PipelineInfo.subpass = 0;
	PipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	VkPipeline Pipeline;
	vulkan(vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &Pipeline));
	Pipelines.Push(Pipeline);

	vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipelines.Get());

	bDirtyPipeline = false;
}