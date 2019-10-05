#include "LightingPass.h"
#include "SceneProxy.h"

LightingPassDrawPlan::LightingPassDrawPlan(const MeshElement& Element, CMaterial& Material, drm::UniformBufferRef LocalToWorldUniform)
	: Element(Element)
{
	VertShader = *ShaderMapRef<LightingPassVS<EMeshType::StaticMesh>>();

	const bool bHasOpacityMap = Material.IsMasked();

	// @todo Pass in the material to ShaderMapRef to do conditional compilations.

	const SamplerState Sampler = { EFilter::Linear, ESamplerAddressMode::Repeat, ESamplerMipmapMode::Linear };

	DescriptorSet = drm::CreateDescriptorSet();

	if (bHasOpacityMap)
	{
		FragShader = *ShaderMapRef<LightingPassFS<true, EMeshType::StaticMesh>>();
		DescriptorSet->Write(Material.Opacity, Sampler, FragShader->Opacity);
	}
	else
	{
		FragShader = *ShaderMapRef<LightingPassFS<false, EMeshType::StaticMesh>>();
	}

	DescriptorSet->Write(LocalToWorldUniform, VertShader->LocalToWorld);
	DescriptorSet->Write(Material.Diffuse, Sampler, FragShader->Diffuse);

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