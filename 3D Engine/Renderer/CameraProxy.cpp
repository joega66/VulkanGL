#include "CameraProxy.h"
#include <Engine/Screen.h>
#include <Engine/Camera.h>

BEGIN_UNIFORM_BUFFER(CameraUniform)
	MEMBER(glm::mat4, worldToView)
	MEMBER(glm::mat4, viewToClip)
	MEMBER(glm::mat4, worldToClip)
	MEMBER(glm::mat4, clipToWorld)
	MEMBER(glm::mat4, prevWorldToClip)
	MEMBER(glm::vec3, position)
	MEMBER(float, _pad0)
	MEMBER(float, aspectRatio)
	MEMBER(float, fov)
	MEMBER(glm::vec2, screenDims)
	MEMBER(glm::vec3, clipData)
	MEMBER(float, _pad1)
END_UNIFORM_BUFFER(CameraUniform)

BEGIN_DESCRIPTOR_SET(CameraDescriptors)
	DESCRIPTOR(gpu::UniformBuffer<CameraUniform>, _CameraUniform)
	DESCRIPTOR(gpu::SampledImage, _SceneDepth)
	DESCRIPTOR(gpu::SampledImage, _GBuffer0)
	DESCRIPTOR(gpu::SampledImage, _GBuffer1)
	DESCRIPTOR(gpu::StorageImage, _SceneColor)
	DESCRIPTOR(gpu::StorageImage, _SSRHistory)
	DESCRIPTOR(gpu::StorageImage, _SSGIHistory)
	DESCRIPTOR(gpu::StorageImage, _DirectLighting)
END_DESCRIPTOR_SET(CameraDescriptors)

CameraProxy::CameraProxy(Screen& screen, gpu::Device& device, gpu::Surface& surface)
{
	_CameraDescriptorSet = device.CreateDescriptorSet<CameraDescriptors>();

	_CameraUniformBuffer = device.CreateBuffer(EBufferUsage::Uniform, EMemoryUsage::CPU_TO_GPU, sizeof(CameraUniform));
	
	screen.OnScreenResize([this, &device, &surface] (int32 width, int32 height)
	{
		_DirectLighting = device.CreateImage(width, height, 1, EFormat::R16G16B16A16_SFLOAT, EImageUsage::Attachment | EImageUsage::Storage);

		_SceneColor = device.CreateImage(width, height, 1, EFormat::R16G16B16A16_SFLOAT, EImageUsage::Attachment | EImageUsage::Storage | EImageUsage::TransferDst);
		_SceneDepth = device.CreateImage(width, height, 1, EFormat::D32_SFLOAT, EImageUsage::Attachment | EImageUsage::Sampled);

		_GBuffer0 = device.CreateImage(width, height, 1, EFormat::R32G32B32A32_SFLOAT, EImageUsage::Attachment | EImageUsage::Sampled);
		_GBuffer1 = device.CreateImage(width, height, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Attachment | EImageUsage::Sampled);
		
		_SSRHistory = device.CreateImage(width, height, 1, EFormat::R16G16B16A16_SFLOAT, EImageUsage::Storage);
		_SSGIHistory = device.CreateImage(width, height, 1, EFormat::R16G16B16A16_SFLOAT, EImageUsage::Storage);

		CreateGBufferRP(device);
		CreateSkyboxRP(device);
		CreateUserInterfaceRP(device, surface);

		const gpu::Sampler sampler = device.CreateSampler({ EFilter::Nearest });

		CameraDescriptors cameraDescriptors;
		cameraDescriptors._CameraUniform = { _CameraUniformBuffer };
		cameraDescriptors._SceneDepth = { _SceneDepth, sampler };
		cameraDescriptors._GBuffer0 = { _GBuffer0, sampler };
		cameraDescriptors._GBuffer1 = { _GBuffer1, sampler };
		cameraDescriptors._SceneColor = _SceneColor;
		cameraDescriptors._SSRHistory = _SSRHistory;
		cameraDescriptors._SSGIHistory = _SSGIHistory;
		cameraDescriptors._DirectLighting = _DirectLighting;
		
		device.UpdateDescriptorSet(_CameraDescriptorSet, cameraDescriptors);

		gpu::CommandList cmdList = device.CreateCommandList(EQueue::Transfer);
		
		ImageMemoryBarrier barriers[] = { { _SSRHistory }, { _SSGIHistory } };

		for (auto& barrier : barriers)
		{
			barrier.srcAccessMask = EAccess::None;
			barrier.dstAccessMask = EAccess::ShaderRead | EAccess::ShaderWrite;
			barrier.oldLayout = EImageLayout::Undefined;
			barrier.newLayout = EImageLayout::General;
		}

		cmdList.PipelineBarrier(EPipelineStage::TopOfPipe, EPipelineStage::ComputeShader, 0, nullptr, std::size(barriers), barriers);

		device.SubmitCommands(cmdList);
	});
}

void CameraProxy::Update(const Camera& camera)
{
	UpdateCameraUniform(camera);
}

void CameraProxy::UpdateCameraUniform(const Camera& camera)
{
	const glm::vec3 clipData(
		camera.GetFarPlane() * camera.GetNearPlane(),
		camera.GetNearPlane() - camera.GetFarPlane(),
		camera.GetFarPlane());

	const glm::mat4 clipToWorld = glm::inverse(camera.GetWorldToClip());

	const CameraUniform cameraUniform =
	{
		camera.GetWorldToView(),
		camera.GetViewToClip(),
		camera.GetWorldToClip(),
		clipToWorld,
		camera.GetPrevWorldToClip(),
		camera.GetPosition(),
		0.0f,
		camera.GetAspectRatio(),
		camera.GetFOV(),
		glm::vec2(camera.GetWidth(), camera.GetHeight()),
		clipData,
		0.0f,
	};

	Platform::Memcpy(_CameraUniformBuffer.GetData(), &cameraUniform, sizeof(cameraUniform));
}

void CameraProxy::CreateGBufferRP(gpu::Device& device)
{
	RenderPassDesc rpDesc = {};
	rpDesc.colorAttachments =
	{
		AttachmentView(&_GBuffer0, ELoadAction::Clear, EStoreAction::Store, ClearColorValue{}, EImageLayout::Undefined, EImageLayout::ShaderReadOnlyOptimal),
		AttachmentView(&_GBuffer1, ELoadAction::Clear, EStoreAction::Store, ClearColorValue{}, EImageLayout::Undefined, EImageLayout::ShaderReadOnlyOptimal)
	};
	rpDesc.depthAttachment = AttachmentView(
		&_SceneDepth,
		ELoadAction::Clear,
		EStoreAction::Store,
		ClearDepthStencilValue{},
		EImageLayout::Undefined,
		EImageLayout::DepthReadStencilWrite);
	rpDesc.renderArea = RenderArea{ glm::ivec2(), glm::uvec2(_SceneDepth.GetWidth(), _SceneDepth.GetHeight()) };
	rpDesc.srcStageMask = EPipelineStage::TopOfPipe;
	rpDesc.dstStageMask = EPipelineStage::ComputeShader;
	rpDesc.srcAccessMask = EAccess::None;
	rpDesc.dstAccessMask = EAccess::ShaderRead | EAccess::ShaderWrite;

	_GBufferRP = device.CreateRenderPass(rpDesc);
}

void CameraProxy::CreateSkyboxRP(gpu::Device& device)
{
	RenderPassDesc rpDesc = {};
	rpDesc.colorAttachments.push_back(
		AttachmentView(&_DirectLighting, ELoadAction::Load, EStoreAction::Store, ClearColorValue{}, EImageLayout::General, EImageLayout::General));
	rpDesc.depthAttachment = AttachmentView(
		&_SceneDepth,
		ELoadAction::Load,
		EStoreAction::Store,
		ClearDepthStencilValue{},
		EImageLayout::DepthReadStencilWrite,
		EImageLayout::DepthReadStencilWrite);
	rpDesc.renderArea = RenderArea{ glm::ivec2(), glm::uvec2(_SceneDepth.GetWidth(), _SceneDepth.GetHeight()) };
	rpDesc.srcStageMask = EPipelineStage::ComputeShader;
	rpDesc.dstStageMask = EPipelineStage::ComputeShader;
	rpDesc.srcAccessMask = EAccess::ShaderRead | EAccess::ShaderWrite;
	rpDesc.dstAccessMask = EAccess::ShaderRead | EAccess::ShaderWrite;
	_SkyboxRP = device.CreateRenderPass(rpDesc);
}

void CameraProxy::CreateUserInterfaceRP(gpu::Device& device, gpu::Surface& surface)
{
	const auto& images = surface.GetImages();

	_UserInterfaceRP.clear();
	_UserInterfaceRP.reserve(images.size());

	for (const auto& image : images)
	{
		// After UI rendering, the image is ready for the present queue.
		RenderPassDesc rpDesc = {};
		rpDesc.colorAttachments.push_back(
			AttachmentView(&image, ELoadAction::Load, EStoreAction::Store, ClearColorValue{}, EImageLayout::ColorAttachmentOptimal, EImageLayout::Present));
		rpDesc.renderArea.extent = { image.GetWidth(), image.GetHeight() };
		rpDesc.srcStageMask = EPipelineStage::ComputeShader;
		rpDesc.dstStageMask = EPipelineStage::ColorAttachmentOutput;
		rpDesc.srcAccessMask = EAccess::ShaderRead | EAccess::ShaderWrite;
		rpDesc.dstAccessMask = EAccess::ColorAttachmentRead | EAccess::ColorAttachmentWrite;

		_UserInterfaceRP.push_back(device.CreateRenderPass(rpDesc));
	}
}