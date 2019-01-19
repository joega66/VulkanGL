#pragma once
#include "GLShader.h"
#include <Engine/View.h>

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
	
	void Execute(const View& View);

private:
	std::multimap<uint64, DrawingPlanType> DrawingPlans;
};

template<typename DrawingPlanType>
inline void DrawingPlanList<DrawingPlanType>::Execute(const View& View)
{
	for (auto& [EntityID, DrawingPlan] : DrawingPlans)
	{
		const GraphicsPipeline Pipeline = DrawingPlan.GetGraphicsPipeline();

		GLSetGraphicsPipeline(
			Pipeline.Vertex
			, Pipeline.TessControl
			, Pipeline.TessEval
			, Pipeline.Geometry
			, Pipeline.Fragment);

		DrawingPlan.SetUniforms(View);

		DrawingPlan.Draw();
	}

	Clear();
}