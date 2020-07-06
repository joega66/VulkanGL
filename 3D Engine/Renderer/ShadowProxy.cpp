#include "ShadowProxy.h"
#include <Components/Light.h>
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
	_ShadowUniform = device.CreateBuffer(EBufferUsage::Uniform | EBufferUsage::HostVisible, sizeof(ShadowUniform));

	const glm::ivec2 shadowMapRes(Platform::GetInt("Engine.ini", "Shadows", "Resolution", 2048));

	_ShadowMap = device.CreateImage(shadowMapRes.x, shadowMapRes.y, 1, EFormat::D32_SFLOAT, EImageUsage::Attachment | EImageUsage::Sampled);

	RenderPassDesc rpDesc = {};
	rpDesc.depthAttachment = gpu::AttachmentView(
		&_ShadowMap,
		ELoadAction::Clear, EStoreAction::Store,
		ClearDepthStencilValue{},
		EImageLayout::Undefined,
		EImageLayout::DepthReadStencilWrite);
	rpDesc.renderArea = RenderArea{ glm::ivec2{}, glm::uvec2(_ShadowMap.GetWidth(), _ShadowMap.GetHeight()) };
	
	_RenderPass = device.CreateRenderPass(rpDesc);

	ShadowDescriptors shadowDescriptors;
	shadowDescriptors._ShadowUniform = _ShadowUniform;
	
	_DescriptorSet = device.CreateDescriptorSet(shadowDescriptors);
}

void ShadowProxy::Update(gpu::Device& device, const DirectionalLight& directionalLight)
{
	_DepthBiasConstantFactor = directionalLight.DepthBiasConstantFactor;
	_DepthBiasSlopeFactor = directionalLight.DepthBiasSlopeFactor;

	_L = glm::vec4(glm::normalize(directionalLight.Direction), 1.0f);
	_Radiance = glm::vec4(directionalLight.Intensity * directionalLight.Color, 1.0f);

	glm::mat4 lightProjMatrix = glm::ortho(-_Width * 0.5f, _Width * 0.5f, -_Width * 0.5f, _Width * 0.5f, _ZNear, _ZFar);
	lightProjMatrix[1][1] *= -1;

	const glm::mat4 lightViewMatrix = glm::lookAt(directionalLight.Direction, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	_LightViewProjMatrix = lightProjMatrix * lightViewMatrix;
	_LightViewProjMatrixInv = glm::inverse(_LightViewProjMatrix);

	ShadowUniform* shadowUniform = static_cast<ShadowUniform*>(_ShadowUniform.GetData());
	shadowUniform->lightViewProj = _LightViewProjMatrix;
	shadowUniform->invLightViewProj = _LightViewProjMatrixInv;
}

template<EMeshType meshType>
class ShadowDepthVS : public MeshShader<meshType>
{
	using Base = MeshShader<meshType>;
public:
	ShadowDepthVS(const ShaderCompilationInfo& compilationInfo)
		: Base(compilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& worker)
	{
		Base::SetEnvironmentVariables(worker);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo info = { "../Shaders/ShadowDepthVS.glsl", "main", EShaderStage::Vertex };
		return info;
	}
};

template<EMeshType meshType>
class ShadowDepthFS : public MeshShader<meshType>
{
	using Base = MeshShader<meshType>;
public:
	ShadowDepthFS(const ShaderCompilationInfo& compilationInfo)
		: Base(compilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& worker)
	{
		Base::SetEnvironmentVariables(worker);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo info = { "../Shaders/ShadowDepthFS.glsl", "main", EShaderStage::Fragment };
		return info;
	}
};

void ShadowProxy::AddMesh(gpu::Device& device, gpu::ShaderLibrary& shaderLibrary, const MeshProxy& meshProxy)
{
	constexpr EMeshType meshType = EMeshType::StaticMesh;

	PipelineStateDesc psoDesc = {};
	psoDesc.renderPass = _RenderPass;
	psoDesc.shaderStages.vertex = shaderLibrary.FindShader<ShadowDepthVS<meshType>>();
	psoDesc.shaderStages.fragment = shaderLibrary.FindShader<ShadowDepthFS<meshType>>();
	psoDesc.viewport.width = _ShadowMap.GetWidth();
	psoDesc.viewport.height = _ShadowMap.GetHeight();
	psoDesc.rasterizationState.depthBiasEnable = true;
	psoDesc.rasterizationState.depthBiasConstantFactor = _DepthBiasConstantFactor;
	psoDesc.rasterizationState.depthBiasSlopeFactor = _DepthBiasSlopeFactor;

	const std::vector<VkDescriptorSet> descriptorSets = { _DescriptorSet, meshProxy.GetSurfaceSet(), device.GetTextures().GetSet(), device.GetSamplers().GetSet() };
	_MeshDrawCommands.push_back(MeshDrawCommand(device, meshProxy, psoDesc, descriptorSets));
}

void ShadowProxy::Render(gpu::CommandList& cmdList)
{
	cmdList.BeginRenderPass(_RenderPass);

	MeshDrawCommand::Draw(cmdList, _MeshDrawCommands);

	cmdList.EndRenderPass();

	_MeshDrawCommands.clear();
}