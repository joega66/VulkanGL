#pragma once
#include <Engine/View.h>
#include <DRM.h>

struct StreamSource
{
	GLVertexBufferRef VertexBuffer;
	uint32 Location;
};

struct MaterialSource
{
	GLImageRef Image;
	uint32 Location;
};

struct UniformSource
{
	GLShaderRef Shader;
	GLUniformBufferRef UniformBuffer;
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
	
	void Execute(RenderCommandList& CmdList, const PipelineStateInitializer& PSOInit, const View& View);

private:
	std::multimap<uint64, DrawingPlanType> DrawingPlans;
};

template<typename DrawingPlanType>
inline void DrawingPlanList<DrawingPlanType>::Execute(RenderCommandList& CmdList, const PipelineStateInitializer& ParentPSOInit, const View& View)
{
	if (DrawingPlans.empty())
	{
		return;
	}

	for (auto& [EntityID, DrawingPlan] : DrawingPlans)
	{
		PipelineStateInitializer PSOInit = ParentPSOInit;
		
		DrawingPlan.GetPipelineState(PSOInit);

		const GraphicsPipeline Pipeline = DrawingPlan.GetGraphicsPipeline();

		CmdList.SetPipelineState(PSOInit);

		CmdList.SetGraphicsPipeline(
			Pipeline.Vertex
			, Pipeline.TessControl
			, Pipeline.TessEval
			, Pipeline.Geometry
			, Pipeline.Fragment);

		DrawingPlan.SetUniforms(CmdList, View);

		DrawingPlan.Draw(CmdList);
	}

	Clear();
}