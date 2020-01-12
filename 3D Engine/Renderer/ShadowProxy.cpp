#include "ShadowProxy.h"
#include <Components/CLight.h>
#include <ECS/EntityManager.h>

void ShadowProxy::InitCallbacks(EntityManager& ECS)
{
	ECS.NewComponentCallback<CDirectionalLight>([&] (Entity& Entity, CDirectionalLight& DirectionalLight)
	{
		ECS.AddComponent<ShadowProxy>(Entity, ShadowProxy(DirectionalLight));
	});
}

ShadowProxy::ShadowProxy(const CDirectionalLight& DirectionalLight)
{
	const int32 Resolution = Platform.GetInt("Engine.ini", "Shadows", "Resolution", 2048);
	const glm::ivec2 ShadowMapRes = glm::ivec2(Resolution);

	ShadowMap = drm::CreateImage(ShadowMapRes.x, ShadowMapRes.y, 1, FORMAT, EImageUsage::Attachment | EImageUsage::Sampled);

	DescriptorSet = drm::CreateDescriptorSet();
	DescriptorSet->Write(ShadowMap, SamplerState{}, 1);

	Update(DirectionalLight);
}

void ShadowProxy::Update(const CDirectionalLight& DirectionalLight)
{
	DepthBiasConstantFactor = DirectionalLight.DepthBiasConstantFactor;
	DepthBiasSlopeFactor = DirectionalLight.DepthBiasSlopeFactor;

	const float64 ZNear = Platform.GetFloat64("Engine.ini", "Shadows", "ZNear", 1.0f);
	const float64 ZFar = Platform.GetFloat64("Engine.ini", "Shadows", "ZFar", 96.0f);
	const float64 FOV = Platform.GetFloat64("Engine.ini", "Shadows", "FOV", 45.0f);

	glm::mat4 LightProjMatrix = glm::perspective(FOV, ShadowMap->GetAspect(), ZNear, ZFar);
	LightProjMatrix[1][1] *= -1;
	const glm::mat4 LightViewMatrix = glm::lookAt(DirectionalLight.Direction, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	const glm::mat4 LightViewProjMatrix = LightProjMatrix * LightViewMatrix;
	
	LightViewProjBuffer = drm::CreateBuffer(EBufferUsage::Uniform, sizeof(glm::mat4), &LightViewProjMatrix);

	DescriptorSet->Write(LightViewProjBuffer, 0);
	DescriptorSet->Update();
}