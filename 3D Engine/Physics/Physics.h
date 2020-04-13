#pragma once
#include <Platform/Platform.h>
#include <ECS/Entity.h>

class BoundingBox
{
public:
	BoundingBox() = default;

	BoundingBox(const glm::vec3& Min, const glm::vec3& Max);

	/** Test the point against the current extents. */
	void TestPoint(const glm::vec3& Point);

	/** Transform the bounding box by a matrix. */
	BoundingBox Transform(const glm::mat4& M) const;

	inline const glm::vec3& GetMin() const { return Min; }
	inline const glm::vec3& GetMax() const { return Max; }
	inline glm::vec3 GetCenter() const { return (Max - Min) / 2.0f + Min; }

private:
	glm::vec3 Min = glm::vec3(std::numeric_limits<float>::max());
	glm::vec3 Max = glm::vec3(std::numeric_limits<float>::min());
};

struct Ray
{
	glm::vec3 Origin;
	glm::vec3 Direction;

	Ray(const glm::vec3& Origin, const glm::vec3& Direction)
		: Origin(Origin), Direction(Direction)
	{
	}

	inline glm::vec3 Intersect(float T) const
	{
		return Origin + Direction * T;
	}
};

struct Plane
{
	glm::vec3 Normal;
	float Distance;

	static const Plane XY;
	static const Plane XZ;
	static const Plane YZ;
};

using FrustumPlanes = std::array<glm::vec4, 6>;

class EntityManager;

class Physics
{
public:
	/** Ray-box intersection. */
	static bool Raycast(EntityManager& ECS, const Ray& Ray, Entity Entity, float& T);

	/** Ray-box intersection. */
	static bool Raycast(EntityManager& ECS, const Ray& Ray, Entity Entity);
	
	/** Ray-plane intersection. */
	static bool Raycast(EntityManager& ECS, const Ray& Ray, const Plane& Plane, float& T);

	/** Test whether the bounding box is inside the frustum. */
	static bool IsBoxInsideFrustum(const FrustumPlanes& FrustumPlanes, const BoundingBox& BB);
};