#pragma once
#include "GL.h"
#include "Physics/Physics.h"

class View
{
public:
	struct ViewUniforms
	{
		glm::mat4 View;
		glm::mat4 Projection;
	};

	GLUniformBufferRef Uniform;

	glm::vec3 Position;
	float MovementSpeed;
	float MouseSensitivity;
	float LastXPos = 0;
	float LastYPos = 0;

	View(const glm::vec3 &Position = glm::vec3(0.0f, 0.0f, 0.0f),
		const glm::vec3 &Up = glm::vec3(0.0f, 1.0f, 0.0f),
		float Yaw = -90.0f, float Pitch = 0.0f,
		float MouseSensitivity = 0.25f, float MovementSpeed = 1.0f,
		float Zoom = 45.0f);

	Ray ScreenPointToRay();

	void SetLastMousePosition();
	void LookAround();
	void Translate();

	glm::mat4 GetViewMatrix() const;
	glm::mat4 GetPerspectiveMatrix() const;

private:
	glm::vec3 Front = glm::vec3(0.0f, 0.0f, 1.0f);
	glm::vec3 Up;
	glm::vec3 Right;
	glm::vec3 WorldUp;

	float Yaw;
	float Pitch;
	float ZoomDegree;

	void UpdateView();
};