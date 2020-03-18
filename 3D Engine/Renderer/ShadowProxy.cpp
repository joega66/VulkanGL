#include "ShadowProxy.h"
#include <Components/Light.h>
#include <ECS/EntityManager.h>
#include "MaterialShader.h"

UNIFORM_STRUCT(LightViewProjUniformData,
	glm::mat4 LightViewProj;
	glm::mat4 InvLightViewProj;
);

ShadowProxy::ShadowProxy(DRMDevice& Device, DescriptorSetLayout<ShadowDescriptors>& ShadowLayout, const DirectionalLight& DirectionalLight)
{
	LightViewProjBuffer = Device.CreateBuffer(EBufferUsage::Uniform | EBufferUsage::HostVisible, sizeof(LightViewProjUniformData));

	const int32 Resolution = Platform::GetInt("Engine.ini", "Shadows", "Resolution", 2048);
	const glm::ivec2 ShadowMapRes = glm::ivec2(Resolution);

	ShadowMap = Device.CreateImage(ShadowMapRes.x, ShadowMapRes.y, 1, EFormat::D32_SFLOAT, EImageUsage::Attachment | EImageUsage::Sampled);

	glm::ivec4 ShadowMapSize = glm::ivec4(ShadowMapRes, 0, 0);
	ShadowMapSizeBuffer = Device.CreateBuffer(EBufferUsage::Uniform | EBufferUsage::HostVisible, sizeof(ShadowMapSize), &ShadowMapSize);

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
	Descriptors.ShadowMap = drm::ImageView(ShadowMap, Device.CreateSampler({}));
	Descriptors.ShadowMapSizeBuffer = ShadowMapSizeBuffer;
	DescriptorSet = ShadowLayout.CreateDescriptorSet(Device);
	ShadowLayout.UpdateDescriptorSet(Device, DescriptorSet, Descriptors);
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
	PSODesc.Layouts = { DescriptorSet.GetLayout(), MeshProxy.GetSurfaceSet().GetLayout(), MeshProxy.GetMaterialSet().GetLayout() };

	const std::vector<VkDescriptorSet> DescriptorSets = { DescriptorSet, MeshProxy.GetSurfaceSet(), MeshProxy.GetMaterialSet() };
	MeshDrawCommands.push_back(MeshDrawCommand(Device, MeshProxy, PSODesc, DescriptorSets));
}

void ShadowProxy::Render(drm::CommandList& CmdList)
{
	CmdList.BeginRenderPass(RenderPass);

	MeshDrawCommand::Draw(CmdList, MeshDrawCommands);

	CmdList.EndRenderPass();

	MeshDrawCommands.clear();
}