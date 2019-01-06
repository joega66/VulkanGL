#pragma once
#include <Platform/Platform.h>
#include <Components/Entity.h>

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
};

class Physics
{
public:
	static bool Raycast(const Ray& Ray, Entity Entity);
};