#pragma once
#include "DRM.h"
#include <Physics/Physics.h>

class View
{
public:
	struct ViewUniform
	{
		glm::mat4 WorldToView;
		glm::mat4 ViewToClip;
		glm::mat4 WorldToClip;
		glm::vec3 Position;
		float _Pad0;
		float AspectRatio;
		float FOV;
		glm::vec2 _Pad1;
	};
	
	drm::UniformBufferRef Uniform;

	View(const glm::vec3 &Position = glm::vec3(0.0f, 0.0f, 0.0f),
		const glm::vec3 &Up = glm::vec3(0.0f, 1.0f, 0.0f),
		float Yaw = -90.0f, float Pitch = 0.0f,
		float FOV = 45.0f);

	// Transform screen coordinate to world ray.
	Ray ScreenPointToRay(const glm::vec2& ScreenPosition) const;
	// Project world coordinate to screen coordinate.
	bool WorldToScreenCoordinate(const glm::vec3& WorldPosition, glm::vec2& ScreenPosition) const;
	// Change view axis.
	void Axis(const glm::vec2& Offset);
	// Translate the view forward/backward.
	void Translate(const float DS);
	// Get the World-to-View matrix.
	glm::mat4 GetWorldToView() const;
	// Get the View-to-Projective matrix.
	glm::mat4 GetViewToClip() const;
	// Get the view position.
	const glm::vec3& GetPosition() const { return Position; }

	// Freeze/Unfreeze the view.
	bool bFreeze = false;

private:
	glm::vec3 Position;
	glm::vec3 Front = glm::vec3(0.0f, 0.0f, 1.0f);

	glm::vec3 Up;
	glm::vec3 Right;
	glm::vec3 WorldUp;

	float Yaw;
	float Pitch;
	float FOV;

	void UpdateView();
};