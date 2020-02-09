#include "EditorControllerSystem.h"
#include <Engine/Scene.h>
#include <Engine/Cursor.h>
#include <Engine/Input.h>
#include <DRMShader.h>
#include <imgui/imgui.h>

void EditorControllerSystem::Start(Scene& Scene)
{
	Scene.Input.AddShortcut("Recompile Shaders", { EKeyCode::LeftControl, EKeyCode::LeftShift, EKeyCode::Period });
}

void EditorControllerSystem::Update(Scene& Scene)
{
	if (ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard)
	{
		return;
	}
	
	Camera& Camera = Scene.Camera;

	// Translate the view.
	const float DS = Scene.Cursor.MouseScrollSpeed * Scene.Cursor.MouseScrollDelta.y;
	Camera.Translate(DS);

	if (Scene.Input.GetKeyDown(EKeyCode::MouseLeft))
	{
		// Disable the mouse while looking around.
		Scene.Cursor.Mode = ECursorMode::Disabled;

		if (!Camera.bFreeze)
		{
			// Look around.
			glm::vec2 Offset = glm::vec2(Scene.Cursor.Position.x - Scene.Cursor.Last.x, Scene.Cursor.Last.y - Scene.Cursor.Position.y) * Scene.Cursor.Sensitivity;
			Camera.Axis(Offset);
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