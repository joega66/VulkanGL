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

Ray View::ScreenPointToRay()
{
	const glm::vec2& Window = GPlatform->GetWindowSize();
	const glm::vec2& Mouse = GPlatform->GetMousePosition();

	float X = (2.0f * Mouse.x) / Window.x - 1.0f;
	float Y = (2.0f * Mouse.y) / Window.y - 1.0f;
	float Z = 1.0f;

	glm::vec3 RayNdc = glm::vec3(X, Y, Z);
	glm::vec4 RayClip = glm::vec4(RayNdc.x, RayNdc.y, RayNdc.z, 1.0);
	glm::vec4 RayEye = glm::inverse(GetPerspectiveMatrix()) * RayClip;
	glm::vec3 RayWorld = glm::vec3(glm::inverse(GetViewMatrix()) * RayEye);
	RayWorld = glm::normalize(RayWorld);

	return Ray(Position, RayWorld);
}

void View::UpdateView()
{
	Front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
	Front.y = sin(glm::radians(Pitch));
	Front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
	Front = glm::normalize(Front);
	Right = glm::normalize(glm::cross(Front, WorldUp));
	Up = glm::normalize(glm::cross(Right, Front));

	ViewUniforms View =
	{
		GetViewMatrix(),
		GetPerspectiveMatrix()
	};

	// @todo VK_KHR_maintenance1
	//View.Projection[1][1] *= -1;

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

	LastXPos = Mouse.x;
	LastYPos = Mouse.y;

	XOffset *= MouseSensitivity;
	YOffset *= MouseSensitivity;

	Yaw += XOffset;
	Pitch += YOffset;

	Pitch = std::clamp(Pitch, -89.0f, 89.0f);

	UpdateView();
}

void View::Translate()
{
	float DS = MovementSpeed * GPlatform->GetScrollOffset().y;
	Position += Front * DS;

	UpdateView();
}

glm::mat4 View::GetPerspectiveMatrix() const
{
	glm::mat4 Perspective = glm::perspective(glm::radians(ZoomDegree), (float)GPlatform->GetWindowSize().x / GPlatform->GetWindowSize().y, 0.1f, 100.0f);
	Perspective[1][1] *= -1;
	return Perspective;
}
