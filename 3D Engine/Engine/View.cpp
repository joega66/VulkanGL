#include "View.h"

View::View(const glm::vec3 &Position, const glm::vec3 &Up, float Yaw, float Pitch, float MouseSensitivity, float MovementSpeed, float Zoom)
	: Position(Position)
	, WorldUp(Up)
	, Yaw(Yaw)
	, Pitch(Pitch)
	, MouseSensitivity(MouseSensitivity)
	, MovementSpeed(MovementSpeed)
	, ZoomDegree(Zoom)
	, Uniform(GLCreateUniformBuffer<ViewUniforms>())
	, LastXPos(GPlatform->GetMousePosition().x)
	, LastYPos(GPlatform->GetMousePosition().y)
{
	UpdateView();
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
		glm::perspective(glm::radians(ZoomDegree), (float)GPlatform->GetWindowSize().x / GPlatform->GetWindowSize().y, 0.1f, 100.0f)
	};

	// @todo VK_KHR_maintenance1
	View.Projection[1][1] *= -1;

	Uniform->Set(View);
}

glm::mat4 View::GetViewMatrix() const
{
	return glm::lookAt(Position, Position + Front, Up);
}

void View::LookAround(float XPos, float YPos)
{
	float XOffset = XPos - LastXPos;
	float YOffset = LastYPos - YPos;

	LastXPos = XPos;
	LastYPos = YPos;

	XOffset *= MouseSensitivity;
	YOffset *= MouseSensitivity;

	Yaw += XOffset;
	Pitch += YOffset;

	Pitch = std::clamp(Pitch, -89.0f, 89.0f);

	UpdateView();
}

void View::Move(float YOffset)
{
	float DS = MovementSpeed * YOffset;
	Position -= Front * DS;
}