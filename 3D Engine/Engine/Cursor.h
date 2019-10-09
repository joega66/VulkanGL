#pragma once
#include <Platform/Platform.h>

enum class ECursorMode
{
	Normal,
	Hidden,
	Disabled
};

class Cursor
{
public:
	// Determines the state of the hardware cursor.
	ECursorMode Mode = ECursorMode::Normal;
	// Mouse scroll delta.
	glm::vec2 MouseScrollDelta;
	// Mouse scroll speed.
	float MouseScrollSpeed = 1.0f;
	// Cursor sensitivity.
	float Sensitivity = 0.25f;
	// Position of the cursor last frame.
	glm::vec2 Last;
	// Position of the cursor this frame.
	glm::vec2 Position;

	Cursor() = default;
	Cursor(const Cursor&) = delete;
	Cursor& operator=(const Cursor&) = delete;

	void Update();
};

extern class Cursor Cursor;