#include "EditorControllerSystem.h"
#include <Engine/Scene.h>
#include <Engine/Cursor.h>
#include <Engine/Input.h>

void EditorControllerSystem::Start(Scene& Scene)
{
	gInput.AddShortcut("Recompile Shaders", { EKeyCode::LeftControl, EKeyCode::LeftShift, EKeyCode::Period });
}

void EditorControllerSystem::Update(Scene& Scene)
{
	View& View = Scene.View;

	// Translate the view.
	const float DS = gCursor.MouseScrollSpeed * gCursor.MouseScrollDelta.y;
	View.Translate(DS);

	if (gInput.GetKeyDown(EKeyCode::MouseLeft))
	{
		// Disable the mouse while looking around.
		gCursor.Mode = ECursorMode::Disabled;

		if (!View.bFreeze)
		{
			// Look around.
			glm::vec2 Offset = glm::vec2(gCursor.Position.x - gCursor.Last.x, gCursor.Last.y - gCursor.Position.y) * gCursor.Sensitivity;
			View.Axis(Offset);
		}
	}
	else if (gInput.GetKeyUp(EKeyCode::MouseLeft))
	{
		gCursor.Mode = ECursorMode::Normal;
	}

	if (gInput.GetShortcutUp("Recompile Shaders"))
	{
		drm::RecompileShaders();
	}
}