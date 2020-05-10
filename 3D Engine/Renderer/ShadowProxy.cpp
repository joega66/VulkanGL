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

ShadowProxy::ShadowProxy(DRMDevice& Device, DescriptorSetLayout<ShadowDescriptors>& ShadowLayout, const DirectionalLight& DirectionalLight)
	: Width(Platform::GetFloat("Engine.ini", "Shadows", "Width", 400.0f))
	, ZNear(Platform::GetFloat("Engine.ini", "Shadows", "ZNear", 1.0f))
	, ZFar(Platform::GetFloat("Engine.ini", "Shadows", "ZFar", 96.0f))
{
	LightViewProjBuffer = Device.CreateBuffer(EBufferUsage::Uniform | EBufferUsage::HostVisible, sizeof(LightViewProjUniformData));

	const int32 Resolution = Platform::GetInt("Engine.ini", "Shadows", "Resolution", 2048);
	const glm::ivec2 ShadowMapRes(Resolution);

	ShadowMap = Device.CreateImage(ShadowMapRes.x, ShadowMapRes.y, 1, EFormat::D32_SFLOAT, EImageUsage::Attachment | EImageUsage::Sampled);

	VolumeLightingUniform = Device.CreateBuffer(EBufferUsage::Uniform | EBufferUsage::HostVisible, sizeof(VolumeLightingUniformData));

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
	Descriptors.LightViewProjBuffer = LightViewProjBuffer;
	Descriptors.VolumeLightingUniform = VolumeLightingUniform;

	DescriptorSet = ShadowLayout.CreateDescriptorSet(Device);
	ShadowLayout.UpdateDescriptorSet(Device, DescriptorSet, Descriptors);
}

void ShadowProxy::Update(DRMDevice& Device, const DirectionalLight& DirectionalLight)
{
	DepthBiasConstantFactor = DirectionalLight.DepthBiasConstantFactor;
	DepthBiasSlopeFactor = DirectionalLight.DepthBiasSlopeFactor;

	VolumeLightingUniformData* VolumeLightingUniformPtr = static_cast<VolumeLightingUniformData*>(VolumeLightingUniform.GetData());
	VolumeLightingUniformPtr->ShadowMapSize = glm::ivec4(ShadowMap.GetWidth(), ShadowMap.GetHeight(), 0, 0);
	VolumeLightingUniformPtr->L = glm::vec4(glm::normalize(DirectionalLight.Direction), 1.0f);
	VolumeLightingUniformPtr->Radiance = glm::vec4(DirectionalLight.Intensity * DirectionalLight.Color, 1.0f);

	glm::mat4 LightProjMatrix = glm::ortho(-Width * 0.5f, Width * 0.5f, -Width * 0.5f, Width * 0.5f, ZNear, ZFar);
	LightProjMatrix[1][1] *= -1;

	const glm::mat4 LightViewMatrix = glm::lookAt(DirectionalLight.Direction, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	const glm::mat4 LightViewProjMatrix = LightProjMatrix * LightViewMatrix;

	LightViewProjUniformData* LightViewProjMatrixPtr = static_cast<LightViewProjUniformData*>(LightViewProjBuffer.GetData());
	LightViewProjMatrixPtr->LightViewProj = LightViewProjMatrix;
	LightViewProjMatrixPtr->InvLightViewProj = glm::inverse(LightViewProjMatrix);
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

void ShadowProxy::AddMesh(DRMDevice& Device, DRMShaderMap& ShaderMap, const MeshProxy& MeshProxy)
{
	constexpr EMeshType MeshType = EMeshType::StaticMesh;

	PipelineStateDesc PSODesc = {};
	PSODesc.RenderPass = RenderPass;
	PSODesc.ShaderStages.Vertex = ShaderMap.FindShader<ShadowDepthVS<MeshType>>();
	PSODesc.ShaderStages.Fragment = ShaderMap.FindShader<ShadowDepthFS<MeshType>>();
	PSODesc.Viewport.Width = ShadowMap.GetWidth();
	PSODesc.Viewport.Height = ShadowMap.GetHeight();
	PSODesc.RasterizationState.DepthBiasEnable = true;
	PSODesc.RasterizationState.DepthBiasConstantFactor = DepthBiasConstantFactor;
	PSODesc.RasterizationState.DepthBiasSlopeFactor = DepthBiasSlopeFactor;
	PSODesc.Layouts = { DescriptorSet.GetLayout(), MeshProxy.GetSurfaceSet().GetLayout(), Device.GetTextures().GetLayout(), Device.GetSamplers().GetLayout() };

	const std::vector<VkDescriptorSet> DescriptorSets = { DescriptorSet, MeshProxy.GetSurfaceSet(), Device.GetTextures().GetSet(), Device.GetSamplers().GetSet() };
	MeshDrawCommands.push_back(MeshDrawCommand(Device, MeshProxy, PSODesc, DescriptorSets));
}

void ShadowProxy::Render(drm::CommandList& CmdList)
{
	CmdList.BeginRenderPass(RenderPass);

	MeshDrawCommand::Draw(CmdList, MeshDrawCommands);

	CmdList.EndRenderPass();

	MeshDrawCommands.clear();
}