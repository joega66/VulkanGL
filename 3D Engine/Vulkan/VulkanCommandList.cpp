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
	Samplers.Destroy();

	vkFreeCommandBuffers(Device, Device.CommandPool, 1, &CommandBuffer);
}

void VulkanCommandList::BeginRenderPass(const RenderPassInitializer& RPInit)
{
	// Determine if this command list touched the surface.
	for (uint32_t ColorTargetIndex = 0; ColorTargetIndex < RPInit.NumRenderTargets; ColorTargetIndex++)
	{
		if (RPInit.ColorTargets[ColorTargetIndex]->Image == drm::GetSurface())
		{
			bTouchedSurface = true;
		}
	}

	std::tie(Pending.RenderPass, Pending.Framebuffer) = Device.GetRenderPass(RPInit);

	const uint32 NumRTs = RPInit.NumRenderTargets;
	std::vector<VkClearValue> ClearValues;

	if (RPInit.DepthTarget)
	{
		ClearValues.resize(NumRTs + 1);
	}
	else
	{
		ClearValues.resize(NumRTs);
	}

	for (uint32 ColorTargetIndex = 0; ColorTargetIndex < NumRTs; ColorTargetIndex++)
	{
		const auto& ClearValue = std::get<ClearColorValue>(RPInit.ColorTargets[ColorTargetIndex]->ClearValue);
		memcpy(ClearValues[ColorTargetIndex].color.float32, ClearValue.Float32, sizeof(ClearValue.Float32));
		memcpy(ClearValues[ColorTargetIndex].color.int32, ClearValue.Int32, sizeof(ClearValue.Int32));
		memcpy(ClearValues[ColorTargetIndex].color.uint32, ClearValue.Uint32, sizeof(ClearValue.Uint32));
	}

	if (RPInit.DepthTarget)
	{
		VulkanRenderTargetViewRef DepthTarget = ResourceCast(RPInit.DepthTarget);
		VulkanImageRef Image = ResourceCast(DepthTarget->Image);

		ClearValues[NumRTs].depthStencil = { 0, 0 };

		if (Image->IsDepth())
		{
			ClearValues[NumRTs].depthStencil.depth = std::get<ClearDepthStencilValue>(RPInit.DepthTarget->ClearValue).DepthClear;
		}

		if (Image->IsStencil())
		{
			ClearValues[NumRTs].depthStencil.stencil = std::get<ClearDepthStencilValue>(RPInit.DepthTarget->ClearValue).StencilClear;
		}
	}

	VkRect2D RenderArea;
	RenderArea.offset.x = RPInit.RenderArea.Offset.x;
	RenderArea.offset.y = RPInit.RenderArea.Offset.y;
	RenderArea.extent.width = RPInit.RenderArea.Extent.x;
	RenderArea.extent.height = RPInit.RenderArea.Extent.y;

	VkRenderPassBeginInfo BeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	BeginInfo.renderPass = Pending.RenderPass;
	BeginInfo.framebuffer = Pending.Framebuffer;
	BeginInfo.renderArea = RenderArea;
	BeginInfo.pClearValues = ClearValues.data();
	BeginInfo.clearValueCount = ClearValues.size();

	vkCmdBeginRenderPass(CommandBuffer, &BeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	Pending.NumRenderTargets = RPInit.NumRenderTargets;
}

void VulkanCommandList::EndRenderPass()
{
	vkCmdEndRenderPass(CommandBuffer);
}

void VulkanCommandList::BindPipeline(const PipelineStateInitializer& PSOInit)
{
	if (Pending.GraphicsPipelineState != PSOInit.GraphicsPipelineState)
	{
		// Clear descriptors and vertex streams.
		DescriptorImages.clear();
		DescriptorBuffers.clear();
		std::fill(Pending.VertexStreams.begin(), Pending.VertexStreams.end(), VulkanVertexBufferRef());
		Pending.GraphicsPipelineState = PSOInit.GraphicsPipelineState;
	}

	std::tie(Pending.Pipeline, Pending.PipelineLayout, Pending.DescriptorSetLayout) = Device.GetPipeline(PSOInit, Pending.RenderPass, Pending.NumRenderTargets);

	vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pending.Pipeline);
}

void VulkanCommandList::BindVertexBuffers(uint32 Location, drm::VertexBufferRef VertexBuffer)
{
	check(Location < Device.Properties.limits.maxVertexInputBindings, "Invalid location.");

	VulkanVertexBufferRef VulkanVertexBuffer = ResourceCast(VertexBuffer);

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

	for (const auto& Binding : Bindings)
	{
		if (Binding.binding == Location)
		{
			check(Binding.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, "Shader resource at this location isn't a combined image sampler.");

			VkSampler VulkanSampler = VulkanImage::CreateSampler(Device, Sampler);
			Samplers.Push(VulkanSampler);

			VkDescriptorImageInfo DescriptorImageInfo = { VulkanSampler, VulkanImage->ImageView, VulkanImage->GetVulkanLayout() };
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
	const auto& Bindings = VulkanShader.Bindings;
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
	const VkIndexType IndexType = VulkanIndexBuffer->IndexStride == sizeof(uint32) ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16;

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
	vulkan(vkEndCommandBuffer(CommandBuffer));
	bFinished = true;
}

void VulkanCommandList::CleanDescriptorSets()
{
	VkDescriptorSet DescriptorSet = DescriptorPool.Spawn(Pending.DescriptorSetLayout);
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
	vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pending.PipelineLayout, 0, 1, &DescriptorSet, 0, nullptr);

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
	if (bDirtyDescriptorSets)
	{
		CleanDescriptorSets();
	}

	if (bDirtyVertexStreams)
	{
		CleanVertexStreams();
	}
}