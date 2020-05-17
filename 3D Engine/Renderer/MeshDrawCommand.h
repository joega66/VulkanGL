#pragma once
#include <DRM.h>
#include "MeshProxy.h"

class MeshDrawCommand
{
public:
	MeshDrawCommand(
		drm::Device& Device,
		const MeshProxy& MeshProxy,
		PipelineStateDesc& PSODesc,
		const std::vector<VkDescriptorSet>& DescriptorSets)
		: Submeshes(MeshProxy.GetSubmeshes())
		, Material(MeshProxy.GetMaterial())
		, DescriptorSets(DescriptorSets)
	{
		PSODesc.SpecializationInfo = MeshProxy.GetSpecializationInfo();
		PSODesc.PushConstantRanges.push_back(Material->GetPushConstantRange());
		Pipeline = Device.CreatePipeline(PSODesc);
	}

	void Draw(drm::CommandList& CmdList)
	{
		CmdList.BindPipeline(Pipeline);

		CmdList.BindDescriptorSets(Pipeline, static_cast<uint32>(DescriptorSets.size()), DescriptorSets.data());

		CmdList.PushConstants(Pipeline, EShaderStage::Fragment, 0, sizeof(Material->GetPushConstants()), &Material->GetPushConstants());

		for (const auto& Submesh : Submeshes)
		{
			CmdList.BindVertexBuffers(static_cast<uint32>(Submesh.GetVertexBuffers().size()), Submesh.GetVertexBuffers().data());

			CmdList.DrawIndexed(Submesh.GetIndexBuffer(), Submesh.GetIndexCount(), 1, 0, 0, 0, Submesh.GetIndexType());
		}
	}

	static void Draw(drm::CommandList& CmdList, std::vector<MeshDrawCommand>& MeshDrawCommands)
	{
		for (auto& MeshDrawCommand : MeshDrawCommands)
		{
			MeshDrawCommand.Draw(CmdList);
		}
	}

private:
	drm::Pipeline Pipeline;
	std::vector<VkDescriptorSet> DescriptorSets;
	const std::vector<Submesh>& Submeshes;
	const Material* Material;
};