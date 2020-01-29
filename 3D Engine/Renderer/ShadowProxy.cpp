#include "ShadowProxy.h"
#include <Components/CLight.h>
#include <ECS/EntityManager.h>

void ShadowProxy::InitCallbacks(DRMDevice& Device, EntityManager& ECS)
{
	ECS.NewComponentCallback<CDirectionalLight>([&] (Entity& Entity, CDirectionalLight& DirectionalLight)
	{
		ECS.AddComponent<ShadowProxy>(Entity, ShadowProxy(Device, DirectionalLight));
	});
}

ShadowProxy::ShadowProxy(DRMDevice& Device, const CDirectionalLight& DirectionalLight)
{
	const int32 Resolution = Platform::GetInt("Engine.ini", "Shadows", "Resolution", 2048);
	const glm::ivec2 ShadowMapRes = glm::ivec2(Resolution);

	ShadowMap = Device.CreateImage(ShadowMapRes.x, ShadowMapRes.y, 1, FORMAT, EImageUsage::Attachment | EImageUsage::Sampled);

	RenderPassInitializer RPInfo = { 0 };
	RPInfo.DepthAttachment = drm::AttachmentView(
		ShadowMap,
		ELoadAction::Clear, EStoreAction::Store,
		ClearDepthStencilValue{},
		EImageLayout::Undefined,
		EImageLayout::DepthReadStencilWrite
	);
	RPInfo.RenderArea = RenderArea{ glm::ivec2{}, glm::uvec2(ShadowMap->Width, ShadowMap->Height) };

	RenderPass = Device.CreateRenderPass(RPInfo);

	DescriptorSet = Device.CreateDescriptorSet();
	DescriptorSet->Write(ShadowMap, SamplerState{}, 1);

	Update(Device, DirectionalLight);
}

void ShadowProxy::Update(DRMDevice& Device, const CDirectionalLight& DirectionalLight)
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
	
	LightViewProjBuffer = Device.CreateBuffer(EBufferUsage::Uniform, sizeof(glm::mat4), &LightViewProjMatrix);

	DescriptorSet->Write(LightViewProjBuffer, 0);
	DescriptorSet->Update();
}