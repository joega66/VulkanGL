#pragma once
#include "DRMResource.h"

class Transform
{
public:
	Transform(const Transform& Other);

	Transform(const glm::vec3& Position = glm::vec3(0.0f), const glm::vec3& Rotation = glm::vec3(0.0f, 1.0f, 0.0), float Angle = 0.0f, const glm::vec3& Scale = glm::vec3(1.0f));

	const glm::mat4& GetLocalToWorld() const;

	void Translate(const glm::vec3& Position);

	void Rotate(const glm::vec3& Axis, float Angle);

	void Scale(const glm::vec3& ScaleBy);

	void SetParent(Transform* Parent);

	void RemoveChild(Transform* Child);

	inline const glm::vec3& GetPosition() const
	{
		return Position;
	}

	inline const glm::vec3& GetRotation() const
	{
		return Rotation;
	}

	inline const glm::vec3& GetScale() const
	{
		return ScaleBy;
	}

private:
	glm::vec3 Position = glm::vec3(0.0f);

	glm::vec3 Rotation = glm::vec3(0.0f, 1.0f, 0.0);

	float Angle = 0.0f;

	glm::vec3 ScaleBy = glm::vec3(1.0f);

	glm::mat4 LocalToWorld;

	Transform* Parent = nullptr;

	std::list<Transform*> Children;

	void AddChild(Transform* Child);

	glm::mat4 GetLocalToParent();

	void Clean();
};