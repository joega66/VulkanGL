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
	Cursor(Platform& Platform);

	Cursor(const Cursor&) = delete;
	Cursor& operator=(const Cursor&) = delete;

	/** Set end-of-frame updates. */
	void Update(Platform& Platform);

	/** Determines the state of the hardware cursor. */
	ECursorMode Mode = ECursorMode::Normal;

	/** Mouse scroll delta. */
	glm::vec2 MouseScrollDelta;

	/** Mouse scroll speed. */
	float MouseScrollSpeed = 1.0f;

	/** Cursor sensitivity. */
	float Sensitivity = 0.25f;

	/** Position of the cursor last frame. */
	glm::vec2 Last;

	/** Position of the cursor this frame. */
	glm::vec2 Position;

private:
	static void GLFWScrollEvent(struct GLFWwindow* window, double XOffset, double YOffset);

	static void GLFWMouseEvent(struct GLFWwindow* window, double X, double Y);
};