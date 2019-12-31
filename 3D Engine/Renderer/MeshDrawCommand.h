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
		PipelineStateInitializer& PSOInit
		)
	{
		std::for_each(
			MeshDrawCommands.begin(),
			MeshDrawCommands.end(),
			[&] (const MeshDrawCommand& MeshDrawCommand)
		{
			MeshDrawCommand.Draw(CmdList, DescriptorSets, PSOInit);
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
	void Draw(drm::CommandList& CmdList, const DescriptorSetsType& DescriptorSets, PipelineStateInitializer& PSOInit) const
	{
		DescriptorSets.Set(CmdList, MeshProxy);

		PSOInit.SpecializationInfo = MeshProxy.SpecializationInfo;
		PSOInit.ShaderStages = ShaderStages;

		CmdList.BindPipeline(PSOInit);

		MeshProxy.DrawElements(CmdList);
	}

private:
	const MeshProxy& MeshProxy;
	ShaderStages ShaderStages;
};