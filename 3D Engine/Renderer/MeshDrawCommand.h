#pragma once
#include <DRM.h>
#include "MeshProxy.h"

class MeshDrawCommand
{
public:
	MeshDrawCommand(
		DRMDevice& Device,
		const MeshProxy& MeshProxy,
		PipelineStateDesc& PSODesc)
		: MeshProxy(MeshProxy)
		, DescriptorSets(PSODesc.DescriptorSets)
	{
		PSODesc.SpecializationInfo = MeshProxy.GetSpecializationInfo();
		Pipeline = Device.CreatePipeline(PSODesc);
	}

	void Draw(drm::CommandList& CmdList)
	{
		CmdList.BindPipeline(Pipeline);
		CmdList.BindDescriptorSets(Pipeline, DescriptorSets.size(), DescriptorSets.data());
		MeshProxy.DrawElements(CmdList);
	}

	static void Draw(drm::CommandList& CmdList, std::vector<MeshDrawCommand>& MeshDrawCommands)
	{
		std::for_each(
			MeshDrawCommands.begin(),
			MeshDrawCommands.end(),
			[&] (MeshDrawCommand& MeshDrawCommand)
		{
			MeshDrawCommand.Draw(CmdList);
		});
	}

private:
	const MeshProxy& MeshProxy;
	drm::Pipeline Pipeline;
	std::vector<const drm::DescriptorSet*> DescriptorSets;
};