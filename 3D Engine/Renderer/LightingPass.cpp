#include "LightingPass.h"
#include "MaterialShader.h"
#include "SceneProxy.h"
#include <Engine/Input.h>

template<EMeshType MeshType>
class LightingPassVS : public MaterialShader<MeshType>
{
	using Base = MaterialShader<MeshType>;
public:
	LightingPassVS(const ShaderCompilationInfo& CompilationInfo)
		: Base(CompilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		Base::SetEnvironmentVariables(Worker);
		SceneProxy::SetEnvironmentVariables(Worker);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo BaseInfo = { "../Shaders/LightingPassVS.glsl", "main", EShaderStage::Vertex };
		return BaseInfo;
	}
};

template<EMeshType MeshType>
class LightingPassFS : public MaterialShader<MeshType>
{
	using Base = MaterialShader<MeshType>;
public:
	LightingPassFS(const ShaderCompilationInfo& CompilationInfo)
		: Base(CompilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		Base::SetEnvironmentVariables(Worker);
		SceneProxy::SetEnvironmentVariables(Worker);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo BaseInfo = { "../Shaders/LightingPassFS.glsl", "main", EShaderStage::Fragment };
		return BaseInfo;
	}
};

LightingPassDrawPlan::LightingPassDrawPlan(const MeshElement& Element, CMaterial& Material, drm::UniformBufferRef LocalToWorldUniform)
	: Element(Element)
{
	static constexpr EMeshType MeshType = EMeshType::StaticMesh;
	auto VertShader = *ShaderMapRef<LightingPassVS<MeshType>>();
	auto FragShader = *ShaderMapRef<LightingPassFS<MeshType>>();

	SpecInfo.Add(FragShader->HasSpecularMap, Material.HasSpecularMap());
	SpecInfo.Add(FragShader->HasOpacityMap, Material.IsMasked());

	if (!gInput.GetKeyDown(EKeyCode::Keypad0))
	{
		SpecInfo.Add(FragShader->HasBumpMap, false/*Material.HasBumpMap()*/);
	}
	
	const SamplerState LinearSampler = { EFilter::Linear, ESamplerAddressMode::Repeat, ESamplerMipmapMode::Linear };
	const SamplerState BumpSampler = { EFilter::Linear, ESamplerAddressMode::Repeat, ESamplerMipmapMode::Linear};

	DescriptorSet = drm::CreateDescriptorSet();

	DescriptorSet->Write(LocalToWorldUniform, VertShader->LocalToWorld);
	DescriptorSet->Write(Material.Diffuse, LinearSampler, FragShader->Diffuse);
	DescriptorSet->Write(Material.Specular, LinearSampler, FragShader->Specular);
	DescriptorSet->Write(Material.Opacity, LinearSampler, FragShader->Opacity);
	DescriptorSet->Write(Material.Bump, BumpSampler, FragShader->Bump);

	DescriptorSet->Update();

	this->VertShader = VertShader;
	this->FragShader = FragShader;
}

void LightingPassDrawPlan::BindDescriptorSets(RenderCommandList& CmdList, const SceneProxy& Scene) const
{
	const std::array<drm::DescriptorSetRef, 2> DescriptorSets =
	{
		Scene.DescriptorSet,
		DescriptorSet
	};

	CmdList.BindDescriptorSets(DescriptorSets.size(), DescriptorSets.data());
}

void LightingPassDrawPlan::SetPipelineState(PipelineStateInitializer& PSOInit) const
{
	PSOInit.SpecializationInfos[(int32)EShaderStage::Fragment] = SpecInfo;

	PSOInit.GraphicsPipelineState =
	{
		VertShader,
		nullptr,
		nullptr,
		nullptr,
		FragShader
	};
}

void LightingPassDrawPlan::Draw(RenderCommandList& CmdList) const
{
	CmdList.BindVertexBuffers(Element.VertexBuffers.size(), Element.VertexBuffers.data());
	CmdList.DrawIndexed(Element.IndexBuffer, Element.IndexCount, 1, 0, 0, 0);
}