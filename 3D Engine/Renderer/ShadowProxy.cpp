#include "ShadowProxy.h"
#include <Components/Light.h>
#include <ECS/EntityManager.h>

ShadowProxy::ShadowProxy(DRMDevice& Device, DescriptorSetLayout<ShadowDescriptors>& ShadowLayout, const DirectionalLight& DirectionalLight)
{
	LightViewProjBuffer = Device.CreateBuffer(EBufferUsage::Uniform | EBufferUsage::HostVisible, sizeof(glm::mat4));

	const int32 Resolution = Platform::GetInt("Engine.ini", "Shadows", "Resolution", 2048);
	const glm::ivec2 ShadowMapRes = glm::ivec2(Resolution);

	ShadowMap = Device.CreateImage(ShadowMapRes.x, ShadowMapRes.y, 1, FORMAT, EImageUsage::Attachment | EImageUsage::Sampled);

	RenderPassDesc RPDesc = {};
	RPDesc.DepthAttachment = drm::AttachmentView(
		&ShadowMap,
		ELoadAction::Clear, EStoreAction::Store,
		ClearDepthStencilValue{},
		EImageLayout::Undefined,
		EImageLayout::DepthReadStencilWrite);
	RPDesc.RenderArea = RenderArea{ glm::ivec2{}, glm::uvec2(ShadowMap.GetWidth(), ShadowMap.GetHeight()) };

	RenderPass = Device.CreateRenderPass(RPDesc);

	ShadowDescriptors Descriptors;
	Descriptors.ShadowMap = &ShadowMap;
	Descriptors.LightViewProjBuffer = &LightViewProjBuffer;

	DescriptorSet = ShadowLayout.CreateDescriptorSet();
	ShadowLayout.UpdateDescriptorSet(DescriptorSet, Descriptors);
}

void ShadowProxy::Update(DRMDevice& Device, const DirectionalLight& DirectionalLight)
{
	DepthBiasConstantFactor = DirectionalLight.DepthBiasConstantFactor;
	DepthBiasSlopeFactor = DirectionalLight.DepthBiasSlopeFactor;

	const float64 Width = Platform::GetFloat64("Engine.ini", "Shadows", "Width", 400.0f);
	const float64 ZNear = Platform::GetFloat64("Engine.ini", "Shadows", "ZNear", 1.0f);
	const float64 ZFar = Platform::GetFloat64("Engine.ini", "Shadows", "ZFar", 96.0f);
	
	glm::mat4 LightProjMatrix = glm::ortho(-(float)Width * 0.5f, (float)Width * 0.5f, -(float)Width * 0.5f, (float)Width * 0.5f, (float)ZNear, (float)ZFar);
	LightProjMatrix[1][1] *= -1;
	const glm::mat4 LightViewMatrix = glm::lookAt(DirectionalLight.Direction, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	const glm::mat4 LightViewProjMatrix = LightProjMatrix * LightViewMatrix;
	
	glm::mat4* LightViewProjMatrixPtr = static_cast<glm::mat4*>(Device.LockBuffer(LightViewProjBuffer));
	*LightViewProjMatrixPtr = LightViewProjMatrix;
	Device.UnlockBuffer(LightViewProjBuffer);
}