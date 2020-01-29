#pragma once
#include <DRM.h>
#include "MeshProxy.h"

class MeshDrawCommand
{
public:
	template<typename DescriptorSetsType>
	static void Draw(const std::vector<MeshDrawCommand>& MeshDrawCommands, 
		drm::CommandList& CmdList,
		const DescriptorSetsType& DescriptorSets,
		PipelineStateDesc& PSODesc
		)
	{
		std::for_each(
			MeshDrawCommands.begin(),
			MeshDrawCommands.end(),
			[&] (const MeshDrawCommand& MeshDrawCommand)
		{
			MeshDrawCommand.Draw(CmdList, DescriptorSets, PSODesc);
		});
	}

	MeshDrawCommand(
		ShaderStages&& ShaderStages,
		const MeshProxy& MeshProxy)
		: ShaderStages(std::move(ShaderStages))
		, MeshProxy(MeshProxy)
	{
	}

	template<typename DescriptorSetsType>
	void Draw(drm::CommandList& CmdList, const DescriptorSetsType& DescriptorSets, PipelineStateDesc& PSODesc) const
	{
		DescriptorSets.Set(CmdList, MeshProxy);

		PSODesc.SpecializationInfo = MeshProxy.GetSpecializationInfo();

		PSODesc.ShaderStages = ShaderStages;

		CmdList.BindPipeline(PSODesc);

		MeshProxy.DrawElements(CmdList);
	}

private:
	const MeshProxy& MeshProxy;
	ShaderStages ShaderStages;
};