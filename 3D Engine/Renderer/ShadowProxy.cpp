#include "ShadowProxy.h"
#include <Components/Light.h>
#include <ECS/EntityManager.h>
#include "MaterialShader.h"

UNIFORM_STRUCT(LightViewProjUniformData,
	glm::mat4 LightViewProj;
	glm::mat4 InvLightViewProj;
);

UNIFORM_STRUCT(VolumeLightingUniformData,
	glm::ivec4 ShadowMapSize;
	glm::vec4 L;
	glm::vec4 Radiance;
);

ShadowProxy::ShadowProxy(gpu::Device& Device, DescriptorSetLayout<ShadowDescriptors>& ShadowLayout, const DirectionalLight& DirectionalLight)
	: Width(Platform::GetFloat("Engine.ini", "Shadows", "Width", 400.0f))
	, ZNear(Platform::GetFloat("Engine.ini", "Shadows", "ZNear", 1.0f))
	, ZFar(Platform::GetFloat("Engine.ini", "Shadows", "ZFar", 96.0f))
{
	LightViewProjBuffer = Device.CreateBuffer(EBufferUsage::Uniform | EBufferUsage::HostVisible, sizeof(LightViewProjUniformData));

	const int32 Resolution = Platform::GetInt("Engine.ini", "Shadows", "Resolution", 2048);
	const glm::ivec2 ShadowMapRes(Resolution);

	ShadowMap = Device.CreateImage(ShadowMapRes.x, ShadowMapRes.y, 1, EFormat::D32_SFLOAT, EImageUsage::Attachment | EImageUsage::Sampled);

	VolumeLightingUniform = Device.CreateBuffer(EBufferUsage::Uniform | EBufferUsage::HostVisible, sizeof(VolumeLightingUniformData));

	RenderPassDesc rpDesc = {};
	rpDesc.depthAttachment = gpu::AttachmentView(
		&ShadowMap,
		ELoadAction::Clear, EStoreAction::Store,
		ClearDepthStencilValue{},
		EImageLayout::Undefined,
		EImageLayout::DepthReadStencilWrite);
	rpDesc.renderArea = RenderArea{ glm::ivec2{}, glm::uvec2(ShadowMap.GetWidth(), ShadowMap.GetHeight()) };
	
	RenderPass = Device.CreateRenderPass(rpDesc);

	ShadowDescriptors Descriptors;
	Descriptors.LightViewProjBuffer = LightViewProjBuffer;
	Descriptors.VolumeLightingUniform = VolumeLightingUniform;

	DescriptorSet = ShadowLayout.CreateDescriptorSet(Device);
	ShadowLayout.UpdateDescriptorSet(Device, DescriptorSet, Descriptors);
}

void ShadowProxy::Update(gpu::Device& Device, const DirectionalLight& DirectionalLight)
{
	DepthBiasConstantFactor = DirectionalLight.DepthBiasConstantFactor;
	DepthBiasSlopeFactor = DirectionalLight.DepthBiasSlopeFactor;

	L = glm::vec4(glm::normalize(DirectionalLight.Direction), 1.0f);
	Radiance = glm::vec4(DirectionalLight.Intensity * DirectionalLight.Color, 1.0f);

	VolumeLightingUniformData* VolumeLightingUniformPtr = static_cast<VolumeLightingUniformData*>(VolumeLightingUniform.GetData());
	VolumeLightingUniformPtr->ShadowMapSize = glm::ivec4(ShadowMap.GetWidth(), ShadowMap.GetHeight(), 0, 0);
	VolumeLightingUniformPtr->L = L;
	VolumeLightingUniformPtr->Radiance = Radiance;

	glm::mat4 LightProjMatrix = glm::ortho(-Width * 0.5f, Width * 0.5f, -Width * 0.5f, Width * 0.5f, ZNear, ZFar);
	LightProjMatrix[1][1] *= -1;

	const glm::mat4 LightViewMatrix = glm::lookAt(DirectionalLight.Direction, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	LightViewProjMatrix = LightProjMatrix * LightViewMatrix;
	LightViewProjMatrixInv = glm::inverse(LightViewProjMatrix);

	LightViewProjUniformData* LightViewProjMatrixPtr = static_cast<LightViewProjUniformData*>(LightViewProjBuffer.GetData());
	LightViewProjMatrixPtr->LightViewProj = LightViewProjMatrix;
	LightViewProjMatrixPtr->InvLightViewProj = LightViewProjMatrixInv;
}

template<EMeshType MeshType>
class ShadowDepthVS : public MeshShader<MeshType>
{
	using Base = MeshShader<MeshType>;
public:
	ShadowDepthVS(const ShaderCompilationInfo& CompilationInfo)
		: Base(CompilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		Base::SetEnvironmentVariables(Worker);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo BaseInfo = { "../Shaders/ShadowDepthVS.glsl", "main", EShaderStage::Vertex };
		return BaseInfo;
	}
};

template<EMeshType MeshType>
class ShadowDepthFS : public MeshShader<MeshType>
{
	using Base = MeshShader<MeshType>;
public:
	ShadowDepthFS(const ShaderCompilationInfo& CompilationInfo)
		: Base(CompilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		Base::SetEnvironmentVariables(Worker);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo BaseInfo = { "../Shaders/ShadowDepthFS.glsl", "main", EShaderStage::Fragment };
		return BaseInfo;
	}
};

void ShadowProxy::AddMesh(gpu::Device& Device, gpu::ShaderLibrary& ShaderLibrary, const MeshProxy& MeshProxy)
{
	constexpr EMeshType MeshType = EMeshType::StaticMesh;

	PipelineStateDesc PSODesc = {};
	PSODesc.renderPass = RenderPass;
	PSODesc.shaderStages.vertex = ShaderLibrary.FindShader<ShadowDepthVS<MeshType>>();
	PSODesc.shaderStages.fragment = ShaderLibrary.FindShader<ShadowDepthFS<MeshType>>();
	PSODesc.viewport.width = ShadowMap.GetWidth();
	PSODesc.viewport.height = ShadowMap.GetHeight();
	PSODesc.rasterizationState.depthBiasEnable = true;
	PSODesc.rasterizationState.depthBiasConstantFactor = DepthBiasConstantFactor;
	PSODesc.rasterizationState.depthBiasSlopeFactor = DepthBiasSlopeFactor;
	PSODesc.layouts = { DescriptorSet.GetLayout(), MeshProxy.GetSurfaceSet().GetLayout(), Device.GetTextures().GetLayout(), Device.GetSamplers().GetLayout() };

	const std::vector<VkDescriptorSet> DescriptorSets = { DescriptorSet, MeshProxy.GetSurfaceSet(), Device.GetTextures().GetSet(), Device.GetSamplers().GetSet() };
	MeshDrawCommands.push_back(MeshDrawCommand(Device, MeshProxy, PSODesc, DescriptorSets));
}

void ShadowProxy::Render(gpu::CommandList& CmdList)
{
	CmdList.BeginRenderPass(RenderPass);

	MeshDrawCommand::Draw(CmdList, MeshDrawCommands);

	CmdList.EndRenderPass();

	MeshDrawCommands.clear();
}