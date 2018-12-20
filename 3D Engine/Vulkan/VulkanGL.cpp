#include "VulkanGL.h"
#include "VulkanRenderTargetView.h"
#include "VulkanCommands.h"

#define CAST(FromType, ToType) ToType ## Ref ResourceCast(FromType ## Ref From) { return std::static_pointer_cast<ToType>(From); }

CAST(GLImage, VulkanImage);
CAST(GLRenderTargetView, VulkanRenderTargetView);
CAST(GLShader, VulkanShader);
CAST(GLVertexBuffer, VulkanVertexBuffer);
CAST(GLUniformBuffer, VulkanUniformBuffer);
CAST(GLIndexBuffer, VulkanIndexBuffer);

const Map<EImageFormat, uint32> ConvertImageFormatToSize()
{
	Map<EImageFormat, uint32> ImageFormatToGLSLSize;

	for (auto& [GLSLType, Format] : GLSLTypeToVulkanFormat)
	{
		auto ImageFormat = VulkanImage::GetEngineFormat(Format);
		auto GLSLSize = GetValue(GLSLTypeSizes, GLSLType);
		ImageFormatToGLSLSize[ImageFormat] = GLSLSize;
	}

	return ImageFormatToGLSLSize;
}

const Map<VkFormat, uint32> ConvertVulkanFormatToSize()
{
	Map<VkFormat, uint32> SizeOfVulkanFormat;

	for (auto&[GLSLType, Format] : GLSLTypeToVulkanFormat)
	{
		auto SizeOf = GetValue(GLSLTypeSizes, GLSLType);
		SizeOfVulkanFormat[Format] = SizeOf;
	}

	return SizeOfVulkanFormat;
}

VulkanGL::VulkanGL()
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
	, DescriptorSets([&] (VkDescriptorSet DescriptorSet) {})
	, ImageFormatToGLSLSize(ConvertImageFormatToSize())
	, SizeOfVulkanFormat(ConvertVulkanFormatToSize())
{
}

void VulkanSurface::InitSwapchain()
{
	SwapchainSupportDetails SwapchainSupport = {};
	SwapchainSupport.QuerySwapchainSupport(Device, Device.Surface);

	VkSurfaceFormatKHR SurfaceFormat = ChooseSwapSurfaceFormat(SwapchainSupport.Formats);
	VkPresentModeKHR PresentMode = ChooseSwapPresentMode(SwapchainSupport.PresentModes);

	VkExtent2D Extent = ChooseSwapExtent(SwapchainSupport.Capabilities);

	uint32 ImageCount = SwapchainSupport.Capabilities.minImageCount + 1;

	if (SwapchainSupport.Capabilities.maxImageCount > 0 && ImageCount > SwapchainSupport.Capabilities.maxImageCount)
	{
		ImageCount = SwapchainSupport.Capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR SwapchainInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
	SwapchainInfo.surface = Device.Surface;
	SwapchainInfo.minImageCount = ImageCount;
	SwapchainInfo.imageFormat = SurfaceFormat.format;
	SwapchainInfo.imageColorSpace = SurfaceFormat.colorSpace;
	SwapchainInfo.imageExtent = Extent;
	SwapchainInfo.imageArrayLayers = 1;
	SwapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices Indices = {};
	Indices.FindQueueFamilies(Device, Device.Surface);

	uint32 QueueFamilyIndices[] = { static_cast<uint32>(Indices.GraphicsFamily), static_cast<uint32>(Indices.PresentFamily) };

	if (Indices.GraphicsFamily != Indices.PresentFamily)
	{
		SwapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		SwapchainInfo.queueFamilyIndexCount = 2;
		SwapchainInfo.pQueueFamilyIndices = QueueFamilyIndices;
	}
	else
	{
		SwapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	SwapchainInfo.preTransform = SwapchainSupport.Capabilities.currentTransform;
	SwapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	SwapchainInfo.presentMode = PresentMode;
	SwapchainInfo.clipped = VK_TRUE;

	vulkan(vkCreateSwapchainKHR(Device, &SwapchainInfo, nullptr, &Swapchain));

	vkGetSwapchainImagesKHR(Device, Swapchain, &ImageCount, nullptr);

	std::vector<VkImage> VkImages(ImageCount);

	vkGetSwapchainImagesKHR(Device, Swapchain, &ImageCount, VkImages.data());

	Images.resize(ImageCount);

	VulkanGLRef Vulkan = std::static_pointer_cast<VulkanGL>(GRender);

	for (uint32 i = 0; i < ImageCount; i++)
	{
		Images[i] = MakeRef<VulkanImage>(Device
			, VkImages[i]
			, VkDeviceMemory()
			, VK_IMAGE_LAYOUT_UNDEFINED
			, VulkanImage::GetEngineFormat(SurfaceFormat.format)
			, Extent.width
			, Extent.height
			, RU_RenderTargetable
			, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
		Vulkan->CreateImageView(Images[i]);
	}

	RTViews.resize(ImageCount);

	for (uint32 i = 0; i < RTViews.size(); i++)
	{
		RTViews[i] = ResourceCast(GRender->CreateRenderTargetView(Images[i], ELoadAction::Clear, EStoreAction::Store, { 0.0f, 0.0f, 0.0f, 0.0f }));
	}
}

void VulkanGL::InitGL()
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

void VulkanGL::ReleaseGL()
{
	vkDestroySemaphore(Device, RenderEndSem, nullptr);
	vkDestroySemaphore(Device, ImageAvailableSem, nullptr);
	vkFreeCommandBuffers(Device, Device.CommandPool, CommandBuffers.size(), CommandBuffers.data());
}

void VulkanGL::BeginRender()
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

void VulkanGL::EndRender()
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
	DescriptorSets.Destroy();

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

void VulkanGL::SetRenderTargets(uint32 NumRTs, const GLRenderTargetViewRef* ColorTargets, const GLRenderTargetViewRef DepthTarget, EDepthStencilAccess Access)
{
	// @todo-joe Pretty heavyweight function... Should just defer the creation of this stuff until draw.
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

		check(Image && (Image->Usage & RU_RenderTargetable), "Color target is invalid.");
		check(Image->IsColor(), "Color target was not created in color format.");

		Image->Layout = [&] ()
		{
			if (ColorTarget == GetCurrentSwapchainRTView())
			{
				return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			}
			else if (Image->Usage & RU_ShaderResource)
			{
				return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			}
			else if (Image->Usage & RU_RenderTargetable)
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

	if (Access == DS_None)
	{
		DepthStencil.depthWriteEnable = false;
		DepthStencil.stencilTestEnable = false;
	}
	else
	{
		VulkanRenderTargetViewRef DepthTarget = Pending.DepthTarget;
		check(DepthTarget, "Depth target is invalid.");

		VulkanImageRef DepthImage = ResourceCast(DepthTarget->Image);
		check(DepthImage && (DepthImage->Usage & RU_RenderTargetable), "Depth target is invalid.");
		check(DepthImage->IsDepth() || DepthImage->IsStencil(), "Depth target was not created in a depth layout.");

		VkImageLayout FinalLayout = [&] ()
		{
			if (Access == DS_DepthReadStencilRead)
			{
				check(DepthImage->Usage & RU_ShaderResource, "Depth Image must be created with RU_ShaderResource.");
				DepthStencil.depthWriteEnable = true;
				DepthStencil.stencilTestEnable = true;
				return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			}
			else if (Access == DS_DepthReadStencilWrite)
			{
				check(DepthImage->Usage & RU_ShaderResource, "Depth Image must be created with RU_ShaderResource.");
				DepthStencil.depthWriteEnable = true;
				DepthStencil.stencilTestEnable = true;
				return VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
			}
			else if (Access == DS_DepthWriteStencilRead)
			{
				check(DepthImage->Usage & RU_ShaderResource, "Depth Image must be created with RU_ShaderResource.");
				DepthStencil.depthWriteEnable = true;
				DepthStencil.stencilTestEnable = true;
				return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
			}
			else
			{
				if (Access == DS_DepthWrite)
				{
					DepthStencil.depthWriteEnable = true;
					DepthStencil.stencilTestEnable = false;
				}
				else if (Access == DS_StencilWrite)
				{
					DepthStencil.depthWriteEnable = false;
					DepthStencil.stencilTestEnable = true;
				}
				else // DepthWriteStencilWrite
				{
					DepthStencil.depthWriteEnable = true;
					DepthStencil.stencilTestEnable = true;
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
		DepthDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		DepthDescription.finalLayout = FinalLayout;
		Descriptions[NumRTs] = DepthDescription;

		DepthRef.attachment = NumRTs;
		DepthRef.layout = FinalLayout;
	}

	VkSubpassDescription Subpass = {};
	Subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	Subpass.pColorAttachments = ColorRefs.data();
	Subpass.colorAttachmentCount = static_cast<uint32>(ColorRefs.size());
	Subpass.pDepthStencilAttachment = !DepthTarget || DS_None ? nullptr : &DepthRef;

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

void VulkanGL::SetGraphicsPipeline(GLShaderRef Vertex, GLShaderRef TessControl, GLShaderRef TessEval, GLShaderRef Geometry, GLShaderRef Fragment)
{
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

void VulkanGL::SetVertexStream(uint32 Location, GLVertexBufferRef VertexBuffer)
{
	check(Location < Device.Properties.limits.maxVertexInputBindings, "Invalid location.");

	VulkanVertexBufferRef VulkanVertexBuffer = ResourceCast(VertexBuffer);
	check(VulkanVertexBuffer, "Invalid vertex buffer.");

	Pending.VertexStreams[Location] = VulkanVertexBuffer;

	bDirtyVertexStreams = true;
}

GLVertexBufferRef VulkanGL::CreateVertexBuffer(EImageFormat EngineFormat, uint32 NumElements, EResourceUsageFlags Usage, const void* Data)
{
	uint32 GLSLSize = GetValue(ImageFormatToGLSLSize, EngineFormat);
	auto Buffer = Allocator.CreateBuffer(NumElements * GLSLSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, Usage, Data);
	return MakeRef<VulkanVertexBuffer>(Buffer, EngineFormat, Usage);
}

void VulkanGL::SetUniformBuffer(GLShaderRef Shader, uint32 Location, GLUniformBufferRef UniformBuffer)
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
			auto WriteDescriptorBuffer = std::make_unique<VulkanWriteDescriptorBuffer>(Binding, BufferInfo);
			DescriptorBuffers[VulkanShader->Meta.Stage][Location] = std::move(WriteDescriptorBuffer);
			bDirtyDescriptorSets = true;

			return;
		}
	}

	fail("A shader resource doesn't exist at this location.\nShader: %s, Location: %d", VulkanShader->Meta.EntryPoint.c_str(), Location);
}

void VulkanGL::SetShaderImage(GLShaderRef Shader, uint32 Location, GLImageRef Image, const SamplerState& Sampler)
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
			auto WriteDescriptorImage = std::make_unique<VulkanWriteDescriptorImage>(Binding, DescriptorImageInfo);
			DescriptorImages[VulkanShader->Meta.Stage][Location] = std::move(WriteDescriptorImage);
			bDirtyDescriptorSets = true;

			return;
		}
	}

	fail("A shader resource doesn't exist at this location.\nShader: %s, Location: %d", VulkanShader->Meta.EntryPoint.c_str(), Location);
}

void VulkanGL::DrawIndexed(GLIndexBufferRef IndexBuffer, uint32 IndexCount, uint32 InstanceCount, uint32 FirstIndex, uint32 VertexOffset, uint32 FirstInstance)
{
	PrepareForDraw();

	VulkanIndexBufferRef VulkanIndexBuffer = ResourceCast(IndexBuffer);
	VkIndexType IndexType = VulkanIndexBuffer->IndexStride == sizeof(uint32) ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16;

	vkCmdBindIndexBuffer(GetCommandBuffer(), VulkanIndexBuffer->Buffer->GetVulkanHandle(), VulkanIndexBuffer->Buffer->Offset, IndexType);
	vkCmdDrawIndexed(GetCommandBuffer(), IndexCount, InstanceCount, FirstIndex, VertexOffset, FirstInstance);
}

void VulkanGL::Draw(uint32 VertexCount, uint32 InstanceCount, uint32 FirstVertex, uint32 FirstInstance)
{
	PrepareForDraw();

	vkCmdDraw(GetCommandBuffer(), VertexCount, InstanceCount, FirstVertex, FirstInstance);
}

GLIndexBufferRef VulkanGL::CreateIndexBuffer(EImageFormat Format, uint32 NumIndices, EResourceUsageFlags Usage, const void * Data)
{
	check(Format == IF_R16_UINT || Format == IF_R32_UINT, "Format must be single-channel unsigned type.");

	uint32 IndexBufferStride = GetValue(ImageFormatToGLSLSize, Format);
	auto Buffer = Allocator.CreateBuffer(IndexBufferStride * NumIndices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, Usage, Data);
	return MakeRef<VulkanIndexBuffer>(Buffer, IndexBufferStride, Format, Usage);
}

GLUniformBufferRef VulkanGL::CreateUniformBuffer(uint32 Size, const void* Data, EUniformUpdate UniformUsage)
{
	EResourceUsageFlags Usage = UniformUsage == EUniformUpdate::Frequent ? RU_KeepCPUAccessible : RU_None;
	auto Buffer = Allocator.CreateBuffer(Size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, Usage, Data);
	return MakeRef<VulkanUniformBuffer>(Buffer);
}

// @todo CubemapCreateInfo
GLImageRef VulkanGL::CreateImage(uint32 Width, uint32 Height, EImageFormat Format, EResourceUsageFlags UsageFlags, const uint8* Data = nullptr)
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

	CreateImageView(GLImage);

	return GLImage;
}

GLRenderTargetViewRef VulkanGL::CreateRenderTargetView(GLImageRef Image, ELoadAction LoadAction, EStoreAction StoreAction, const std::array<float, 4>& ClearValue)
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

GLRenderTargetViewRef VulkanGL::CreateRenderTargetView(GLImageRef Image, ELoadAction LoadAction, EStoreAction StoreAction, float DepthClear, uint32 StencilClear)
{
	VulkanImageRef VulkanImage = ResourceCast(Image);
	VulkanRenderTargetViewRef RTView = MakeRef<VulkanRenderTargetView>(
		Device,
		VulkanImage,
		LoadAction,
		StoreAction,
		DepthClear,
		StencilClear);
	return RTView;
}

void VulkanGL::SetViewport(float X, float Y, float Width, float Height, float MinDepth, float MaxDepth)
{
	VkViewport& Viewport = Pending.Viewport;
	Viewport.x = X;
	Viewport.y = Y;
	Viewport.width = Width;
	Viewport.height = Height;
	Viewport.minDepth = MinDepth;
	Viewport.maxDepth = MaxDepth;

	bDirtyPipeline = true;
}

void VulkanGL::SetDepthTest(bool bDepthTestEnable, EDepthCompareTest CompareTest)
{
	static const Map<EDepthCompareTest, VkCompareOp> VulkanDepthCompare =
	{
		ENTRY(Depth_Never, VK_COMPARE_OP_NEVER)
		ENTRY(Depth_Less, VK_COMPARE_OP_LESS)
		ENTRY(Depth_Equal, VK_COMPARE_OP_EQUAL)
		ENTRY(Depth_LEqual, VK_COMPARE_OP_LESS_OR_EQUAL)
		ENTRY(Depth_Greater, VK_COMPARE_OP_GREATER)
		ENTRY(Depth_NEqual, VK_COMPARE_OP_NOT_EQUAL)
		ENTRY(Depth_GEqual, VK_COMPARE_OP_GREATER_OR_EQUAL)
		ENTRY(Depth_Always, VK_COMPARE_OP_ALWAYS)
	};

	VkPipelineDepthStencilStateCreateInfo& Depth = Pending.DepthStencilState;
	Depth.depthTestEnable = bDepthTestEnable;
	Depth.depthCompareOp = GetValue(VulkanDepthCompare, CompareTest);

	bDirtyPipeline = true;
}

void VulkanGL::SetStencilTest(bool bStencilTestEnable)
{
	Pending.DepthStencilState.stencilTestEnable = bStencilTestEnable;
	bDirtyPipeline = true;
}

void VulkanGL::SetStencilState(ECompareOp CompareOp,
	EStencilOp FailOp, 
	EStencilOp DepthFailOp, 
	EStencilOp PassOp, 
	uint32 CompareMask, 
	uint32 WriteMask, 
	uint32 Reference)
{
	static const Map<EStencilOp, VkStencilOp> VulkanStencilOp =
	{
		ENTRY(EStencilOp::Keep, VK_STENCIL_OP_KEEP)
		ENTRY(EStencilOp::Replace, VK_STENCIL_OP_REPLACE)
		ENTRY(EStencilOp::Zero, VK_STENCIL_OP_ZERO)
	};

	static const Map<ECompareOp, VkCompareOp> VulkanCompareOp =
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

	auto& DepthStencilState = Pending.DepthStencilState;
	DepthStencilState.back.compareOp = GetValue(VulkanCompareOp, CompareOp);
	DepthStencilState.back.failOp = GetValue(VulkanStencilOp, FailOp);
	DepthStencilState.back.depthFailOp = GetValue(VulkanStencilOp, DepthFailOp);
	DepthStencilState.back.passOp = GetValue(VulkanStencilOp, PassOp);
	DepthStencilState.back.compareMask = CompareMask;
	DepthStencilState.back.writeMask = WriteMask;
	DepthStencilState.back.reference = Reference;
	DepthStencilState.front = DepthStencilState.back;

	bDirtyPipeline = true;
}

void VulkanGL::SetRasterizerState(ECullMode CullMode, EFrontFace FrontFace, EPolygonMode PolygonMode, float LineWidth)
{
	static const Map<ECullMode, VkCullModeFlags> VulkanCullMode =
	{
		ENTRY(CM_None, VK_CULL_MODE_NONE)
		ENTRY(CM_Back, VK_CULL_MODE_BACK_BIT)
		ENTRY(CM_Front, VK_CULL_MODE_FRONT_BIT)
		ENTRY(CM_FrontAndBack, VK_CULL_MODE_FRONT_AND_BACK)
	};

	static const Map<EFrontFace, VkFrontFace> VulkanFrontFace =
	{
		ENTRY(FF_CW, VK_FRONT_FACE_CLOCKWISE)
		ENTRY(FF_CCW, VK_FRONT_FACE_COUNTER_CLOCKWISE)
	};

	static const Map<EPolygonMode, VkPolygonMode> VulkanPolygonMode =
	{
		ENTRY(PM_Fill, VK_POLYGON_MODE_FILL)
		ENTRY(PM_Line, VK_POLYGON_MODE_LINE)
		ENTRY(PM_Point, VK_POLYGON_MODE_POINT)
	};

	VkPipelineRasterizationStateCreateInfo& RasterizerState = Pending.RasterizationState;
	RasterizerState.cullMode = GetValue(VulkanCullMode, CullMode);
	RasterizerState.frontFace = GetValue(VulkanFrontFace, FrontFace);
	RasterizerState.polygonMode = GetValue(VulkanPolygonMode, PolygonMode);
	RasterizerState.lineWidth = LineWidth;

	bDirtyPipeline = true;
}

void VulkanGL::SetColorMask(uint32 RenderTargetIndex, EColorWriteMask ColorWriteMask)
{
	check(RenderTargetIndex < MaxSimultaneousRenderTargets, "Invalid render target index.");

	VkColorComponentFlags Flags = 0;
	Flags |= ColorWriteMask & Color_R ? VK_COLOR_COMPONENT_R_BIT : 0;
	Flags |= ColorWriteMask & Color_G ? VK_COLOR_COMPONENT_G_BIT : 0;
	Flags |= ColorWriteMask & Color_B ? VK_COLOR_COMPONENT_B_BIT : 0;
	Flags |= ColorWriteMask & Color_A ? VK_COLOR_COMPONENT_A_BIT : 0;

	Pending.ColorBlendAttachments[RenderTargetIndex].colorWriteMask = Flags;

	bDirtyPipeline = true;
}

void VulkanGL::SetInputAssembly(EPrimitiveTopology Topology)
{
	for (VkPrimitiveTopology VulkanTopology = VK_PRIMITIVE_TOPOLOGY_BEGIN_RANGE; VulkanTopology < VK_PRIMITIVE_TOPOLOGY_RANGE_SIZE;)
	{
		if (Topology == VulkanTopology)
		{
			Pending.InputAssemblyState.topology = VulkanTopology;
			bDirtyPipeline = true;
			return;
		}
		VulkanTopology = VkPrimitiveTopology(VulkanTopology + 1);
	}
	fail("VkPrimitiveTopology not found.");
}

GLRenderTargetViewRef VulkanGL::GetSurfaceView(ELoadAction LoadAction, EStoreAction StoreAction, const std::array<float, 4>& ClearValue)
{
	VulkanRenderTargetViewRef SurfaceView = GetCurrentSwapchainRTView();
	SurfaceView->LoadAction = LoadAction;
	SurfaceView->StoreAction = StoreAction;
	SurfaceView->ClearValue = ClearValue;
	return SurfaceView;
}

void* VulkanGL::LockBuffer(GLVertexBufferRef VertexBuffer, uint32 Size, uint32 Offset)
{
	// @todo-joe Handle Size, Offset
	VulkanVertexBufferRef VulkanVertexBuffer = ResourceCast(VertexBuffer);
	return Allocator.LockBuffer(*VulkanVertexBuffer->Buffer);
}

void VulkanGL::UnlockBuffer(GLVertexBufferRef VertexBuffer)
{
	VulkanVertexBufferRef VulkanVertexBuffer = ResourceCast(VertexBuffer);
	Allocator.UnlockBuffer(*VulkanVertexBuffer->Buffer);
}

void* VulkanGL::LockBuffer(GLIndexBufferRef IndexBuffer, uint32 Size, uint32 Offset)
{
	// @todo-joe Handle Size, Offset
	VulkanIndexBufferRef VulkanIndexBuffer = ResourceCast(IndexBuffer);
	return Allocator.LockBuffer(*VulkanIndexBuffer->Buffer);
}

void VulkanGL::UnlockBuffer(GLIndexBufferRef IndexBuffer)
{
	VulkanIndexBufferRef VulkanIndexBuffer = ResourceCast(IndexBuffer);
	Allocator.UnlockBuffer(*VulkanIndexBuffer->Buffer);
}

void VulkanGL::RebuildResolutionDependents()
{
	// @todo-joe
}

VkCommandBuffer& VulkanGL::GetCommandBuffer()
{
	return CommandBuffers[SwapchainIndex];
}

VulkanRenderTargetViewRef VulkanGL::GetCurrentSwapchainRTView()
{
	return Swapchain.RTViews[SwapchainIndex];
}

void VulkanGL::CleanPipelineLayout()
{
	std::vector<VkDescriptorSetLayoutBinding> AllBindings;

	static auto AddBindings = [&] (const VulkanShaderRef& Shader)
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

void VulkanGL::CleanDescriptorSets()
{
	VkDescriptorSet DescriptorSet = DescriptorPool.Spawn(DescriptorSetLayouts.Get());
	DescriptorSets.Push(DescriptorSet);

	std::vector<VkWriteDescriptorSet> WriteDescriptors;

	for (auto& DescriptorImagesInShaderStage : DescriptorImages)
	{
		auto& ImageDescriptors = DescriptorImagesInShaderStage.second;
		for (auto& Descriptors : ImageDescriptors)
		{
			const auto& WriteDescriptorImage = Descriptors.second;
			const auto& Binding = WriteDescriptorImage->Binding;

			VkWriteDescriptorSet WriteDescriptor = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			WriteDescriptor.dstSet = DescriptorSets.Get();
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
			const auto& WriteDescriptorImage = Descriptors.second;
			const auto& Binding = WriteDescriptorImage->Binding;

			VkWriteDescriptorSet WriteDescriptor = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			WriteDescriptor.dstSet = DescriptorSets.Get();
			WriteDescriptor.dstBinding = Binding.binding;
			WriteDescriptor.dstArrayElement = 0;
			WriteDescriptor.descriptorType = Binding.descriptorType;
			WriteDescriptor.descriptorCount = 1;
			WriteDescriptor.pBufferInfo = &WriteDescriptorImage->DescriptorBuffer;

			WriteDescriptors.push_back(WriteDescriptor);
		}
	}

	vkUpdateDescriptorSets(Device.Device, WriteDescriptors.size(), WriteDescriptors.data(), 0, nullptr);
	vkCmdBindDescriptorSets(GetCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayouts.Get(), 0, 1, &DescriptorSets.Get(), 0, nullptr);
	bDirtyDescriptorSets = false;
}

void VulkanGL::PrepareForDraw()
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
}

void VulkanGL::CleanRenderPass()
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

		const auto& ClearValue = ColorTarget->ClearValue;
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
			ClearValues[NumRTs].depthStencil.depth = DepthTarget->DepthClear;
		}

		if (Image->IsStencil())
		{
			ClearValues[NumRTs].depthStencil.stencil = DepthTarget->StencilClear;
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

void VulkanGL::CleanPipeline()
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
	Scissor.extent.width = 0;
	Scissor.extent.height = 0;
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
	ColorBlendState.pAttachments = Pending.ColorBlendAttachments.data();
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

void VulkanGL::CreateImageView(VulkanImageRef Image)
{
	VkImageViewCreateInfo ViewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	ViewInfo.image = Image->Image;
	ViewInfo.viewType = Image->Usage & RU_Cubemap ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
	ViewInfo.format = Image->GetVulkanFormat();
	ViewInfo.subresourceRange.aspectMask = Image->GetVulkanAspect();
	ViewInfo.subresourceRange.baseMipLevel = 0;
	ViewInfo.subresourceRange.levelCount = 1;
	ViewInfo.subresourceRange.baseArrayLayer = 0;
	ViewInfo.subresourceRange.layerCount = Image->Usage & RU_Cubemap ? 6 : 1;

	vulkan(vkCreateImageView(Device, &ViewInfo, nullptr, &Image->ImageView));
}

void VulkanGL::TransitionImageLayout(VulkanImageRef Image, VkImageLayout NewLayout, VkPipelineStageFlags DestinationStage)
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
	Barrier.subresourceRange.layerCount = Image->Usage & RU_Cubemap ? 6 : 1;

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

void VulkanGL::CreateImage(VkImage& Image, VkDeviceMemory& Memory, VkImageLayout& Layout
	, uint32 Width, uint32 Height, EImageFormat& Format, EResourceUsageFlags UsageFlags, const void* Data)
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
	Info.arrayLayers = UsageFlags & RU_Cubemap ? 6 : 1;
	Info.format = VulkanImage::GetVulkanFormat(Format);
	Info.tiling = VK_IMAGE_TILING_OPTIMAL;
	Info.initialLayout = Layout;
	Info.usage = [&] ()
	{
		VkFlags Usage = 0;

		if (UsageFlags & RU_RenderTargetable)
		{
			Usage |= GLImage::IsDepth(Format) ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		}

		Usage |= Data ? VK_IMAGE_USAGE_TRANSFER_DST_BIT : 0;
		Usage |= UsageFlags & RU_ShaderResource ? VK_IMAGE_USAGE_SAMPLED_BIT : 0;
		Usage |= UsageFlags & RU_UnorderedAccess ? VK_IMAGE_USAGE_STORAGE_BIT : 0;

		return Usage;
	}();
	Info.flags = UsageFlags & RU_Cubemap ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
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

VkSampler VulkanGL::CreateSampler(const SamplerState& SamplerState)
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

	VkFilter Filter = VulkanFilters[SamplerState.Filter];
	VkSamplerMipmapMode SMM = VulkanMipmapModes[SamplerState.SMM];
	VkSamplerAddressMode SAM = VulkanAddressModes[SamplerState.SAM];

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

VulkanGL::PendingGraphicsState::PendingGraphicsState(VulkanDevice& Device)
{
	VertexStreams.resize(Device.Properties.limits.maxVertexInputBindings);
	SetDefaultPipeline(Device);
}

void VulkanGL::PendingGraphicsState::SetDefaultPipeline(const VulkanDevice& Device)
{
	ResetVertexStreams();

	VkPipelineRasterizationStateCreateInfo& Rasterization = RasterizationState;
	Rasterization.depthBiasClamp = false;
	Rasterization.rasterizerDiscardEnable = false;
	Rasterization.depthBiasEnable = false;

	VkPipelineDepthStencilStateCreateInfo& DepthStencil = DepthStencilState;
	DepthStencil.depthBoundsTestEnable = false;

	VkPipelineInputAssemblyStateCreateInfo& InputAssembly = InputAssemblyState;
	InputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	InputAssembly.primitiveRestartEnable = false;

	VkPipelineMultisampleStateCreateInfo& Multisample = MultisampleState;
	Multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendStateCreateInfo& ColorBlend = ColorBlendState;
	ColorBlend.logicOpEnable = false;
	ColorBlend.logicOp = VK_LOGIC_OP_COPY;
	ColorBlend.blendConstants[0] = 0.0f;
	ColorBlend.blendConstants[1] = 0.0f;
	ColorBlend.blendConstants[2] = 0.0f;
	ColorBlend.blendConstants[3] = 0.0f;

	for (VkPipelineColorBlendAttachmentState& ColorBlendAttachment : ColorBlendAttachments)
	{
		ColorBlendAttachment = {};
		ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		ColorBlendAttachment.blendEnable = false;
	}
}

void VulkanGL::PendingGraphicsState::ResetVertexStreams()
{
	std::fill(VertexStreams.begin(), VertexStreams.end(), VulkanVertexBufferRef());
}