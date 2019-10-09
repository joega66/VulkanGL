#include "Cursor.h"
#include <GLFW/glfw3.h>

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