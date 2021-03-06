#pragma once
#include <Physics/Physics.h>
#include <ECS/Component.h>
#include <glm/gtx/quaternion.hpp>

class Camera : public Component
{
public:
	/** Freeze/Unfreeze the camera. */
	bool bFreeze = false;

	Camera(
		const glm::vec3& lookFrom = glm::vec3(0.0f),
		const glm::vec3& lookAt = glm::vec3(0.0f),
		const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f),
		float fieldOfView = 45.0f
	);

	/** Resize the camera. */
	void Resize(uint32 width, uint32 height);

	/** Get the width of the camera pixels. */
	inline uint32 GetWidth() const { return _Width; }

	/** Get the height of the camera pixels. */
	inline uint32 GetHeight() const { return _Height; }

	/** Get the camera's aspect ratio. */
	inline float GetAspectRatio() const { return static_cast<float>(_Width) / static_cast<float>(_Height); }

	/** Transform screen coordinate to world ray. */
	Ray ScreenPointToRay(const glm::vec2& screenPosition) const;

	/** Project world coordinate to screen coordinate. */
	bool WorldToScreenCoordinate(const glm::vec3& worldPosition, glm::vec2& screenPosition) const;

	/** Rotate the camera by yaw (x), pitch (y) degrees. */
	void RotateBy(const glm::vec2& degrees);

	/** Translate the camera forward/backward. */
	void TranslateBy(const float ds);

	/** Rotate the camera to look at the point. */
	void LookAt(const glm::vec3& point);

	/** Set the field-of-view. */
	void SetFieldOfView(float degrees);

	/** Get the World-to-View matrix. */
	inline const glm::mat4& GetWorldToView() const { return _WorldToView; }
	/** Get the View-to-Projective matrix. */
	inline const glm::mat4& GetViewToClip() const { return _ViewToClip; }
	/** Get the World-to-Projective matrix. */
	inline glm::mat4 GetWorldToClip() const { return GetViewToClip() * GetWorldToView(); }
	/** Get the camera position. */
	inline const glm::vec3& GetPosition() const { return _Position; }
	/** Get the camera rotation. */
	inline const glm::quat& GetRotation() const { return _Rotation; }
	/** Get the camera forward vector. */
	inline const glm::vec3& GetForward() const { return _Forward; }
	/** Get the world up vector. */
	inline const glm::vec3& GetWorldUp() const { return _WorldUp; }
	/** Get the FOV. */
	inline float GetFieldOfView() const { return _FieldOfView; }
	/** Get the combined clipping planes in model space. */
	FrustumPlanes GetFrustumPlanes() const;
	/** Get the near/far planes. */
	inline float GetNearPlane() const { return _NearPlane; }
	inline float GetFarPlane() const { return _FarPlane; }

	/** Save state for use in the next frame. */
	void SaveState();

	inline const glm::vec3& GetPrevPosition() const { return _PrevPosition; }
	inline const glm::quat& GetPrevRotation() const { return _PrevRotation; }
	inline const glm::mat4& GetPrevWorldToClip() const { return _PrevWorldToClip; }

private:
	/** Camera width. */
	uint32 _Width;

	/** Camera height. */
	uint32 _Height;
	
	/** Camera position. */
	glm::vec3 _Position;
	
	/** Camera rotation. */
	glm::quat _Rotation;

	/** Cached world-to-view matrix. */
	glm::mat4 _WorldToView;

	/** Camera forward vector. */
	glm::vec3 _Forward;

	/** World up vector. */
	glm::vec3 _WorldUp;

	/** Camera FOV in degrees. */
	float _FieldOfView;

	/** Transforms points from view to clip space. */
	glm::mat4 _ViewToClip;
	
	/** Previous frame's world-to-clip matrix. */
	glm::mat4 _PrevWorldToClip = {};
	
	/** Near and far planes. */
	float _NearPlane;
	float _FarPlane;

	/** Previous state. */
	glm::vec3 _PrevPosition = {};
	glm::quat _PrevRotation = {};

	/** Create the view-to-clip matrix from scratch. */
	void CreateViewToClip();
};