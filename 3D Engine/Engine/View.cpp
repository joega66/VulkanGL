#include "View.h"

View::View(const glm::vec3 &Position, const glm::vec3 &Up, float Yaw, float Pitch, float MouseSensitivity, float MovementSpeed, float Zoom)
	: Position(Position)
	, WorldUp(Up)
	, Yaw(Yaw)
	, Pitch(Pitch)
	, MouseSensitivity(MouseSensitivity)
	, MovementSpeed(MovementSpeed)
	, ZoomDegree(Zoom)
	, Uniform(GLCreateUniformBuffer<ViewUniforms>(EUniformUpdate::Frequent))
	, LastXPos(GPlatform->GetMousePosition().x)
	, LastYPos(GPlatform->GetMousePosition().y)
{
	UpdateView();
}

Ray View::ScreenPointToRay(const glm::vec2& ScreenPosition)
{
	const glm::vec2& Window = GPlatform->GetWindowSize();

	const float NormalizedX = ScreenPosition.x / (float)Window.x;
	const float NormalizedY = ScreenPosition.y / (float)Window.y;

	const float ScreenSpaceX = (NormalizedX - 0.5f) * 2.0f;
	const float ScreenSpaceY = (NormalizedY - 0.5f) * 2.0f;

	const glm::vec4 RayStartProjectionSpace = glm::vec4(ScreenSpaceX, ScreenSpaceY, 0.0f, 1.0f);
	const glm::vec4 RayEndProjectionSpace = glm::vec4(ScreenSpaceX, ScreenSpaceY, 0.5f, 1.0f);

	const glm::mat4 InvViewProjMatrix = glm::inverse(GetPerspectiveMatrix() * GetViewMatrix());
	const glm::vec4 HRayStartWorldSpace = InvViewProjMatrix * RayStartProjectionSpace;
	const glm::vec4 HRayEndWorldSpace = InvViewProjMatrix * RayEndProjectionSpace;
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

void View::SetLastMousePosition()
{
	LastXPos = GPlatform->GetMousePosition().x;
	LastYPos = GPlatform->GetMousePosition().y;
}

void View::LookAround()
{
	const glm::vec2& Mouse = GPlatform->GetMousePosition();

	float XOffset = Mouse.x - LastXPos;
	float YOffset = LastYPos - Mouse.y;

	XOffset *= MouseSensitivity;
	YOffset *= MouseSensitivity;

	LastXPos = Mouse.x;
	LastYPos = Mouse.y;

	Yaw += XOffset;
	Pitch += YOffset;

	Pitch = std::clamp(Pitch, -89.0f, 89.0f);

	UpdateView();
}

void View::Translate()
{
	const float DS = MovementSpeed * GPlatform->GetScrollOffset().y;
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
