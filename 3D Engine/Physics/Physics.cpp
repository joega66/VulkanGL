#include "Physics.h"
#include <ECS/EntityManager.h>
#include <Engine/StaticMesh.h>
#include <Components/StaticMeshComponent.h>
#include <Components/Transform.h>

const Plane Plane::_XY = Plane{ glm::vec3(0.0f, 0.0f, 1.0f), 0.0f };
const Plane Plane::_YZ = Plane{ glm::vec3(1.0f, 0.0f, 0.0f), 0.0f };
const Plane Plane::_XZ = Plane{ glm::vec3(0.0f, 1.0f, 0.0f), 0.0f };

BoundingBox::BoundingBox(const glm::vec3& min, const glm::vec3& max)
	: _Min(min)
	, _Max(max)
{
}

void BoundingBox::TestPoint(const glm::vec3& point)
{
	if (point.x > _Max.x)
		_Max.x = point.x;
	if (point.y > _Max.y)
		_Max.y = point.y;
	if (point.z > _Max.z)
		_Max.z = point.z;
	if (point.x < _Min.x)
		_Min.x = point.x;
	if (point.y < _Min.y)
		_Min.y = point.y;
	if (point.z < _Min.z)
		_Min.z = point.z;
}

BoundingBox BoundingBox::Transform(const glm::mat4& transform) const
{
	const glm::vec3 min(transform * glm::vec4(_Min, 1.0f));
	const glm::vec3 max(transform * glm::vec4(_Max, 1.0f));
	return BoundingBox(min, max);
}

bool Physics::Raycast(EntityManager& ecs, const Ray& ray, Entity entity, float& t)
{
	const StaticMeshComponent& staticMeshComponent = ecs.GetComponent<StaticMeshComponent>(entity);
	const Transform& transform = ecs.GetComponent<class Transform>(entity);
	const BoundingBox bounds = staticMeshComponent.StaticMesh->GetBounds().Transform(transform.GetLocalToWorld());
	
	const glm::vec3 dirInv = glm::vec3(
		1.0f / ray._Direction.x,
		1.0f / ray._Direction.y,
		1.0f / ray._Direction.z
	);

	const float t1 = (bounds.GetMin().x - ray._Origin.x) * dirInv.x;
	const float t2 = (bounds.GetMax().x - ray._Origin.x) * dirInv.x;
	const float t3 = (bounds.GetMin().y - ray._Origin.y) * dirInv.y;
	const float t4 = (bounds.GetMax().y - ray._Origin.y) * dirInv.y;
	const float t5 = (bounds.GetMin().z - ray._Origin.z) * dirInv.z;
	const float t6 = (bounds.GetMax().z - ray._Origin.z) * dirInv.z;

	const float tMin = std::max(std::max(std::min(t1, t2), std::min(t3, t4)), std::min(t5, t6));
	const float tMax = std::min(std::min(std::max(t1, t2), std::max(t3, t4)), std::max(t5, t6));

	if (tMax < 0)
	{
		t = tMax;
		return false;
	}

	if (tMin > tMax)
	{
		t = tMax;
		return false;
	}

	t = tMin;
	return true;
}

bool Physics::Raycast(EntityManager& ecs, const Ray& ray, Entity entity)
{
	float t;
	return Raycast(ecs, ray, entity, t);
}

bool Physics::Raycast(EntityManager& ecs, const Ray& ray, const Plane& plane, float& t)
{
	check(plane._Normal != glm::vec3(0.0f), "Plane should have non-zero normal.");

	const glm::vec3 pointOnPlane = [&]()
	{
		if (plane._Normal.x != 0.0f)
		{
			return glm::vec3(-plane._Distance / plane._Normal.x, 0.0f, 0.0f);
		}
		else if (plane._Normal.y != 0.0f)
		{
			return glm::vec3(0.0f, -plane._Distance / plane._Normal.y, 0.0f);
		}
		else //plane.Normal.z != 0.0f
		{
			return glm::vec3(0.0f, 0.0f, -plane._Distance / plane._Normal.z);
		}
	}();

	if (const float denom = glm::dot(ray._Direction, plane._Normal); denom == 0.0f)
	{
		// Line and plane are parallel.
		if (glm::dot(pointOnPlane - ray._Origin, plane._Normal) == 0.0f)
		{
			// Line is contained in the plane.
			t = 0;
		}
		else
		{
			// No intersection.
			t = std::numeric_limits<float>().lowest();
		}
		return false;
	}
	else
	{
		// There is a single point of intersection.
		t = (glm::dot(pointOnPlane - ray._Origin, plane._Normal)) / denom;
		return true;
	}
}

bool Physics::IsBoxInsideFrustum(const FrustumPlanes& frustumPlanes, const BoundingBox& bb)
{
	return std::all_of(frustumPlanes.begin(), frustumPlanes.end(), [&] (const glm::vec4& FrustumPlane)
	{
		int32 numVerticesOutside = 0;
		numVerticesOutside += ((glm::dot(FrustumPlane, glm::vec4(bb.GetMin().x, bb.GetMin().y, bb.GetMin().z, 1.0)) < 0.0) ? 1 : 0);
		numVerticesOutside += ((glm::dot(FrustumPlane, glm::vec4(bb.GetMax().x, bb.GetMin().y, bb.GetMin().z, 1.0)) < 0.0) ? 1 : 0);
		numVerticesOutside += ((glm::dot(FrustumPlane, glm::vec4(bb.GetMin().x, bb.GetMax().y, bb.GetMin().z, 1.0)) < 0.0) ? 1 : 0);
		numVerticesOutside += ((glm::dot(FrustumPlane, glm::vec4(bb.GetMax().x, bb.GetMax().y, bb.GetMin().z, 1.0)) < 0.0) ? 1 : 0);
		numVerticesOutside += ((glm::dot(FrustumPlane, glm::vec4(bb.GetMin().x, bb.GetMin().y, bb.GetMax().z, 1.0)) < 0.0) ? 1 : 0);
		numVerticesOutside += ((glm::dot(FrustumPlane, glm::vec4(bb.GetMax().x, bb.GetMin().y, bb.GetMax().z, 1.0)) < 0.0) ? 1 : 0);
		numVerticesOutside += ((glm::dot(FrustumPlane, glm::vec4(bb.GetMin().x, bb.GetMax().y, bb.GetMax().z, 1.0)) < 0.0) ? 1 : 0);
		numVerticesOutside += ((glm::dot(FrustumPlane, glm::vec4(bb.GetMax().x, bb.GetMax().y, bb.GetMax().z, 1.0)) < 0.0) ? 1 : 0);
		return numVerticesOutside < 8;
	});
}
