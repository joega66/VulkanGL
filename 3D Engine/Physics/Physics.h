#pragma once
#include <Platform/Platform.h>
#include <ECS/Entity.h>

class BoundingBox
{
public:
	BoundingBox() = default;

	BoundingBox(const glm::vec3& min, const glm::vec3& max);

	/** Test the point against the current extents. */
	void TestPoint(const glm::vec3& point);

	/** Transform the bounding box by a matrix. */
	BoundingBox Transform(const glm::mat4& transform) const;

	inline const glm::vec3& GetMin() const { return _Min; }
	inline const glm::vec3& GetMax() const { return _Max; }
	inline glm::vec3 GetCenter() const { return (_Max - _Min) / 2.0f + _Min; }

private:
	glm::vec3 _Min = glm::vec3(std::numeric_limits<float>::max());
	glm::vec3 _Max = glm::vec3(std::numeric_limits<float>::min());
};

struct Ray
{
	glm::vec3 _Origin;
	glm::vec3 _Direction;

	Ray(const glm::vec3& origin, const glm::vec3& direction)
		: _Origin(origin), _Direction(direction)
	{
	}

	inline glm::vec3 Intersect(float t) const
	{
		return _Origin + _Direction * t;
	}
};

struct Plane
{
	glm::vec3 _Normal;
	float _Distance;

	static const Plane _XY;
	static const Plane _XZ;
	static const Plane _YZ;
};

using FrustumPlanes = std::array<glm::vec4, 6>;

class EntityManager;

class Physics
{
public:
	/** Ray-box intersection. */
	static bool Raycast(EntityManager& ecs, const Ray& ray, Entity entity, float& t);

	/** Ray-box intersection. */
	static bool Raycast(EntityManager& ecs, const Ray& ray, Entity entity);
	
	/** Ray-plane intersection. */
	static bool Raycast(EntityManager& ecs, const Ray& ray, const Plane& plane, float& t);

	/** Test whether the bounding box is inside the frustum. */
	static bool IsBoxInsideFrustum(const FrustumPlanes& frustumPlanes, const BoundingBox& bb);
};