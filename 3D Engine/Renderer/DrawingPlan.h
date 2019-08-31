#pragma once
#include <DRM.h>
#include <ECS/Entity.h>

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
	
	void Execute(RenderCommandList& CmdList, const PipelineStateInitializer& PSOInit, const class SceneRenderer& SceneRenderer);

private:
	std::multimap<uint64, DrawingPlanType> DrawingPlans;
};

template<typename DrawingPlanType>
inline void DrawingPlanList<DrawingPlanType>::Execute(RenderCommandList& CmdList, const PipelineStateInitializer& ParentPSOInit, const class SceneRenderer& SceneRenderer)
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

		DrawingPlan.SetUniforms(CmdList, SceneRenderer);

		DrawingPlan.Draw(CmdList);
	}

	Clear();
}