#include "Camera.h"
#include "Screen.h"

Camera::Camera(Screen& Screen, const glm::vec3& Position, const glm::vec3& Up, float InFOV)
	: Position(Position)
	, WorldUp(Up)
	, FOV(InFOV)
{
	RotateBy({ 0.0f, 0.0f });
	Screen.OnScreenResize([&] (uint32 InWidth, uint32 InHeight)
	{
		Width = InWidth;
		Height = InHeight;
		ViewToClip = glm::perspective(glm::radians(FOV), static_cast<float>(Width) / static_cast<float>(Height), 0.1f, 1000.0f);
		ViewToClip[1][1] *= -1;
	});
}

Ray Camera::ScreenPointToRay(const glm::vec2& ScreenPosition) const
{ 
	const glm::vec2 Normalized = glm::vec2(ScreenPosition.x / (float)GetWidth(), ScreenPosition.y / (float)GetHeight());

	const glm::vec2 ScreenSpace = glm::vec2((Normalized.x - 0.5f) * 2.0f, (Normalized.y - 0.5f) * 2.0f);
	
	const glm::vec4 RayStartClipSpace = glm::vec4(ScreenSpace.x, ScreenSpace.y, 0.0f, 1.0f);
	const glm::vec4 RayEndClipSpace = glm::vec4(ScreenSpace.x, ScreenSpace.y, 0.5f, 1.0f);

	const glm::mat4 InvViewProjMatrix = glm::inverse(GetViewToClip() * GetWorldToView());
	const glm::vec4 HRayStartWorldSpace = InvViewProjMatrix * RayStartClipSpace;
	const glm::vec4 HRayEndWorldSpace = InvViewProjMatrix * RayEndClipSpace;

	glm::vec3 RayStartWorldSpace = glm::vec3(HRayStartWorldSpace);
	glm::vec3 RayEndWorldSpace = glm::vec3(HRayEndWorldSpace);

	if (HRayStartWorldSpace.w != 0.0f)
	{
		RayStartWorldSpace /= HRayStartWorldSpace.w;
	}
	if (HRayEndWorldSpace.w != 0.0f)
	{
		RayEndWorldSpace /= HRayEndWorldSpace.w;
	}

	const glm::vec3 RayDirWorldSpace = glm::normalize(RayEndWorldSpace - RayStartWorldSpace);

	return Ray(RayStartWorldSpace, RayDirWorldSpace);
}

bool Camera::WorldToScreenCoordinate(const glm::vec3& WorldPosition, glm::vec2& ScreenPosition) const
{
	const glm::vec4 ProjectiveSpace = GetViewToClip() * GetWorldToView() * glm::vec4(WorldPosition, 1.0f);
	if (ProjectiveSpace.w > 0.0f)
	{
		const float WInv = 1.0f / ProjectiveSpace.w;
		const glm::vec4 ClipSpace = glm::vec4(ProjectiveSpace.x * WInv, ProjectiveSpace.y * WInv, ProjectiveSpace.z * WInv, ProjectiveSpace.w);

		// Projective space to normalized [0, 1] space.
		const glm::vec2 Normalized = glm::vec2((ClipSpace.x / 2.0f) + 0.5f, (ClipSpace.y / 2.0f) + 0.5f);
		ScreenPosition = glm::vec2(Normalized.x * GetWidth(), Normalized.y * GetHeight());

		return true;
	}

	return false;
}

void Camera::RotateBy(const glm::vec2& Degrees)
{
	const glm::quat Yaw = glm::normalize(glm::angleAxis(glm::radians(Degrees.x), glm::vec3(0, 1, 0)));
	const glm::quat Pitch = glm::normalize(glm::angleAxis(glm::radians(Degrees.y), glm::vec3(1, 0, 0)));
	const glm::mat4 Translation = glm::translate(glm::mat4(1.0f), -Position);
	Rotation = glm::normalize(Pitch * Rotation * Yaw);
	WorldToView = glm::mat4_cast(Rotation) * Translation;
	Forward = glm::normalize(glm::vec3(WorldToView[0][2], WorldToView[1][2], WorldToView[2][2]));
}

void Camera::TranslateBy(const float DS)
{
	Position += -Forward * DS;
	const glm::mat4 Translation = glm::translate(glm::mat4(1.0f), -Position);
	WorldToView = glm::mat4_cast(Rotation) * Translation;
	Forward = glm::normalize(glm::vec3(WorldToView[0][2], WorldToView[1][2], WorldToView[2][2]));
}

void Camera::LookAt(const glm::vec3& Point)
{
	Forward = glm::normalize(Point - Position);
	const glm::vec3 Right = glm::normalize(glm::cross(Forward, WorldUp));
	const glm::vec3 Up = glm::normalize(glm::cross(Right, Forward));
	WorldToView = glm::lookAt(Position, Position + Forward, Up);
	Rotation = glm::quat_cast(WorldToView);
}

FrustumPlanes Camera::GetFrustumPlanes() const
{
	const glm::mat4 WorldToClip = glm::transpose(GetWorldToClip());
	const FrustumPlanes FrustumPlanes =
	{
		WorldToClip[3] + WorldToClip[0],
		WorldToClip[3] - WorldToClip[0],
		WorldToClip[3] + WorldToClip[1],
		WorldToClip[3] - WorldToClip[1],
		WorldToClip[3] + WorldToClip[2],
		WorldToClip[3] - WorldToClip[2]
	};
	return FrustumPlanes;
}