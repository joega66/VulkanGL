#include "View.h"

View::View(const glm::vec3 &Position, const glm::vec3 &Up, float Yaw, float Pitch, float Zoom)
	: Position(Position)
	, WorldUp(Up)
	, Yaw(Yaw)
	, Pitch(Pitch)
	, ZoomDegree(Zoom)
	, Uniform(GLCreateUniformBuffer<ViewUniforms>(EUniformUpdate::Frequent))
{
	UpdateView();
}

// @todo Move Window to View =) 

Ray View::ScreenPointToRay(const glm::vec2& ScreenPosition)
{
	const glm::vec2 Window = GPlatform->GetWindowSize();

	const glm::vec2 Normalized = glm::vec2(ScreenPosition.x / (float)Window.x, ScreenPosition.y / (float)Window.y);

	const glm::vec2 ScreenSpace = glm::vec2((Normalized.x - 0.5f) * 2.0f, (Normalized.y - 0.5f) * 2.0f);
	
	const glm::vec4 RayStartClipSpace = glm::vec4(ScreenSpace.x, ScreenSpace.y, 0.0f, 1.0f);
	const glm::vec4 RayEndClipSpace = glm::vec4(ScreenSpace.x, ScreenSpace.y, 0.5f, 1.0f);

	const glm::mat4 InvViewProjMatrix = glm::inverse(GetPerspectiveMatrix() * GetViewMatrix());
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

bool View::WorldToScreenCoordinate(const glm::vec3& WorldPosition, glm::vec2& ScreenPosition)
{
	const glm::vec4 ProjectiveSpace = GetPerspectiveMatrix() * GetViewMatrix() * glm::vec4(WorldPosition, 1.0f);
	if (ProjectiveSpace.w > 0.0f)
	{
		const glm::vec2 Window = GPlatform->GetWindowSize();

		const float WInv = 1 / ProjectiveSpace.w;
		const glm::vec4 ClipSpace = glm::vec4(ProjectiveSpace.x * WInv, ProjectiveSpace.y * WInv, ProjectiveSpace.z * WInv, ProjectiveSpace.w);

		// Projective space to normalized [0, 1] space.
		const glm::vec2 Normalized = glm::vec2((ClipSpace.x / 2.0f) + 0.5f, (ClipSpace.y / 2.0f) + 0.5f);
		ScreenPosition = glm::vec2(Normalized.x * Window.x, Normalized.y * Window.y);

		return true;
	}

	return false;
}

void View::UpdateView()
{
	Front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
	Front.y = sin(glm::radians(Pitch));
	Front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
	Front = glm::normalize(Front);
	Right = glm::normalize(glm::cross(Front, WorldUp));
	Up = glm::normalize(glm::cross(Right, Front));

	const ViewUniforms View =
	{
		GetViewMatrix(),
		GetPerspectiveMatrix()
	};

	Uniform->Set(View);
}

glm::mat4 View::GetViewMatrix() const
{
	return glm::lookAt(Position, Position + Front, Up);
}

void View::Axis(const glm::vec2& Offset)
{
	Yaw += Offset.x;
	Pitch += Offset.y;
	Pitch = std::clamp(Pitch, -89.0f, 89.0f);

	UpdateView();
}

void View::Translate(const float DS)
{
	Position += Front * DS;

	UpdateView();
}

glm::mat4 View::GetPerspectiveMatrix() const
{
	glm::mat4 Perspective = glm::perspective(glm::radians(ZoomDegree), (float)GPlatform->GetWindowSize().x / GPlatform->GetWindowSize().y, 0.1f, 100.0f);
	// @todo VK_KHR_maintenance1
	Perspective[1][1] *= -1;
	return Perspective;
}