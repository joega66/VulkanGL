#pragma once
#include <DRM.h>
#include <ECS/Entity.h>

template<typename DrawPlanType>
class DrawList
{
public:
	inline void Add(Entity Entity, DrawPlanType&& DrawPlan)
	{
		List.emplace(Entity.GetEntityID(), std::forward<DrawPlanType>(DrawPlan));
	}

	inline void Remove(Entity Entity)
	{
		List.erase(Entity.GetEntityID());
	}

	inline void Clear()
	{
		List.clear();
	}
	
	void Draw(RenderCommandList& CmdList, const PipelineStateInitializer& PSOInit, const class SceneProxy& Scene);

private:
	std::multimap<uint64, DrawPlanType> List;
};

template<typename DrawPlanType>
inline void DrawList<DrawPlanType>::Draw(RenderCommandList& CmdList, const PipelineStateInitializer& ParentPSOInit, const class SceneProxy& Scene)
{
	if (List.empty())
	{
		return;
	}

	for (auto& [EntityID, DrawPlan] : List)
	{
		PipelineStateInitializer PSOInit = ParentPSOInit;
		DrawPlan.BindDescriptorSets(CmdList, Scene);
		DrawPlan.SetPipelineState(PSOInit);
		CmdList.BindPipeline(PSOInit);
		DrawPlan.Draw(CmdList);
	}
}