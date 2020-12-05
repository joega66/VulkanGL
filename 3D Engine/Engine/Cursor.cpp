#include "Cursor.h"
#include <GLFW/glfw3.h>

void Cursor::GLFWScrollEvent(GLFWwindow* window, double XOffset, double YOffset)
{
	Cursor* Cursor = static_cast<GLFWWindowUserPointer*>(glfwGetWindowUserPointer(window))->_Cursor;

	Cursor->MouseScrollDelta = glm::vec2(XOffset, YOffset);
}

void Cursor::GLFWMouseEvent(GLFWwindow* window, double X, double Y)
{
	Cursor* Cursor = static_cast<GLFWWindowUserPointer*>(glfwGetWindowUserPointer(window))->_Cursor;

	Cursor->Position = glm::vec2(X, Y);
}

Cursor::Cursor(Platform& Platform)
{
	glfwSetScrollCallback(Platform._Window, GLFWScrollEvent);

	glfwSetCursorPosCallback(Platform._Window, GLFWMouseEvent);
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

	glfwSetInputMode(Platform._Window, GLFW_CURSOR, InputMode);

	MouseScrollDelta = {};

	Last = Position;
}