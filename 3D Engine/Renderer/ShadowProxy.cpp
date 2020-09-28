#include "ShadowProxy.h"
#include <Components/Light.h>
#include <Components/Transform.h>
#include <ECS/EntityManager.h>
#include "MaterialShader.h"

BEGIN_UNIFORM_BUFFER(ShadowUniform)
	MEMBER(glm::mat4, lightViewProj)
	MEMBER(glm::mat4, invLightViewProj)
END_UNIFORM_BUFFER(ShadowUniform)

BEGIN_DESCRIPTOR_SET(ShadowDescriptors)
	DESCRIPTOR(gpu::UniformBuffer<ShadowUniform>, _ShadowUniform)
END_DESCRIPTOR_SET(ShadowDescriptors)

ShadowProxy::ShadowProxy(gpu::Device& device, const DirectionalLight& directionalLight)
	: _Width(Platform::GetFloat("Engine.ini", "Shadows", "Width", 400.0f))
	, _ZNear(Platform::GetFloat("Engine.ini", "Shadows", "ZNear", 1.0f))
	, _ZFar(Platform::GetFloat("Engine.ini", "Shadows", "ZFar", 96.0f))
{
	_ShadowUniform = device.CreateBuffer(EBufferUsage::Uniform, EMemoryUsage::CPU_TO_GPU, sizeof(ShadowUniform));

	const glm::ivec2 shadowMapRes(Platform::GetInt("Engine.ini", "Shadows", "Resolution", 2048));

	_ShadowMap = device.CreateImage(shadowMapRes.x, shadowMapRes.y, 1, EFormat::D32_SFLOAT, EImageUsage::Attachment | EImageUsage::Sampled);
	
	RenderPassDesc rpDesc = {};
	rpDesc.depthAttachment = AttachmentView(
		&_ShadowMap,
		ELoadAction::Clear, EStoreAction::Store,
		ClearDepthStencilValue{},
		EImageLayout::Undefined,
		EImageLayout::DepthReadStencilWrite);
	rpDesc.renderArea = RenderArea{ glm::ivec2{}, glm::uvec2(_ShadowMap.GetWidth(), _ShadowMap.GetHeight()) };
	rpDesc.srcStageMask = EPipelineStage::TopOfPipe;
	rpDesc.dstStageMask = EPipelineStage::ComputeShader;
	rpDesc.srcAccessMask = EAccess::None;
	rpDesc.dstAccessMask = EAccess::ShaderRead;
	
	_RenderPass = device.CreateRenderPass(rpDesc);

	ShadowDescriptors shadowDescriptors;
	shadowDescriptors._ShadowUniform = _ShadowUniform;
	
	_DescriptorSet = device.CreateDescriptorSet(shadowDescriptors);
}

void ShadowProxy::Update(gpu::Device& device, const DirectionalLight& directionalLight, const Transform& transform)
{
	_DepthBiasConstantFactor = directionalLight.DepthBiasConstantFactor;
	_DepthBiasSlopeFactor = directionalLight.DepthBiasSlopeFactor;

	glm::mat4 lightProjMatrix = glm::ortho(-_Width * 0.5f, _Width * 0.5f, -_Width * 0.5f, _Width * 0.5f, _ZNear, _ZFar);
	lightProjMatrix[1][1] *= -1;

	const glm::mat4 lightViewMatrix = glm::lookAt(transform.GetForward(), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	_LightViewProjMatrix = lightProjMatrix * lightViewMatrix;
	_LightViewProjMatrixInv = glm::inverse(_LightViewProjMatrix);

	ShadowUniform* shadowUniform = static_cast<ShadowUniform*>(_ShadowUniform.GetData());
	shadowUniform->lightViewProj = _LightViewProjMatrix;
	shadowUniform->invLightViewProj = _LightViewProjMatrixInv;
}