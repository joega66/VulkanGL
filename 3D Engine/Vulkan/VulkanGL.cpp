#include "VulkanGL.h"
#include "VulkanRenderTargetView.h"
#include "VulkanCommands.h"
#include <Engine/Timers.h>

CAST(GLImage, VulkanImage);
CAST(GLShader, VulkanShader);
CAST(GLVertexBuffer, VulkanVertexBuffer);
CAST(GLUniformBuffer, VulkanUniformBuffer);
CAST(GLStorageBuffer, VulkanStorageBuffer);
CAST(GLIndexBuffer, VulkanIndexBuffer);

VulkanCommandList::VulkanCommandList()
	: Swapchain(Device)
	, Allocator(Device)
	, DescriptorPool(Device)
	, Pending(Device)
	, RenderPasses([&] (VkRenderPass RenderPass) { vkDestroyRenderPass(Device, RenderPass, nullptr); })
	, Framebuffers([&] (VkFramebuffer Framebuffer) { vkDestroyFramebuffer(Device, Framebuffer, nullptr); })
	, DescriptorSetLayouts([&] (VkDescriptorSetLayout DescriptorSetLayout) { vkDestroyDescriptorSetLayout(Device, DescriptorSetLayout, nullptr); })
	, PipelineLayouts([&] (VkPipelineLayout PipelineLayout) { vkDestroyPipelineLayout(Device, PipelineLayout, nullptr); })
	, Pipelines([&] (VkPipeline Pipeline) { vkDestroyPipeline(Device, Pipeline, nullptr); })
	, Samplers([&] (VkSampler Sampler) { vkDestroySampler(Device, Sampler, nullptr); })
	, ImageFormatToGLSLSize([&](){HashTable<EImageFormat,uint32>Result;for(auto&[GLSLType,Format]:GLSLTypeToVulkanFormat){
		auto ImageFormat=VulkanImage::GetEngineFormat(Format);auto GLSLSize=GetValue(GLSLTypeSizes,GLSLType); Result[ImageFormat]=GLSLSize;}return Result;}())
	, SizeOfVulkanFormat([&](){HashTable<VkFormat,uint32>Result;for(auto&[GLSLType,Format]:GLSLTypeToVulkanFormat){auto SizeOf=GetValue(GLSLTypeSizes,GLSLType);
		Result[Format]=SizeOf;}return Result;}())
{
}

void VulkanCommandList::Init()
{
	Swapchain.InitSwapchain();

	CommandBuffers.resize(Swapchain.Images.size());

	VkCommandBufferAllocateInfo CommandBufferInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	CommandBufferInfo.commandPool = Device.CommandPool;
	CommandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	CommandBufferInfo.pNext = nullptr;
	CommandBufferInfo.commandBufferCount = static_cast<uint32>(CommandBuffers.size());

	vkAllocateCommandBuffers(Device, &CommandBufferInfo, CommandBuffers.data());

	VkSemaphoreCreateInfo SemaphoreInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	vulkan(vkCreateSemaphore(Device, &SemaphoreInfo, nullptr, &ImageAvailableSem));
	vulkan(vkCreateSemaphore(Device, &SemaphoreInfo, nullptr, &RenderEndSem));
}

void VulkanCommandList::Release()
{
	vkDestroySemaphore(Device, RenderEndSem, nullptr);
	vkDestroySemaphore(Device, ImageAvailableSem, nullptr);
	vkFreeCommandBuffers(Device, Device.CommandPool, CommandBuffers.size(), CommandBuffers.data());
}

void VulkanCommandList::BeginFrame()
{
	if (VkResult Result = vkAcquireNextImageKHR(Device, 
		Swapchain, 
		std::numeric_limits<uint32>::max(), 
		ImageAvailableSem, 
		VK_NULL_HANDLE, 
		&SwapchainIndex); Result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		signal_unimplemented();
	}
	else
	{
		vulkan(Result);
		check(SwapchainIndex >= 0 && SwapchainIndex < Swapchain.Images.size(), "Error acquiring swapchain.");
	}

	vulkan(vkResetCommandBuffer(GetCommandBuffer(), 0));

	VkCommandBufferBeginInfo BeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	vulkan(vkBeginCommandBuffer(GetCommandBuffer(), &BeginInfo));
}

void VulkanCommandList::EndFrame()
{
	if (RenderPasses.Size())
	{
		vkCmdEndRenderPass(GetCommandBuffer());
	}

	vulkan(vkEndCommandBuffer(GetCommandBuffer()));

	VkSubmitInfo SubmitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	SubmitInfo.pWaitSemaphores = &ImageAvailableSem;
	SubmitInfo.waitSemaphoreCount = 1;
	SubmitInfo.pCommandBuffers = &GetCommandBuffer();
	SubmitInfo.commandBufferCount = 1;
	SubmitInfo.pSignalSemaphores = &RenderEndSem;
	SubmitInfo.signalSemaphoreCount = 1;

	vulkan(vkQueueSubmit(Device.GraphicsQueue, 1, &SubmitInfo, VK_NULL_HANDLE));

	VkPresentInfoKHR PresentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	PresentInfo.pWaitSemaphores = &RenderEndSem;
	PresentInfo.waitSemaphoreCount = 1;
	PresentInfo.pSwapchains = &Swapchain.Swapchain;
	PresentInfo.swapchainCount = 1;
	PresentInfo.pImageIndices = &SwapchainIndex;

	if (VkResult Result = vkQueuePresentKHR(Device.PresentQueue, &PresentInfo); Result == VK_ERROR_OUT_OF_DATE_KHR || Result == VK_SUBOPTIMAL_KHR)
	{
		signal_unimplemented();
	}
	else
	{
		vulkan(Result);
	}

	vkQueueWaitIdle(Device.PresentQueue);

	RenderPasses.Destroy();
	Framebuffers.Destroy();
	DescriptorSetLayouts.Destroy();
	PipelineLayouts.Destroy();
	Pipelines.Destroy();
	Samplers.Destroy();

	DescriptorImages.clear();
	DescriptorBuffers.clear();
	DescriptorPool.Reset();

	// "Transition" render targets back to UNDEFINED
	// This is mainly for consistency with non-rendertargetable images and error checking
	for (uint32 i = 0; i < Pending.NumRTs; i++)
	{
		VulkanImageRef Image = ResourceCast(Pending.ColorTargets[i]->Image);
		Image->Layout = VK_IMAGE_LAYOUT_UNDEFINED;
	}

	if (Pending.DepthTarget)
	{
		VulkanImageRef Image = ResourceCast(Pending.DepthTarget->Image);
		Image->Layout = VK_IMAGE_LAYOUT_UNDEFINED;
		Pending.DepthTarget = nullptr;
	}

	Pending.SetDefaultPipeline(Device);
}

void VulkanCommandList::SetRenderTargets(uint32 NumRTs, const GLRenderTargetViewRef* ColorTargets, const GLRenderTargetViewRef DepthTarget, EDepthStencilAccess Access)
{
	// @todo Pretty heavyweight function... Should just defer the creation of this stuff until draw.
	// @todo Check if the render pass is the same. 
	check(NumRTs < MaxSimultaneousRenderTargets, "Trying to set too many render targets.");

	Pending.NumRTs = NumRTs;
	Pending.DepthTarget = ResourceCast(DepthTarget);

	std::vector<VkAttachmentDescription> Descriptions;
	std::vector<VkAttachmentReference> ColorRefs(NumRTs);
	VkAttachmentReference DepthRef = {};

	if (DepthTarget)
	{
		Descriptions.resize(NumRTs + 1);
	}
	else
	{
		Descriptions.resize(NumRTs);
	}

	for (uint32 i = 0; i < NumRTs; i++)
	{
		check(ColorTargets[i], "Color target is null.");

		VulkanRenderTargetViewRef ColorTarget = ResourceCast(ColorTargets[i]);
		Pending.ColorTargets[i] = ColorTarget;
		ColorTarget = ResourceCast(ColorTargets[i]);
		VulkanImageRef Image = ResourceCast(ColorTarget->Image);

		check(Image && Any(Image->Usage & EResourceUsage::RenderTargetable), "Color target is invalid.");
		check(Image->IsColor(), "Color target was not created in color format.");

		Image->Layout = [&] ()
		{
			if (ColorTarget == GetCurrentSwapchainRTView())
			{
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

	VkPipelineDepthStencilStateCreateInfo& DepthStencil = Pending.DepthStencilState;

	if (Access == EDepthStencilAccess::None)
	{
		DepthStencil.depthWriteEnable = false;
	}
	else
	{
		VulkanRenderTargetViewRef DepthTarget = Pending.DepthTarget;
		check(DepthTarget, "Depth target is invalid.");

		VulkanImageRef DepthImage = ResourceCast(DepthTarget->Image);
		check(DepthImage && Any(DepthImage->Usage & EResourceUsage::RenderTargetable), "Depth target is invalid.");
		check(DepthImage->IsDepth() || DepthImage->IsStencil(), "Depth target was not created in a depth layout.");

		VkImageLayout FinalLayout = [&] ()
		{
			if (Access == EDepthStencilAccess::DepthReadStencilRead)
			{
				check(Any(DepthImage->Usage & EResourceUsage::ShaderResource), "Depth Image must be created with EResourceUsage::ShaderResource.");
				DepthStencil.depthWriteEnable = true;
				return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			}
			else if (Access == EDepthStencilAccess::DepthReadStencilWrite)
			{
				check(Any(DepthImage->Usage & EResourceUsage::ShaderResource), "Depth Image must be created with EResourceUsage::ShaderResource.");
				DepthStencil.depthWriteEnable = true;
				return VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
			}
			else if (Access == EDepthStencilAccess::DepthWriteStencilRead)
			{
				check(Any(DepthImage->Usage & EResourceUsage::ShaderResource), "Depth Image must be created with EResourceUsage::ShaderResource.");
				DepthStencil.depthWriteEnable = true;
				return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
			}
			else
			{
				if (Access == EDepthStencilAccess::DepthWrite)
				{
					DepthStencil.depthWriteEnable = true;
				}
				else if (Access == EDepthStencilAccess::StencilWrite)
				{
					DepthStencil.depthWriteEnable = false;
				}
				else // DepthWriteStencilWrite
				{
					DepthStencil.depthWriteEnable = true;
				}

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
		Descriptions[NumRTs] = DepthDescription;

		DepthRef.attachment = NumRTs;
		DepthRef.layout = FinalLayout;

		DepthImage->Layout = FinalLayout;
	}

	VkSubpassDescription Subpass = {};
	Subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	Subpass.pColorAttachments = ColorRefs.data();
	Subpass.colorAttachmentCount = static_cast<uint32>(ColorRefs.size());
	Subpass.pDepthStencilAttachment = !DepthTarget ? nullptr : &DepthRef;
	
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

	bDirtyRenderPass = true;
}

void VulkanCommandList::SetGraphicsPipeline(GLShaderRef Vertex, GLShaderRef TessControl, GLShaderRef TessEval, GLShaderRef Geometry, GLShaderRef Fragment)
{
	if (Pending.Vertex == Vertex &&
		Pending.TessControl == TessControl &&
		Pending.TessEval == TessEval &&
		Pending.Geometry == Geometry &&
		Pending.Fragment == Fragment)
		return;

	DescriptorImages.clear();
	DescriptorBuffers.clear();

	Pending.ResetVertexStreams();

	Pending.Vertex = ResourceCast(Vertex);
	Pending.TessControl = ResourceCast(TessControl);
	Pending.TessEval = ResourceCast(TessEval);
	Pending.Geometry = ResourceCast(Geometry);
	Pending.Fragment = ResourceCast(Fragment);

	bDirtyPipelineLayout = true;
}

void VulkanCommandList::SetVertexStream(uint32 Location, GLVertexBufferRef VertexBuffer)
{
	check(Location < Device.Properties.limits.maxVertexInputBindings, "Invalid location.");

	VulkanVertexBufferRef VulkanVertexBuffer = ResourceCast(VertexBuffer);
	check(VulkanVertexBuffer, "Invalid vertex buffer.");

	Pending.VertexStreams[Location] = VulkanVertexBuffer;

	bDirtyVertexStreams = true;
}

GLVertexBufferRef VulkanCommandList::CreateVertexBuffer(EImageFormat EngineFormat, uint32 NumElements, EResourceUsage Usage, const void* Data)
{
	uint32 GLSLSize = GetValue(ImageFormatToGLSLSize, EngineFormat);
	auto Buffer = Allocator.CreateBuffer(NumElements * GLSLSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, Usage, Data);
	return MakeRef<VulkanVertexBuffer>(Buffer, EngineFormat, Usage);
}

void VulkanCommandList::SetUniformBuffer(GLShaderRef Shader, uint32 Location, GLUniformBufferRef UniformBuffer)
{
	VulkanShaderRef VulkanShader = ResourceCast(Shader);
	VulkanUniformBufferRef VulkanUniformBuffer = ResourceCast(UniformBuffer);
	auto& Bindings = VulkanShader->Bindings;
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
			DescriptorBuffers[VulkanShader->Meta.Stage][Location] = std::make_unique<VulkanWriteDescriptorBuffer>(Binding, BufferInfo);
			bDirtyDescriptorSets = true;

			return;
		}
	}

	fail("A shader resource doesn't exist at this location.\nShader: %s, Location: %d", VulkanShader->Meta.EntryPoint.c_str(), Location);
}

void VulkanCommandList::SetShaderImage(GLShaderRef Shader, uint32 Location, GLImageRef Image, const SamplerState& Sampler)
{
	VulkanShaderRef VulkanShader = ResourceCast(Shader);
	VulkanImageRef VulkanImage = ResourceCast(Image);
	auto& Bindings = VulkanShader->Bindings;

	check(VulkanImage->Layout != VK_IMAGE_LAYOUT_UNDEFINED, "Invalid Vulkan image layout for shader read.");

	for (const auto& Binding : Bindings)
	{
		if (Binding.binding == Location)
		{
			check(Binding.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, "Shader resource at this location isn't a combined image sampler.");

			VkSampler VulkanSampler = CreateSampler(Sampler);
			Samplers.Push(VulkanSampler);

			VkDescriptorImageInfo DescriptorImageInfo = { VulkanSampler, VulkanImage->ImageView, VulkanImage->Layout };
			DescriptorImages[VulkanShader->Meta.Stage][Location] = std::make_unique<VulkanWriteDescriptorImage>(Binding, DescriptorImageInfo);
			bDirtyDescriptorSets = true;

			return;
		}
	}

	fail("A shader resource doesn't exist at this location.\nShader: %s, Location: %d", VulkanShader->Meta.EntryPoint.c_str(), Location);
}

void VulkanCommandList::SetStorageBuffer(GLShaderRef Shader, uint32 Location, GLStorageBufferRef StorageBuffer)
{
	VulkanShaderRef VulkanShader = ResourceCast(Shader);
	VulkanStorageBufferRef VulkanStorageBuffer = ResourceCast(StorageBuffer);
	auto& Bindings = VulkanShader->Bindings;
	auto SharedBuffer = VulkanStorageBuffer->Buffer;

	check(SharedBuffer->Shared->Usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, "Invalid buffer type.");

	for (const auto& Binding : Bindings)
	{
		if (Binding.binding == Location)
		{
			check(Binding.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, "Shader resource at this location isn't a storage buffer.");

			VkDescriptorBufferInfo BufferInfo = { SharedBuffer->GetVulkanHandle(), SharedBuffer->Offset, SharedBuffer->Size };
			DescriptorBuffers[VulkanShader->Meta.Stage][Location] = std::make_unique<VulkanWriteDescriptorBuffer>(Binding, BufferInfo);
			bDirtyDescriptorSets = true;

			return;
		}
	}

	fail("A shader resource doesn't exist at this location.\nShader: %s, Location: %d", VulkanShader->Meta.EntryPoint.c_str(), Location);
}

void VulkanCommandList::DrawIndexed(GLIndexBufferRef IndexBuffer, uint32 IndexCount, uint32 InstanceCount, uint32 FirstIndex, uint32 VertexOffset, uint32 FirstInstance)
{
	PrepareForDraw();

	VulkanIndexBufferRef VulkanIndexBuffer = ResourceCast(IndexBuffer);
	VkIndexType IndexType = VulkanIndexBuffer->IndexStride == sizeof(uint32) ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16;

	vkCmdBindIndexBuffer(GetCommandBuffer(), VulkanIndexBuffer->Buffer->GetVulkanHandle(), VulkanIndexBuffer->Buffer->Offset, IndexType);
	vkCmdDrawIndexed(GetCommandBuffer(), IndexCount, InstanceCount, FirstIndex, VertexOffset, FirstInstance);
}

void VulkanCommandList::Draw(uint32 VertexCount, uint32 InstanceCount, uint32 FirstVertex, uint32 FirstInstance)
{
	PrepareForDraw();

	vkCmdDraw(GetCommandBuffer(), VertexCount, InstanceCount, FirstVertex, FirstInstance);
}

GLIndexBufferRef VulkanCommandList::CreateIndexBuffer(EImageFormat Format, uint32 NumIndices, EResourceUsage Usage, const void * Data)
{
	check(Format == IF_R16_UINT || Format == IF_R32_UINT, "Format must be single-channel unsigned type.");

	uint32 IndexBufferStride = GetValue(ImageFormatToGLSLSize, Format);
	auto Buffer = Allocator.CreateBuffer(IndexBufferStride * NumIndices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, Usage, Data);
	return MakeRef<VulkanIndexBuffer>(Buffer, IndexBufferStride, Format, Usage);
}

GLUniformBufferRef VulkanCommandList::CreateUniformBuffer(uint32 Size, const void* Data, EUniformUpdate UniformUsage)
{
	EResourceUsage Usage = UniformUsage == EUniformUpdate::Frequent || UniformUsage == EUniformUpdate::SingleFrame 
		? EResourceUsage::KeepCPUAccessible : EResourceUsage::None;
	auto Buffer = Allocator.CreateBuffer(Size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, Usage, Data);
	return MakeRef<VulkanUniformBuffer>(Buffer);
}

GLStorageBufferRef VulkanCommandList::CreateStorageBuffer(uint32 Size, const void * Data, EResourceUsage Usage)
{
	auto Buffer = Allocator.CreateBuffer(Size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, Usage, Data);
	return MakeRef<VulkanStorageBuffer>(Buffer, Usage);
}

GLImageRef VulkanCommandList::CreateImage(uint32 Width, uint32 Height, EImageFormat Format, EResourceUsage UsageFlags, const uint8* Data = nullptr)
{
	VkImage Image;
	VkDeviceMemory Memory;
	VkImageLayout Layout;

	CreateImage(Image, Memory, Layout, Width, Height, Format, UsageFlags, Data);

	VulkanImageRef GLImage = MakeRef<VulkanImage>(Device
		, Image
		, Memory
		, Layout
		, Format
		, Width
		, Height
		, UsageFlags);

	if (Data)
	{
		TransitionImageLayout(GLImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT);
		Allocator.UploadImageData(GLImage, Data);
		TransitionImageLayout(GLImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
	}

	return GLImage;
}

GLImageRef VulkanCommandList::CreateCubemap(uint32 Width, uint32 Height, EImageFormat Format, EResourceUsage UsageFlags, const CubemapCreateInfo& CubemapCreateInfo)
{
	// This path will be supported, but should really prefer to use a compressed format.
	VkImage Image;
	VkDeviceMemory Memory;
	VkImageLayout Layout;

	bool bHasData = std::find_if(CubemapCreateInfo.CubeFaces.begin(), CubemapCreateInfo.CubeFaces.end(),
		[](const auto& TextureInfo) { return TextureInfo.Data; })
		!= CubemapCreateInfo.CubeFaces.end();

	CreateImage(Image, Memory, Layout, Width, Height, Format, UsageFlags, bHasData);

	VulkanImageRef GLImage = MakeRef<VulkanImage>(Device
		, Image
		, Memory
		, Layout
		, Format
		, Width
		, Height
		, UsageFlags);

	if (bHasData)
	{
		TransitionImageLayout(GLImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT);
		Allocator.UploadCubemapData(GLImage, CubemapCreateInfo);
		TransitionImageLayout(GLImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
	}

	return GLImage;
}

GLRenderTargetViewRef VulkanCommandList::CreateRenderTargetView(GLImageRef Image, ELoadAction LoadAction, EStoreAction StoreAction, const std::array<float, 4>& ClearValue)
{
	VulkanImageRef VulkanImage = ResourceCast(Image);
	VulkanRenderTargetViewRef RTView = MakeRef<VulkanRenderTargetView>(
		Device,
		VulkanImage,
		LoadAction,
		StoreAction,
		ClearValue);
	return RTView;
}

GLRenderTargetViewRef VulkanCommandList::CreateRenderTargetView(GLImageRef Image, ELoadAction LoadAction, EStoreAction StoreAction, const ClearDepthStencilValue& DepthStencil)
{
	VulkanImageRef VulkanImage = ResourceCast(Image);
	VulkanRenderTargetViewRef RTView = MakeRef<VulkanRenderTargetView>(
		Device,
		VulkanImage,
		LoadAction,
		StoreAction,
		DepthStencil);
	return RTView;
}

void VulkanCommandList::SetPipelineState(const PipelineStateInitializer& PSOInit)
{
	{
		const Viewport& In = PSOInit.Viewport;
		VkViewport& Out = Pending.Viewport;
		Out.x = In.X;
		Out.y = In.Y;
		Out.width = In.Width;
		Out.height = In.Height;
		Out.minDepth = In.MinDepth;
		Out.maxDepth = In.MaxDepth;
	}

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

		const DepthStencilState& In = PSOInit.DepthStencilState;
		VkPipelineDepthStencilStateCreateInfo& Out = Pending.DepthStencilState;

		Out.depthTestEnable = In.DepthTestEnable;
		Out.depthCompareOp = GetValue(VulkanDepthCompare, In.DepthCompareTest);
		Out.stencilTestEnable = In.StencilTestEnable;

		const StencilOpState& Back = In.Back;

		Out.back.failOp = GetValue(VulkanStencilOp, Back.FailOp);
		Out.back.passOp = GetValue(VulkanStencilOp, Back.PassOp);
		Out.back.depthFailOp = GetValue(VulkanStencilOp, Back.DepthFailOp);
		Out.back.compareOp = GetValue(VulkanCompareOp, Back.CompareOp);
		Out.back.compareMask = Back.CompareMask;
		Out.back.writeMask = Back.WriteMask;
		Out.back.reference = Back.Reference;
		Out.front = Out.back;
	}

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

		const RasterizationState& In = PSOInit.RasterizationState;
		VkPipelineRasterizationStateCreateInfo& Out = Pending.RasterizationState;

		Out.depthClampEnable = In.DepthClampEnable;
		Out.rasterizerDiscardEnable = In.RasterizerDiscardEnable;
		Out.polygonMode = GetValue(VulkanPolygonMode, In.PolygonMode);
		Out.cullMode = GetValue(VulkanCullMode, In.CullMode);
		Out.frontFace = GetValue(VulkanFrontFace, In.FrontFace);
		Out.depthBiasEnable = In.DepthBiasEnable;
		Out.depthBiasConstantFactor = In.DepthBiasConstantFactor;
		Out.depthBiasClamp = In.DepthBiasClamp;
		Out.depthBiasSlopeFactor = In.DepthBiasSlopeFactor;
		Out.lineWidth = In.LineWidth;
	}

	{
		for (uint32 RenderTargetIndex = 0; RenderTargetIndex < MaxSimultaneousRenderTargets; RenderTargetIndex++)
		{
			const ColorBlendAttachmentState& In = PSOInit.ColorBlendAttachmentStates[RenderTargetIndex];
			VkPipelineColorBlendAttachmentState& Out = Pending.ColorBlendAttachmentStates[RenderTargetIndex];

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

	{
		const InputAssemblyState& In = PSOInit.InputAssemblyState;
		VkPipelineInputAssemblyStateCreateInfo& Out = Pending.InputAssemblyState;

		for (VkPrimitiveTopology VulkanTopology = VK_PRIMITIVE_TOPOLOGY_BEGIN_RANGE; VulkanTopology < VK_PRIMITIVE_TOPOLOGY_RANGE_SIZE;)
		{
			if ((uint32)In.Topology == VulkanTopology)
			{
				Out.topology = VulkanTopology;
				return;
			}
			VulkanTopology = VkPrimitiveTopology(VulkanTopology + 1);
		}

		Out.primitiveRestartEnable = In.PrimitiveRestartEnable;

		fail("VkPrimitiveTopology not found.");
	}
}

GLRenderTargetViewRef VulkanCommandList::GetSurfaceView(ELoadAction LoadAction, EStoreAction StoreAction, const std::array<float, 4>& ClearValue)
{
	VulkanRenderTargetViewRef SurfaceView = GetCurrentSwapchainRTView();
	SurfaceView->LoadAction = LoadAction;
	SurfaceView->StoreAction = StoreAction;
	SurfaceView->ClearValue = ClearValue;
	return SurfaceView;
}

void* VulkanCommandList::LockBuffer(GLVertexBufferRef VertexBuffer, uint32 Size, uint32 Offset)
{
	// @todo-joe Handle Size, Offset
	VulkanVertexBufferRef VulkanVertexBuffer = ResourceCast(VertexBuffer);
	return Allocator.LockBuffer(*VulkanVertexBuffer->Buffer);
}

void VulkanCommandList::UnlockBuffer(GLVertexBufferRef VertexBuffer)
{
	VulkanVertexBufferRef VulkanVertexBuffer = ResourceCast(VertexBuffer);
	Allocator.UnlockBuffer(*VulkanVertexBuffer->Buffer);
}

void* VulkanCommandList::LockBuffer(GLIndexBufferRef IndexBuffer, uint32 Size, uint32 Offset)
{
	// @todo-joe Handle Size, Offset
	VulkanIndexBufferRef VulkanIndexBuffer = ResourceCast(IndexBuffer);
	return Allocator.LockBuffer(*VulkanIndexBuffer->Buffer);
}

void VulkanCommandList::UnlockBuffer(GLIndexBufferRef IndexBuffer)
{
	VulkanIndexBufferRef VulkanIndexBuffer = ResourceCast(IndexBuffer);
	Allocator.UnlockBuffer(*VulkanIndexBuffer->Buffer);
}

void VulkanCommandList::RebuildResolutionDependents()
{
	// @todo-joe
}

VkCommandBuffer& VulkanCommandList::GetCommandBuffer()
{
	return CommandBuffers[SwapchainIndex];
}

VulkanRenderTargetViewRef VulkanCommandList::GetCurrentSwapchainRTView()
{
	return Swapchain.RTViews[SwapchainIndex];
}

void VulkanCommandList::CleanPipelineLayout()
{
	std::vector<VkDescriptorSetLayoutBinding> AllBindings;

	auto AddBindings = [&] (const VulkanShaderRef& Shader)
	{
		if (Shader)
		{
			const auto& Bindings = Shader->Bindings;
			if (Bindings.size() > 0)
			{
				AllBindings.insert(AllBindings.end(), Bindings.begin(), Bindings.end());
			}
		}
	};

	AddBindings(Pending.Vertex);
	AddBindings(Pending.TessControl);
	AddBindings(Pending.TessEval);
	AddBindings(Pending.Geometry);
	AddBindings(Pending.Fragment);

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
	vkCmdBindDescriptorSets(GetCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayouts.Get(), 0, 1, &DescriptorSet, 0, nullptr);

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

	vkCmdBindVertexBuffers(GetCommandBuffer(), 0, Buffers.size(), Buffers.data(), Offsets.data());

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
		vkCmdEndRenderPass(GetCommandBuffer());
	}

	const uint32 NumRTs = Pending.NumRTs;
	std::vector<VkClearValue> ClearValues;
	std::vector<VkImageView> AttachmentViews;

	if (Pending.DepthTarget)
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
		VulkanRenderTargetViewRef ColorTarget = Pending.ColorTargets[i];
		VulkanImageRef Image = ResourceCast(ColorTarget->Image);

		const auto& ClearValue = std::get<std::array<float, 4>>(ColorTarget->ClearValue);
		ClearValues[i].color.float32[0] = ClearValue[0];
		ClearValues[i].color.float32[1] = ClearValue[1];
		ClearValues[i].color.float32[2] = ClearValue[2];
		ClearValues[i].color.float32[3] = ClearValue[3];

		AttachmentViews[i] = Image->ImageView;
	}

	if (Pending.DepthTarget)
	{
		VulkanRenderTargetViewRef DepthTarget = Pending.DepthTarget;
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

	GLImageRef Image = Pending.DepthTarget ? Pending.DepthTarget->Image : Pending.ColorTargets[0]->Image;

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

	vkCmdBeginRenderPass(GetCommandBuffer(), &BeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	bDirtyRenderPass = false;
	bDirtyPipeline = true;
}

void VulkanCommandList::CleanPipeline()
{
	check(Pending.Vertex, "No vertex shader bound...");

	std::vector<VulkanShaderRef> Shaders;

	Shaders.push_back(Pending.Vertex);

	if (Pending.TessControl)
	{
		Shaders.push_back(Pending.TessControl);
	}
	if (Pending.TessEval)
	{
		Shaders.push_back(Pending.TessEval);
	}
	if (Pending.Geometry)
	{
		Shaders.push_back(Pending.Geometry);
	}
	if (Pending.Fragment)
	{
		Shaders.push_back(Pending.Fragment);
	}

	std::vector<VkPipelineShaderStageCreateInfo> ShaderStages(Shaders.size(), { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO });

	for (uint32 i = 0; i < ShaderStages.size(); i++)
	{
		VulkanShaderRef Shader = Shaders[i];
		VkPipelineShaderStageCreateInfo& ShaderStage = ShaderStages[i];
		ShaderStage.stage = Shader->GetVulkanStage();
		ShaderStage.module = Shader->ShaderModule;
		ShaderStage.pName = Shader->Meta.EntryPoint.data();
	}

	VkPipelineVertexInputStateCreateInfo& VertexInputState = Pending.VertexInputState;
	const std::vector<VkVertexInputAttributeDescription>& AttributeDescriptions = Pending.Vertex->Attributes;
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

	VkPipelineInputAssemblyStateCreateInfo& InputAssemblyState = Pending.InputAssemblyState;

	VkViewport& Viewport = Pending.Viewport;

	VkRect2D& Scissor = Pending.Scissor;
	Scissor.extent.width = (uint32)Viewport.width;
	Scissor.extent.height = (uint32)Viewport.height;
	Scissor.offset = { 0, 0 };

	VkPipelineViewportStateCreateInfo& ViewportState = Pending.ViewportState;
	ViewportState.pViewports = &Viewport;
	ViewportState.viewportCount = 1;
	ViewportState.pScissors = &Scissor;
	ViewportState.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo& RasterizationState = Pending.RasterizationState;
	VkPipelineMultisampleStateCreateInfo& MultisampleState = Pending.MultisampleState;
	VkPipelineDepthStencilStateCreateInfo& DepthStencilState = Pending.DepthStencilState;

	VkPipelineColorBlendStateCreateInfo& ColorBlendState = Pending.ColorBlendState;
	ColorBlendState.pAttachments = Pending.ColorBlendAttachmentStates.data();
	ColorBlendState.attachmentCount = Pending.NumRTs;

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

	VkPipeline GraphicsPipeline;
	vulkan(vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &GraphicsPipeline));
	Pipelines.Push(GraphicsPipeline);

	vkCmdBindPipeline(GetCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, Pipelines.Get());

	bDirtyPipeline = false;
}

void VulkanCommandList::TransitionImageLayout(VulkanImageRef Image, VkImageLayout NewLayout, VkPipelineStageFlags DestinationStage)
{
	VulkanScopedCommandBuffer ScopedCommandBuffer(Device);

	VkImageMemoryBarrier Barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	Barrier.oldLayout = Image->Layout;
	Barrier.newLayout = NewLayout;
	Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	Barrier.image = Image->Image;
	Barrier.subresourceRange.aspectMask = Image->GetVulkanAspect();
	Barrier.subresourceRange.baseMipLevel = 0;
	Barrier.subresourceRange.levelCount = 1;
	Barrier.subresourceRange.baseArrayLayer = 0;
	Barrier.subresourceRange.layerCount = Any(Image->Usage & EResourceUsage::Cubemap) ? 6 : 1;

	if (Image->Layout == VK_IMAGE_LAYOUT_UNDEFINED && NewLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		check(Image->Stage == VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, "Pipeline stage is invalid for this transition.");
		Barrier.srcAccessMask = 0;
		Barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	}
	else if (Image->Layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && NewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		check(Image->Stage == VK_PIPELINE_STAGE_TRANSFER_BIT, "Pipeline stage is invalid for this transition.");
		Barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		Barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	}
	else
	{
		fail("Unsupported layout transition.");
	}

	vkCmdPipelineBarrier(
		ScopedCommandBuffer,
		Image->Stage, DestinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &Barrier
	);

	Image->Layout = NewLayout;
	Image->Stage = DestinationStage;
}

void VulkanCommandList::CreateImage(VkImage& Image, VkDeviceMemory& Memory, VkImageLayout& Layout
	, uint32 Width, uint32 Height, EImageFormat& Format, EResourceUsage UsageFlags, bool bTransferDstBit)
{
	Layout = VK_IMAGE_LAYOUT_UNDEFINED;

	if (GLImage::IsDepth(Format))
	{
		Format = VulkanImage::GetEngineFormat(FindSupportedDepthFormat(Format));
	}

	VkImageCreateInfo Info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	Info.imageType = VK_IMAGE_TYPE_2D;
	Info.extent.width = Width;
	Info.extent.height = Height;
	Info.extent.depth = 1;
	Info.mipLevels = 1;
	Info.arrayLayers = Any(UsageFlags & EResourceUsage::Cubemap) ? 6 : 1;
	Info.format = VulkanImage::GetVulkanFormat(Format);
	Info.tiling = VK_IMAGE_TILING_OPTIMAL;
	Info.initialLayout = Layout;
	Info.usage = [&] ()
	{
		VkFlags Usage = 0;
		
		if (Any(UsageFlags & EResourceUsage::RenderTargetable))
		{
			Usage |= GLImage::IsDepth(Format) ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		}

		Usage |= bTransferDstBit ? VK_IMAGE_USAGE_TRANSFER_DST_BIT : 0;
		Usage |= Any(UsageFlags & EResourceUsage::ShaderResource) ? VK_IMAGE_USAGE_SAMPLED_BIT : 0;
		Usage |= Any(UsageFlags & EResourceUsage::UnorderedAccess) ? VK_IMAGE_USAGE_STORAGE_BIT : 0;

		return Usage;
	}();
	Info.flags = Any(UsageFlags & EResourceUsage::Cubemap) ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
	Info.samples = VK_SAMPLE_COUNT_1_BIT;
	Info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	vulkan(vkCreateImage(Device, &Info, nullptr, &Image));

	VkMemoryRequirements MemRequirements = {};
	vkGetImageMemoryRequirements(Device, Image, &MemRequirements);

	VkMemoryAllocateInfo MemInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	MemInfo.allocationSize = MemRequirements.size;
	MemInfo.memoryTypeIndex = Allocator.FindMemoryType(MemRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	vulkan(vkAllocateMemory(Device, &MemInfo, nullptr, &Memory));
	vulkan(vkBindImageMemory(Device, Image, Memory, 0));
}

VkSampler VulkanCommandList::CreateSampler(const SamplerState& SamplerState)
{
	static const VkFilter VulkanFilters[] =
	{
		VK_FILTER_NEAREST,
		VK_FILTER_LINEAR,
		VK_FILTER_CUBIC_IMG
	};

	static const VkSamplerMipmapMode VulkanMipmapModes[] =
	{
		VK_SAMPLER_MIPMAP_MODE_NEAREST,
		VK_SAMPLER_MIPMAP_MODE_LINEAR
	};

	static const VkSamplerAddressMode VulkanAddressModes[] =
	{
		VK_SAMPLER_ADDRESS_MODE_REPEAT,
		VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
		VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE
	};

	VkFilter Filter = VulkanFilters[(uint32)SamplerState.Filter];
	VkSamplerMipmapMode SMM = VulkanMipmapModes[(uint32)SamplerState.SMM];
	VkSamplerAddressMode SAM = VulkanAddressModes[(uint32)SamplerState.SAM];

	VkSamplerCreateInfo SamplerInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	SamplerInfo.magFilter = Filter;
	SamplerInfo.minFilter = Filter;
	SamplerInfo.mipmapMode = SMM;
	SamplerInfo.addressModeU = SAM;
	SamplerInfo.addressModeV = SAM;
	SamplerInfo.addressModeW = SAM;
	SamplerInfo.anisotropyEnable = Device.Features.samplerAnisotropy;
	SamplerInfo.maxAnisotropy = Device.Features.samplerAnisotropy ? Device.Properties.limits.maxSamplerAnisotropy : 1.0f;
	SamplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	SamplerInfo.unnormalizedCoordinates = VK_FALSE;
	SamplerInfo.compareEnable = VK_FALSE;
	SamplerInfo.compareOp = VK_COMPARE_OP_NEVER;

	VkSampler Sampler;
	vulkan(vkCreateSampler(Device, &SamplerInfo, nullptr, &Sampler));

	return Sampler;
}

VulkanCommandList::PendingGraphicsState::PendingGraphicsState(VulkanDevice& Device)
{
	VertexStreams.resize(Device.Properties.limits.maxVertexInputBindings);
	SetDefaultPipeline(Device);
}

void VulkanCommandList::PendingGraphicsState::SetDefaultPipeline(const VulkanDevice& Device)
{
	NumRTs = 0;
	std::fill(ColorTargets.begin(), ColorTargets.end(), VulkanRenderTargetViewRef());
	DepthTarget = nullptr;
	
	ResetVertexStreams();

	VertexInputState.pVertexAttributeDescriptions = nullptr;
	VertexInputState.pVertexBindingDescriptions = nullptr;
	VertexInputState.vertexAttributeDescriptionCount = 0;
	VertexInputState.vertexBindingDescriptionCount = 0;

	InputAssemblyState.primitiveRestartEnable = false;
	InputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	ViewportState.pScissors = nullptr;
	ViewportState.pViewports = nullptr;
	ViewportState.scissorCount = 0;
	ViewportState.viewportCount = 0;

	RasterizationState.cullMode = VK_CULL_MODE_NONE;
	RasterizationState.depthBiasClamp = false;
	RasterizationState.depthBiasEnable = false;
	RasterizationState.depthClampEnable = false;
	RasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	RasterizationState.lineWidth = 1.0f;
	RasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
	RasterizationState.rasterizerDiscardEnable = false;

	MultisampleState.alphaToCoverageEnable = false;
	MultisampleState.alphaToOneEnable = false;
	MultisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	MultisampleState.sampleShadingEnable = true;

	DepthStencilState.depthBoundsTestEnable = false;
	DepthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
	DepthStencilState.depthTestEnable = true;
	DepthStencilState.depthWriteEnable = true;
	DepthStencilState.stencilTestEnable = false;

	ColorBlendState.attachmentCount = 0;
	ColorBlendState.blendConstants[0] = 0.0f;
	ColorBlendState.blendConstants[1] = 0.0f;
	ColorBlendState.blendConstants[2] = 0.0f;
	ColorBlendState.blendConstants[3] = 0.0f;
	ColorBlendState.logicOp = VK_LOGIC_OP_COPY;
	ColorBlendState.logicOpEnable = false;
	ColorBlendState.pAttachments = nullptr;

	for (VkPipelineColorBlendAttachmentState& ColorBlendAttachment : ColorBlendAttachmentStates)
	{
		ColorBlendAttachment = {};
		ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		ColorBlendAttachment.blendEnable = false;
	}

	DynamicState.dynamicStateCount = 0;
	DynamicState.pDynamicStates = nullptr;

	Vertex = nullptr;
	TessControl = nullptr;
	TessEval = nullptr;
	Geometry = nullptr;
	Fragment = nullptr;
	Compute = nullptr;
}

void VulkanCommandList::PendingGraphicsState::ResetVertexStreams()
{
	std::fill(VertexStreams.begin(), VertexStreams.end(), VulkanVertexBufferRef());
}