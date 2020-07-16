#include "Camera.h"
#include "Screen.h"

Camera::Camera(
	Screen& screen, 
	const glm::vec3& lookFrom,
	const glm::vec3& lookAt,
	const glm::vec3& up, 
	float fov)
	: _Position(lookFrom)
	, _WorldUp(up)
	, _Fov(fov)
	, _NearPlane(0.1f)
	, _FarPlane(1000.0f)
{
	LookAt(lookAt);

	screen.OnScreenResize([&] (uint32 width, uint32 height)
	{
		_Width = width;
		_Height = height;
		_ViewToClip = glm::perspective(glm::radians(_Fov), static_cast<float>(_Width) / static_cast<float>(_Height), _NearPlane, _FarPlane);
		_ViewToClip[1][1] *= -1;
	});
}

Ray Camera::ScreenPointToRay(const glm::vec2& screenPosition) const
{ 
	const glm::vec2 normalized = glm::vec2(screenPosition.x / (float)GetWidth(), screenPosition.y / (float)GetHeight());

	const glm::vec2 screenSpace = glm::vec2((normalized.x - 0.5f) * 2.0f, (normalized.y - 0.5f) * 2.0f);
	
	const glm::vec4 rayStartClipSpace = glm::vec4(screenSpace.x, screenSpace.y, 0.0f, 1.0f);
	const glm::vec4 rayEndClipSpace = glm::vec4(screenSpace.x, screenSpace.y, 0.5f, 1.0f);

	const glm::mat4 invViewProjMatrix = glm::inverse(GetViewToClip() * GetWorldToView());
	const glm::vec4 hRayStartWorldSpace = invViewProjMatrix * rayStartClipSpace;
	const glm::vec4 hRayEndWorldSpace = invViewProjMatrix * rayEndClipSpace;

	glm::vec3 rayStartWorldSpace = glm::vec3(hRayStartWorldSpace);
	glm::vec3 rayEndWorldSpace = glm::vec3(hRayEndWorldSpace);

	if (hRayStartWorldSpace.w != 0.0f)
	{
		rayStartWorldSpace /= hRayStartWorldSpace.w;
	}
	if (hRayEndWorldSpace.w != 0.0f)
	{
		rayEndWorldSpace /= hRayEndWorldSpace.w;
	}

	const glm::vec3 rayDirWorldSpace = glm::normalize(rayEndWorldSpace - rayStartWorldSpace);

	return Ray(rayStartWorldSpace, rayDirWorldSpace);
}

bool Camera::WorldToScreenCoordinate(const glm::vec3& worldPosition, glm::vec2& screenPosition) const
{
	const glm::vec4 projectiveSpace = GetViewToClip() * GetWorldToView() * glm::vec4(worldPosition, 1.0f);
	if (projectiveSpace.w > 0.0f)
	{
		const float wInv = 1.0f / projectiveSpace.w;
		const glm::vec4 clipSpace = glm::vec4(projectiveSpace.x * wInv, projectiveSpace.y * wInv, projectiveSpace.z * wInv, projectiveSpace.w);

		// Projective space to normalized [0, 1] space.
		const glm::vec2 normalized = glm::vec2((clipSpace.x / 2.0f) + 0.5f, (clipSpace.y / 2.0f) + 0.5f);
		screenPosition = glm::vec2(normalized.x * GetWidth(), normalized.y * GetHeight());

		return true;
	}

	return false;
}

void Camera::RotateBy(const glm::vec2& degrees)
{
	const glm::quat yaw = glm::normalize(glm::angleAxis(glm::radians(degrees.x), glm::vec3(0, 1, 0)));
	const glm::quat pitch = glm::normalize(glm::angleAxis(glm::radians(degrees.y), glm::vec3(1, 0, 0)));
	const glm::mat4 translation = glm::translate(glm::mat4(1.0f), -_Position);
	_Rotation = glm::normalize(pitch * _Rotation * yaw);
	_WorldToView = glm::mat4_cast(_Rotation) * translation;
	_Forward = glm::normalize(glm::vec3(_WorldToView[0][2], _WorldToView[1][2], _WorldToView[2][2]));
}

void Camera::TranslateBy(const float ds)
{
	_Position += -_Forward * ds;
	const glm::mat4 translation = glm::translate(glm::mat4(1.0f), -_Position);
	_WorldToView = glm::mat4_cast(_Rotation) * translation;
	_Forward = glm::normalize(glm::vec3(_WorldToView[0][2], _WorldToView[1][2], _WorldToView[2][2]));
}

void Camera::LookAt(const glm::vec3& point)
{
	_Forward = glm::normalize(point - _Position);
	const glm::vec3 right = glm::normalize(glm::cross(_Forward, _WorldUp));
	const glm::vec3 up = glm::normalize(glm::cross(right, _Forward));
	_WorldToView = glm::lookAt(_Position, _Position + _Forward, up);
	_Rotation = glm::quat_cast(_WorldToView);
}

FrustumPlanes Camera::GetFrustumPlanes() const
{
	const glm::mat4 worldToClip = glm::transpose(GetWorldToClip());
	const FrustumPlanes frustumPlanes =
	{
		worldToClip[3] + worldToClip[0],
		worldToClip[3] - worldToClip[0],
		worldToClip[3] + worldToClip[1],
		worldToClip[3] - worldToClip[1],
		worldToClip[3] + worldToClip[2],
		worldToClip[3] - worldToClip[2]
	};
	return frustumPlanes;
}

void Camera::SaveState()
{
	_PrevPosition = _Position;
	_PrevRotation = _Rotation;
}