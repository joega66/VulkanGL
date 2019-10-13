#include "Cursor.h"
#include <GLFW/glfw3.h>

Cursor gCursor;

void Cursor::ScrollCallback(GLFWwindow* Window, double XOffset, double YOffset)
{
	gCursor.MouseScrollDelta = glm::vec2(XOffset, YOffset);
}

void Cursor::MouseCallback(GLFWwindow* Window, double X, double Y)
{
	gCursor.Position = glm::vec2(X, Y);
}

void Cursor::Init() const
{
	glfwSetScrollCallback(Platform.Window, ScrollCallback);
	glfwSetCursorPosCallback(Platform.Window, MouseCallback);
}

void Cursor::Update()
{
	uint32 InputMode;

	switch (Mode)
	{
	case ECursorMode::Normal:
		InputMode = GLFW_CURSOR_NORMAL;
		break;
	case ECursorMode::Hidden:
		InputMode = GLFW_CURSOR_HIDDEN;
		break;
	case ECursorMode::Disabled:
		InputMode = GLFW_CURSOR_DISABLED;
		break;
	}

	glfwSetInputMode(Platform.Window, GLFW_CURSOR, InputMode);

	MouseScrollDelta = {};
	Last = Position;
}