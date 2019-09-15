#include "EditorControllerSystem.h"
#include <Engine/Scene.h>
#include <Engine/Cursor.h>
#include <Engine/Input.h>

void EditorControllerSystem::Start(Scene& Scene)
{
	Input.AddShortcut("Recompile Shaders", { EKeyCode::LeftControl, EKeyCode::LeftShift, EKeyCode::Period });
}

void EditorControllerSystem::Update(Scene& Scene)
{
	View& View = Scene.View;

	// Translate the view.
	const float DS = Cursor.MouseScrollSpeed * Cursor.MouseScrollDelta.y;
	View.Translate(DS);

	if (Input.GetKeyDown(EKeyCode::MouseLeft))
	{
		// Disable the mouse while looking around.
		Cursor.Mode = ECursorMode::Disabled;

		if (!View.bFreeze)
		{
			// Look around.
			glm::vec2 Offset = glm::vec2(Cursor.Position.x - Cursor.Last.x, Cursor.Last.y - Cursor.Position.y) * Cursor.Sensitivity;
			// Filter noise from glfw cursor jumping after disabling cursor...
			if (glm::abs(Offset.x) < 10 && glm::abs(Offset.y) < 10)
			{
				View.Axis(Offset);
			}
		}
	}
	else if (Input.GetKeyUp(EKeyCode::MouseLeft))
	{
		Cursor.Mode = ECursorMode::Normal;
	}

	if (Input.GetShortcutUp("Recompile Shaders"))
	{
		// @todo
	}
}