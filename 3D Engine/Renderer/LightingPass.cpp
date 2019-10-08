#include "LightingPass.h"
#include "SceneProxy.h"
#include <Engine/Input.h>

LightingPassDrawPlan::LightingPassDrawPlan(const MeshElement& Element, CMaterial& Material, drm::UniformBufferRef LocalToWorldUniform)
	: Element(Element)
{
	static constexpr EMeshType MeshType = EMeshType::StaticMesh;

	VertShader = *ShaderMapRef<LightingPassVS<MeshType>>();
	FragShader = *ShaderMapRef<LightingPassFS<MeshType>>();

	SpecInfo.Add(FragShader->HasSpecularMap, Material.HasSpecularMap());
	SpecInfo.Add(FragShader->HasOpacityMap, Material.IsMasked());

	if (!Input.GetKeyDown(EKeyCode::Keypad0))
	{
		SpecInfo.Add(FragShader->HasBumpMap, Material.HasBumpMap());
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