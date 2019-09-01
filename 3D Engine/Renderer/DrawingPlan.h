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
	
	void Draw(RenderCommandList& CmdList, const PipelineStateInitializer& PSOInit, const class SceneRenderer& SceneRenderer);

private:
	std::multimap<uint64, DrawPlanType> List;
};

template<typename DrawPlanType>
inline void DrawList<DrawPlanType>::Draw(RenderCommandList& CmdList, const PipelineStateInitializer& ParentPSOInit, const class SceneRenderer& SceneRenderer)
{
	if (List.empty())
	{
		return;
	}

	for (auto& [EntityID, DrawPlan] : List)
	{
		PipelineStateInitializer PSOInit = ParentPSOInit;
		DrawPlan.SetPipelineState(PSOInit);

		CmdList.BindPipeline(PSOInit);

		DrawPlan.SetUniforms(CmdList, SceneRenderer);

		DrawPlan.Draw(CmdList);
	}

	Clear();
}