#include "Light.h"
#include <Components/CLight.h>

CShadowProxy::CShadowProxy(const CDirectionalLight& DirectionalLight)
	: DepthBiasConstantFactor(DirectionalLight.DepthBiasConstantFactor)
	, DepthBiasSlopeFactor(DirectionalLight.DepthBiasSlopeFactor)
{
	const int32 Resolution = Platform.GetInt("Engine.ini", "Shadows", "Resolution", 2048);
	const glm::ivec2 ShadowMapRes = glm::ivec2(Resolution);

	ShadowMap = drm::CreateImage(ShadowMapRes.x, ShadowMapRes.y, 1, FORMAT, EImageUsage::RenderTargetable | EImageUsage::Sampled);

	DescriptorSet = drm::CreateDescriptorSet();
	DescriptorSet->Write(ShadowMap, SamplerState{}, ShaderBinding(1));

	Update(DirectionalLight.Direction);
}

void CShadowProxy::Update(const glm::vec3& Direction)
{
	const float64 ZNear = Platform.GetFloat64("Engine.ini", "Shadows", "ZNear", 1.0f);
	const float64 ZFar = Platform.GetFloat64("Engine.ini", "Shadows", "ZFar", 96.0f);
	const float64 FOV = Platform.GetFloat64("Engine.ini", "Shadows", "FOV", 45.0f);

	glm::mat4 LightProjMatrix = glm::perspective(FOV, ShadowMap->GetAspect(), ZNear, ZFar);
	LightProjMatrix[1][1] *= -1;
	const glm::mat4 LightViewMatrix = glm::lookAt(Direction, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	const glm::mat4 LightViewProjMatrix = LightProjMatrix * LightViewMatrix;

	LightViewProjBuffer = drm::CreateBuffer(EBufferUsage::Uniform, sizeof(glm::mat4), &LightViewProjMatrix);

	DescriptorSet->Write(LightViewProjBuffer, ShaderBinding(0));
	DescriptorSet->Update();
}