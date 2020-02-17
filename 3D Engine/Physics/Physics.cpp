#include "Physics.h"
#include <ECS/EntityManager.h>
#include <Engine/StaticMesh.h>
#include <Components/StaticMeshComponent.h>
#include <Components/Transform.h>

const Plane Plane::XY = Plane{ glm::vec3(0.0f, 0.0f, 1.0f), 0.0f };
const Plane Plane::YZ = Plane{ glm::vec3(1.0f, 0.0f, 0.0f), 0.0f };
const Plane Plane::XZ = Plane{ glm::vec3(0.0f, 1.0f, 0.0f), 0.0f };

BoundingBox::BoundingBox(const glm::vec3& Min, const glm::vec3& Max)
	: Min(Min)
	, Max(Max)
{
}

void BoundingBox::TestPoint(const glm::vec3& Point)
{
	if (Point.x > Max.x)
		Max.x = Point.x;
	if (Point.y > Max.y)
		Max.y = Point.y;
	if (Point.z > Max.z)
		Max.z = Point.z;
	if (Point.x < Min.x)
		Min.x = Point.x;
	if (Point.y < Min.y)
		Min.y = Point.y;
	if (Point.z < Min.z)
		Min.z = Point.z;
}

BoundingBox BoundingBox::Transform(const glm::mat4& M) const
{
	const glm::vec3 OutMin(M * glm::vec4(Min, 1.0f));
	const glm::vec3 OutMax(M * glm::vec4(Max, 1.0f));
	return BoundingBox(OutMin, OutMax);
}

bool Physics::Raycast(EntityManager& ECS, const Ray& Ray, Entity Entity, float& T)
{
	const StaticMeshComponent& StaticMeshComponent = ECS.GetComponent<class StaticMeshComponent>(Entity);
	const Transform& Transform = ECS.GetComponent<class Transform>(Entity);
	const BoundingBox Bounds = StaticMeshComponent.StaticMesh->GetBounds().Transform(Transform.GetLocalToWorld());
	
	glm::vec3 DirInv = glm::vec3(
		1.0f / Ray.Direction.x,
		1.0f / Ray.Direction.y,
		1.0f / Ray.Direction.z
	);

	float T1 = (Bounds.GetMin().x - Ray.Origin.x) * DirInv.x;
	float T2 = (Bounds.GetMax().x - Ray.Origin.x) * DirInv.x;
	float T3 = (Bounds.GetMin().y - Ray.Origin.y) * DirInv.y;
	float T4 = (Bounds.GetMax().y - Ray.Origin.y) * DirInv.y;
	float T5 = (Bounds.GetMin().z - Ray.Origin.z) * DirInv.z;
	float T6 = (Bounds.GetMax().z - Ray.Origin.z) * DirInv.z;

	float TMin = std::max(std::max(std::min(T1, T2), std::min(T3, T4)), std::min(T5, T6));
	float TMax = std::min(std::min(std::max(T1, T2), std::max(T3, T4)), std::max(T5, T6));

	if (TMax < 0)
	{
		T = TMax;
		return false;
	}

	if (TMin > TMax)
	{
		T = TMax;
		return false;
	}

	T = TMin;
	return true;
}

bool Physics::Raycast(EntityManager& ECS, const Ray& Ray, Entity Entity)
{
	float T;
	return Raycast(ECS, Ray, Entity, T);
}

bool Physics::Raycast(EntityManager& ECS, const Ray& Ray, const Plane& Plane, float& T)
{
	check(Plane.Normal != glm::vec3(0.0f), "Plane should have non-zero normal.");

	glm::vec3 PointOnPlane = [&]()
	{
		if (Plane.Normal.x != 0.0f)
		{
			return glm::vec3(-Plane.Distance / Plane.Normal.x, 0.0f, 0.0f);
		}
		else if (Plane.Normal.y != 0.0f)
		{
			return glm::vec3(0.0f, -Plane.Distance / Plane.Normal.y, 0.0f);
		}
		else //Plane.Normal.z != 0.0f
		{
			return glm::vec3(0.0f, 0.0f, -Plane.Distance / Plane.Normal.z);
		}
	}();

	if (float Denom = glm::dot(Ray.Direction, Plane.Normal); Denom == 0.0f)
	{
		// Line and plane are parallel.
		if (glm::dot(PointOnPlane - Ray.Origin, Plane.Normal) == 0.0f)
		{
			// Line is contained in the plane.
			T = 0;
		}
		else
		{
			// No intersection.
			T = std::numeric_limits<float>().lowest();
		}
		return false;
	}
	else
	{
		// There is a single point of intersection.
		T = (glm::dot(PointOnPlane - Ray.Origin, Plane.Normal)) / Denom;
		return true;
	}
}

bool Physics::IsBoxInsideFrustum(const FrustumPlanes& FrustumPlanes, const BoundingBox& BB)
{
	return std::all_of(FrustumPlanes.begin(), FrustumPlanes.end(), [&] (const glm::vec4& FrustumPlane)
	{
		int32 NumVerticesOutside = 0;
		NumVerticesOutside += ((glm::dot(FrustumPlane, glm::vec4(BB.GetMin().x, BB.GetMin().y, BB.GetMin().z, 1.0)) < 0.0) ? 1 : 0);
		NumVerticesOutside += ((glm::dot(FrustumPlane, glm::vec4(BB.GetMax().x, BB.GetMin().y, BB.GetMin().z, 1.0)) < 0.0) ? 1 : 0);
		NumVerticesOutside += ((glm::dot(FrustumPlane, glm::vec4(BB.GetMin().x, BB.GetMax().y, BB.GetMin().z, 1.0)) < 0.0) ? 1 : 0);
		NumVerticesOutside += ((glm::dot(FrustumPlane, glm::vec4(BB.GetMax().x, BB.GetMax().y, BB.GetMin().z, 1.0)) < 0.0) ? 1 : 0);
		NumVerticesOutside += ((glm::dot(FrustumPlane, glm::vec4(BB.GetMin().x, BB.GetMin().y, BB.GetMax().z, 1.0)) < 0.0) ? 1 : 0);
		NumVerticesOutside += ((glm::dot(FrustumPlane, glm::vec4(BB.GetMax().x, BB.GetMin().y, BB.GetMax().z, 1.0)) < 0.0) ? 1 : 0);
		NumVerticesOutside += ((glm::dot(FrustumPlane, glm::vec4(BB.GetMin().x, BB.GetMax().y, BB.GetMax().z, 1.0)) < 0.0) ? 1 : 0);
		NumVerticesOutside += ((glm::dot(FrustumPlane, glm::vec4(BB.GetMax().x, BB.GetMax().y, BB.GetMax().z, 1.0)) < 0.0) ? 1 : 0);
		return NumVerticesOutside < 8;
	});
}
