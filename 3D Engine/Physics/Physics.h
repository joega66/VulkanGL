#pragma once
#include <Platform/Platform.h>
#include <ECS/Entity.h>

struct BoundingBox
{
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

	glm::vec3 Intersect(float T) const
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

class Physics
{
public:
	static bool Raycast(const Ray& Ray, Entity Entity, float& T);
	static bool Raycast(const Ray& Ray, Entity Entity);
	static bool Raycast(const Ray& Ray, const Plane& Plane, float& T);
};