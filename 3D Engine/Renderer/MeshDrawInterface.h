#pragma once
#include <DRM.h>
#include "MeshProxy.h"

template<typename MeshDrawType>
class MeshDrawInterface
{
	using PassDescriptorsType = typename MeshDrawType::PassDescriptors;

public:

	inline void Add(const MeshProxy& MeshProxy, MeshDrawType&& InMeshDrawType)
	{
		MeshProxies.emplace_back(&MeshProxy);
		MeshDrawTypes.emplace_back(std::forward<MeshDrawType>(InMeshDrawType));
	}

	void Draw(RenderCommandList& CmdList, const PipelineStateInitializer& ParentPSOInit, const PassDescriptorsType& PassDescriptors);

private:
	std::vector<const MeshProxy*> MeshProxies;
	std::vector<MeshDrawType> MeshDrawTypes;
};

template<typename MeshDrawType>
inline void MeshDrawInterface<MeshDrawType>::Draw(
	RenderCommandList& CmdList, 
	const PipelineStateInitializer& ParentPSOInit, 
	const PassDescriptorsType& PassDescriptors)
{
	for (uint32 MeshDrawIndex = 0; MeshDrawIndex < MeshProxies.size(); MeshDrawIndex++)
	{
		const MeshProxy& MeshProxy = *MeshProxies[MeshDrawIndex];
		const MeshDrawType& DrawType = MeshDrawTypes[MeshDrawIndex];

		PipelineStateInitializer PSOInit = ParentPSOInit;
		DrawType.BindDescriptorSets(CmdList, MeshProxy, PassDescriptors);
		DrawType.SetPipelineState(PSOInit, MeshProxy);
		CmdList.BindPipeline(PSOInit);

		for (auto& MeshElement : MeshProxy.Elements)
		{
			DrawType.Draw(CmdList, MeshElement);
		}
	}
}