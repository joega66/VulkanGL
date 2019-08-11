#pragma once
#include <Engine/View.h>
#include <DRM.h>

struct StreamSource
{
	drm::VertexBufferRef VertexBuffer;
	uint32 Location;
};

struct MaterialSource
{
	drm::ImageRef Image;
	uint32 Location;
};

struct UniformSource
{
	drm::ShaderRef Shader;
	drm::UniformBufferRef UniformBuffer;
	uint32 Location;
};

template<typename DrawingPlanType>
class DrawingPlanList
{
public:
	inline void Add(Entity Entity, DrawingPlanType&& DrawingPlan)
	{
		DrawingPlans.emplace(Entity.GetEntityID(), std::forward<DrawingPlanType>(DrawingPlan));
	}

	inline void Remove(Entity Entity)
	{
		DrawingPlans.erase(Entity.GetEntityID());
	}

	inline void Clear()
	{
		DrawingPlans.clear();
	}
	
	void Execute(RenderCommandList& CmdList, const PipelineStateInitializer& PSOInit, const class Scene& Scene);

private:
	std::multimap<uint64, DrawingPlanType> DrawingPlans;
};

template<typename DrawingPlanType>
inline void DrawingPlanList<DrawingPlanType>::Execute(RenderCommandList& CmdList, const PipelineStateInitializer& ParentPSOInit, const class Scene& Scene)
{
	if (DrawingPlans.empty())
	{
		return;
	}

	for (auto& [EntityID, DrawingPlan] : DrawingPlans)
	{
		PipelineStateInitializer PSOInit = ParentPSOInit;
		DrawingPlan.SetPipelineState(PSOInit);

		CmdList.BindPipeline(PSOInit);

		DrawingPlan.SetUniforms(CmdList, Scene);

		DrawingPlan.Draw(CmdList);
	}

	Clear();
}