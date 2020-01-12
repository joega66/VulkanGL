#pragma once
#include <Physics/Physics.h>

class View
{
public:
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
	inline const glm::mat4& GetViewToClip() const { return ViewToClip; }

	/** Get the World-to-Projective matrix. */
	inline glm::mat4 GetWorldToClip() const { return GetViewToClip() * GetWorldToView(); }

	// Get the view position.
	const glm::vec3& GetPosition() const { return Position; }
	// Get the FOV.
	float GetFOV() const { return FOV; }

	// Freeze/Unfreeze the view.
	bool bFreeze = false;

	/** Get the combined clipping planes in model space. */
	FrustumPlanes GetFrustumPlanes() const;

private:
	glm::vec3 Position;
	glm::vec3 Front = glm::vec3(0.0f, 0.0f, 1.0f);

	glm::vec3 Up;
	glm::vec3 Right;
	glm::vec3 WorldUp;

	float Yaw;
	float Pitch;
	float FOV;

	/** Transforms points from view to clip space. */
	glm::mat4 ViewToClip;

	void CalcViewToClip(float Width, float Height);
};