#include "ShadowRender.h"
#include "Surface.h"
#include "MaterialShader.h"
#include "SceneRenderer.h"
#include <Components/Light.h>
#include <Components/Transform.h>
#include <Systems/ShadowSystem.h>

ShadowRender::ShadowRender(gpu::Device& device, const DirectionalLight& directionalLight)
	: _Width(Platform::GetFloat("Engine.ini", "Shadows", "Width", 400.0f))
	, _ZNear(Platform::GetFloat("Engine.ini", "Shadows", "ZNear", 1.0f))
	, _ZFar(Platform::GetFloat("Engine.ini", "Shadows", "ZFar", 96.0f))
{
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
}

void ShadowRender::Update(gpu::Device& device, const DirectionalLight& directionalLight, const Transform& transform, uint32 dynamicOffset)
{
	_DepthBiasConstantFactor = directionalLight.DepthBiasConstantFactor;
	_DepthBiasSlopeFactor = directionalLight.DepthBiasSlopeFactor;

	glm::mat4 lightProjMatrix = glm::ortho(-_Width * 0.5f, _Width * 0.5f, -_Width * 0.5f, _Width * 0.5f, _ZNear, _ZFar);
	lightProjMatrix[1][1] *= -1;

	const glm::mat4 lightViewMatrix = glm::lookAt(transform.GetForward(), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	_LightViewProjMatrix = lightProjMatrix * lightViewMatrix;
	_LightViewProjMatrixInv = glm::inverse(_LightViewProjMatrix);

	_DynamicOffset = dynamicOffset;
}

class ShadowDepthVS : public MeshShader
{
	using Base = MeshShader;
public:
	ShadowDepthVS() = default;

	static void SetEnvironmentVariables(ShaderCompilerWorker& worker)
	{
		Base::SetEnvironmentVariables(worker);
	}
};

REGISTER_SHADER(ShadowDepthVS, "../Shaders/ShadowDepthVS.glsl", "main", EShaderStage::Vertex);

class ShadowDepthFS : public MeshShader
{
	using Base = MeshShader;
public:
	ShadowDepthFS() = default;

	static void SetEnvironmentVariables(ShaderCompilerWorker& worker)
	{
		Base::SetEnvironmentVariables(worker);
	}
};

REGISTER_SHADER(ShadowDepthFS, "../Shaders/ShadowDepthFS.glsl", "main", EShaderStage::Fragment);

void SceneRenderer::RenderShadowDepths(CameraRender& camera, gpu::CommandBuffer& cmdBuf)
{
	for (auto entity : _ECS.GetEntities<ShadowRender>())
	{
		ShadowRender& shadowRender = _ECS.GetComponent<ShadowRender>(entity);

		cmdBuf.BeginRenderPass(shadowRender.GetRenderPass());

		cmdBuf.SetViewportAndScissor({ .width = shadowRender.GetShadowMap().GetWidth(), .height = shadowRender.GetShadowMap().GetHeight() });

		for (auto entity : _ECS.GetEntities<SurfaceGroup>())
		{
			auto& surfaceGroup = _ECS.GetComponent<SurfaceGroup>(entity);
			const VkDescriptorSet descriptorSets[] = { ShadowDescriptors::_DescriptorSet, surfaceGroup.GetSurfaceSet(), _Device.GetTextures() };
			const uint32 dynamicOffsets[] = { shadowRender.GetDynamicOffset() };

			surfaceGroup.Draw<false>(_Device, cmdBuf, std::size(descriptorSets), descriptorSets, std::size(dynamicOffsets), dynamicOffsets, [&] ()
			{
				PipelineStateDesc psoDesc = {};
				psoDesc.renderPass = shadowRender.GetRenderPass();
				psoDesc.shaderStages.vertex = _ShaderLibrary.FindShader<ShadowDepthVS>();
				psoDesc.shaderStages.fragment = _ShaderLibrary.FindShader<ShadowDepthFS>();
				psoDesc.rasterizationState.depthBiasEnable = true;
				psoDesc.rasterizationState.depthBiasConstantFactor = shadowRender.GetDepthBiasConstantFactor();
				psoDesc.rasterizationState.depthBiasSlopeFactor = shadowRender.GetDepthBiasSlopeFactor();
				return psoDesc;
			});
		}

		cmdBuf.EndRenderPass();
	}
}