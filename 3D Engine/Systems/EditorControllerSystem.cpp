#include "EditorControllerSystem.h"
#include <Engine/Scene.h>
#include <Engine/Cursor.h>
#include <Engine/Input.h>
#include <DRMShader.h>

void EditorControllerSystem::Start(Scene& Scene)
{
	Scene.Input.AddShortcut("Recompile Shaders", { EKeyCode::LeftControl, EKeyCode::LeftShift, EKeyCode::Period });
}

void EditorControllerSystem::Update(Scene& Scene)
{
	View& View = Scene.View;

	// Translate the view.
	const float DS = Scene.Cursor.MouseScrollSpeed * Scene.Cursor.MouseScrollDelta.y;
	View.Translate(DS);

	if (Scene.Input.GetKeyDown(EKeyCode::MouseLeft))
	{
		// Disable the mouse while looking around.
		Scene.Cursor.Mode = ECursorMode::Disabled;

		if (!View.bFreeze)
		{
			// Look around.
			glm::vec2 Offset = glm::vec2(Scene.Cursor.Position.x - Scene.Cursor.Last.x, Scene.Cursor.Last.y - Scene.Cursor.Position.y) * Scene.Cursor.Sensitivity;
			View.Axis(Offset);
		}
	}
	else if (Scene.Input.GetKeyUp(EKeyCode::MouseLeft))
	{
		Scene.Cursor.Mode = ECursorMode::Normal;
	}

	if (Scene.Input.GetShortcutUp("Recompile Shaders"))
	{
		Scene.ShaderMap.RecompileShaders();
	}
}