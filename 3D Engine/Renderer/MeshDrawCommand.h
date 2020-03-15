#pragma once
#include <DRM.h>
#include "MeshProxy.h"

class MeshDrawCommand
{
public:
	MeshDrawCommand(
		DRMDevice& Device,
		const MeshProxy& MeshProxy,
		PipelineStateDesc& PSODesc,
		const std::vector<VkDescriptorSet>& DescriptorSets)
		: Submeshes(MeshProxy.GetSubmeshes())
		, DescriptorSets(DescriptorSets)
	{
		PSODesc.SpecializationInfo = MeshProxy.GetSpecializationInfo();
		Pipeline = Device.CreatePipeline(PSODesc);
	}

	void Draw(drm::CommandList& CmdList)
	{
		CmdList.BindPipeline(Pipeline);

		CmdList.BindDescriptorSets(Pipeline, (uint32)DescriptorSets.size(), DescriptorSets.data());

		for (const auto& Submesh : Submeshes)
		{
			CmdList.BindVertexBuffers((uint32)Submesh.GetVertexBuffers().size(), Submesh.GetVertexBuffers().data());

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
	std::shared_ptr<drm::Pipeline> Pipeline;
	std::vector<VkDescriptorSet> DescriptorSets;
	const std::vector<Submesh>& Submeshes;
};