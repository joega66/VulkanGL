#include "View.h"
#include "Screen.h"

View::View(const glm::vec3 &Position, const glm::vec3 &Up, float Yaw, float Pitch, float FOV)
	: Position(Position)
	, WorldUp(Up)
	, Yaw(Yaw)
	, Pitch(Pitch)
	, FOV(FOV)
{
	Axis({ 0.0f, 0.0f });
	gScreen.RegisterScreenResChangedCallback([&] (uint32 Width, uint32 Height)
	{
		CalcViewToClip(static_cast<float>(Width), static_cast<float>(Height));
	});
}

Ray View::ScreenPointToRay(const glm::vec2& ScreenPosition) const
{ 
	const glm::vec2 Normalized = glm::vec2(ScreenPosition.x / (float)gScreen.GetWidth(), ScreenPosition.y / (float)gScreen.GetHeight());

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

bool View::WorldToScreenCoordinate(const glm::vec3& WorldPosition, glm::vec2& ScreenPosition) const
{
	const glm::vec4 ProjectiveSpace = GetViewToClip() * GetWorldToView() * glm::vec4(WorldPosition, 1.0f);
	if (ProjectiveSpace.w > 0.0f)
	{
		const float WInv = 1.0f / ProjectiveSpace.w;
		const glm::vec4 ClipSpace = glm::vec4(ProjectiveSpace.x * WInv, ProjectiveSpace.y * WInv, ProjectiveSpace.z * WInv, ProjectiveSpace.w);

		// Projective space to normalized [0, 1] space.
		const glm::vec2 Normalized = glm::vec2((ClipSpace.x / 2.0f) + 0.5f, (ClipSpace.y / 2.0f) + 0.5f);
		ScreenPosition = glm::vec2(Normalized.x * gScreen.GetWidth(), Normalized.y * gScreen.GetHeight());

		return true;
	}

	return false;
}

glm::mat4 View::GetWorldToView() const
{
	return glm::lookAt(Position, Position + Front, Up);
}

void View::Axis(const glm::vec2& Offset)
{
	Yaw += Offset.x;
	Pitch += Offset.y;
	Pitch = std::clamp(Pitch, -89.0f, 89.0f);

	Front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
	Front.y = sin(glm::radians(Pitch));
	Front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
	Front = glm::normalize(Front);
	Right = glm::normalize(glm::cross(Front, WorldUp));
	Up = glm::normalize(glm::cross(Right, Front));
}

void View::Translate(const float DS)
{
	Position += Front * DS;
}

FrustumPlanes View::GetFrustumPlanes() const
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

void View::CalcViewToClip(float Width, float Height)
{
	ViewToClip = glm::perspective(glm::radians(FOV), Width / Height, 0.1f, 1000.0f);
	ViewToClip[1][1] *= -1;
}