#include "Cursor.h"
#include <GLFW/glfw3.h>

void Cursor::GLFWScrollEvent(GLFWwindow* Window, double XOffset, double YOffset)
{
	Cursor* Cursor = static_cast<GLFWWindowUserPointer*>(glfwGetWindowUserPointer(Window))->Cursor;

	Cursor->MouseScrollDelta = glm::vec2(XOffset, YOffset);
}

void Cursor::GLFWMouseEvent(GLFWwindow* Window, double X, double Y)
{
	Cursor* Cursor = static_cast<GLFWWindowUserPointer*>(glfwGetWindowUserPointer(Window))->Cursor;

	Cursor->Position = glm::vec2(X, Y);
}

Cursor::Cursor(Platform& Platform)
{
	glfwSetScrollCallback(Platform.Window, GLFWScrollEvent);

	glfwSetCursorPosCallback(Platform.Window, GLFWMouseEvent);
}

void Cursor::Update(Platform& Platform)
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