#pragma once
#include <Physics/Physics.h>
#include <glm/gtx/quaternion.hpp>

class Camera
{
public:
	Camera(class Screen& Screen, const glm::vec3& Position = glm::vec3(0.0f, 0.0f, 0.0f), const glm::vec3& Up = glm::vec3(0.0f, 1.0f, 0.0f), float FOV = 45.0f);

	/** Get the width of the camera pixels. */
	inline uint32 GetWidth() const { return Width; }

	/** Get the height of the camera pixels. */
	inline uint32 GetHeight() const { return Height; }

	/** Get the camera's aspect ratio. */
	inline float GetAspectRatio() const { return static_cast<float>(Width) / static_cast<float>(Height); }

	/** Transform screen coordinate to world ray. */
	Ray ScreenPointToRay(const glm::vec2& ScreenPosition) const;

	/** Project world coordinate to screen coordinate. */
	bool WorldToScreenCoordinate(const glm::vec3& WorldPosition, glm::vec2& ScreenPosition) const;

	/** Rotate the camera by yaw (x), pitch (y) degrees. */
	void RotateBy(const glm::vec2& Degrees);

	/** Translate the camera forward/backward. */
	void TranslateBy(const float DS);

	/** Rotate the camera to look at the point. */
	void LookAt(const glm::vec3& Point);

	/** Get the World-to-View matrix. */
	inline const glm::mat4& GetWorldToView() const { return WorldToView; }
	/** Get the View-to-Projective matrix. */
	inline const glm::mat4& GetViewToClip() const { return ViewToClip; }
	/** Get the World-to-Projective matrix. */
	inline glm::mat4 GetWorldToClip() const { return GetViewToClip() * GetWorldToView(); }
	/** Get the camera position. */
	inline const glm::vec3& GetPosition() const { return Position; }
	/** Get the camera rotation. */
	inline const glm::quat& GetRotation() const { return Rotation; }
	/** Get the camera forward vector. */
	inline const glm::vec3& GetForward() const { return Forward; }
	/** Get the FOV. */
	inline float GetFOV() const { return FOV; }
	/** Get the combined clipping planes in model space. */
	FrustumPlanes GetFrustumPlanes() const;

	/** Freeze/Unfreeze the camera. */
	bool bFreeze = false;

private:
	/** Camera width. */
	uint32 Width;

	/** Camera height. */
	uint32 Height;
	
	/** Camera position. */
	glm::vec3 Position;
	
	/** Camera rotation. */
	glm::quat Rotation;

	/** Cached world-to-view matrix. */
	glm::mat4 WorldToView;

	/** Camera forward vector. */
	glm::vec3 Forward;

	/** World up vector. */
	glm::vec3 WorldUp;

	/** Camera FOV. */
	float FOV;

	/** Transforms points from view to clip space. */
	glm::mat4 ViewToClip;
};