#pragma once
#include "DRMResource.h"

class CTransform
{
public:
	CTransform(const CTransform& Other);

	CTransform(const glm::vec3& Position = glm::vec3(0.0f), const glm::vec3& Rotation = glm::vec3(0.0f, 1.0f, 0.0), float Angle = 0.0f, const glm::vec3& Scale = glm::vec3(1.0f));

	const glm::mat4& GetLocalToWorld() const;

	void Translate(const glm::vec3& Position);

	void Rotate(const glm::vec3& Axis, float Angle);

	void Scale(const glm::vec3& ScaleBy);

	void SetParent(CTransform* Parent);

	void RemoveChild(CTransform* Child);

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

	CTransform* Parent = nullptr;

	std::list<CTransform*> Children;

	void AddChild(CTransform* Child);

	glm::mat4 GetLocalToParent();

	void Clean();
};