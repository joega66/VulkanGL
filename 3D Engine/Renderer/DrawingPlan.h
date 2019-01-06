#pragma once
#include "GLShader.h"
#include <Engine/View.h>

struct GraphicsPipeline
{
	GLShaderRef Vertex;
	GLShaderRef TessControl;
	GLShaderRef TessEval;
	GLShaderRef Geometry;
	GLShaderRef Fragment;

	GraphicsPipeline() = default;
	GraphicsPipeline(GLShaderRef Vertex, GLShaderRef TessControl, GLShaderRef TessEval, GLShaderRef Geometry, GLShaderRef Fragment)
		: Vertex(Vertex), TessControl(TessControl), TessEval(TessEval), Geometry(Geometry), Fragment(Fragment)
	{
	}
};

struct StreamSource
{
	GLVertexBufferRef VertexBuffer;
	uint32 Location;

	StreamSource() = default;
	StreamSource(GLVertexBufferRef VertexBuffer, uint32 Location)
		: VertexBuffer(VertexBuffer), Location(Location)
	{
	}
};

struct MaterialSource
{
	GLImageRef Image;
	uint32 Location;

	MaterialSource(GLImageRef Image, uint32 Location)
		: Image(Image), Location(Location)
	{
	}
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
}