#include "Cursor.h"
#include <GLFW/glfw3.h>

void Cursor::GLFWScrollEvent(GLFWwindow* window, double xOffset, double yOffset)
{
	Cursor* cursor = static_cast<GLFWWindowUserPointer*>(glfwGetWindowUserPointer(window))->_Cursor;

	cursor->_MouseScrollDelta = glm::vec2(xOffset, yOffset);
}

void Cursor::GLFWMouseEvent(GLFWwindow* window, double x, double y)
{
	Cursor* cursor = static_cast<GLFWWindowUserPointer*>(glfwGetWindowUserPointer(window))->_Cursor;

	cursor->_Position = glm::vec2(x, y);
}

Cursor::Cursor(Platform& platform)
	: _Window(platform._Window)
{
	glfwSetScrollCallback(_Window, GLFWScrollEvent);

	glfwSetCursorPosCallback(_Window, GLFWMouseEvent);
}

void Cursor::Update()
{
	uint32 inputMode;

	switch (_Mode)
	{
	case ECursorMode::Normal:
		inputMode = GLFW_CURSOR_NORMAL;
		break;
	case ECursorMode::Hidden:
		inputMode = GLFW_CURSOR_HIDDEN;
		break;
	case ECursorMode::Disabled:
		inputMode = GLFW_CURSOR_DISABLED;
		break;
	}

	glfwSetInputMode(_Window, GLFW_CURSOR, inputMode);

	_MouseScrollDelta = {};

	_Last = _Position;
}