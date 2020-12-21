#pragma once
#include <Platform/Platform.h>

enum class ECursorMode
{
	Normal,
	Hidden,
	Disabled
};

/** Abstraction of GLFW cursor state. */
class Cursor
{
public:
	/** Set GLFW cursor callbacks. */
	Cursor(Platform& platform);

	Cursor(const Cursor&) = delete;
	Cursor& operator=(const Cursor&) = delete;

	/** Set end-of-frame updates. */
	void Update(Platform& platform);

	/** Determines the state of the hardware cursor. */
	ECursorMode _Mode = ECursorMode::Normal;

	/** Mouse scroll delta. */
	glm::vec2 _MouseScrollDelta;

	/** Mouse scroll speed. */
	float _MouseScrollSpeed = 1.0f;

	/** Cursor sensitivity. */
	float _Sensitivity = 0.25f;

	/** Position of the cursor last frame. */
	glm::vec2 _Last;

	/** Position of the cursor this frame. */
	glm::vec2 _Position;

private:
	static void GLFWScrollEvent(struct GLFWwindow* window, double xOffset, double yOffset);

	static void GLFWMouseEvent(struct GLFWwindow* window, double x, double y);
};