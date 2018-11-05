#pragma once
#include "Component.h"
#include "GLRenderResource.h"

class CTransform : public Component
{
public:
	GLUniformBufferRef LocalToWorldUniform;

	CTransform(glm::vec3&& Position = glm::vec3(0.0f), glm::vec3&& Rotation = glm::vec3(0.0f, 1.0f, 0.0), float Angle = 0.0f, glm::vec3&& Scale = glm::vec3(1.0f));
	~CTransform();

	const glm::mat4& GetLocalToWorld();
	void Translate(glm::vec3&& Position);
	void Rotate(glm::vec3&& Axis, float Angle);
	void Scale(glm::vec3&& S);
	void SetParent(CTransform* Parent);
	void AddChild(CTransform* Child);
	void RemoveChild(CTransform* Child);
	bool IsDirty() const;

private:
	glm::vec3 Position = glm::vec3(0.0f);
	glm::vec3 Rotation = glm::vec3(0.0f, 1.0f, 0.0);
	float Angle = 0.0f;
	glm::vec3 ScaleBy = glm::vec3(1.0f);

	glm::mat4 LocalToParent;
	glm::mat4 LocalToWorld;

	CTransform* Parent = nullptr;
	std::list<CTransform*> Children;
	bool bDirty = true;

	const glm::mat4& GetLocalToParent();
	void MarkDirty();
};

CLASS(CTransform);