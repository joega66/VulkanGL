#include "EditorControllerSystem.h"
#include <Engine/Engine.h>
#include <Engine/Cursor.h>
#include <Engine/Input.h>
#include <DRMShader.h>
#include <imgui/imgui.h>

void EditorControllerSystem::Start(Engine& Engine)
{
	Engine._Input.AddShortcut("Recompile Shaders", { EKeyCode::LeftControl, EKeyCode::LeftShift, EKeyCode::Period });
}

void EditorControllerSystem::Update(Engine& Engine)
{
	if (ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard)
	{
		return;
	}
	
	Cursor& Cursor = Engine._Cursor;
	Input& Input = Engine._Input;
	Camera& Camera = Engine.Camera;

	const float DS = Cursor.MouseScrollSpeed * Cursor.MouseScrollDelta.y;
	Camera.TranslateBy(DS);

	if (Input.GetKeyDown(EKeyCode::MouseLeft))
	{
		// Disable the mouse while looking around.
		Cursor.Mode = ECursorMode::Disabled;

		if (!Camera.bFreeze)
		{
			const glm::vec2 Degrees = glm::vec2(Cursor.Position.x - Cursor.Last.x, -(Cursor.Last.y - Cursor.Position.y)) * Cursor.Sensitivity;
			Camera.RotateBy(Degrees);
		}
	}
	else if (Input.GetKeyUp(EKeyCode::MouseLeft))
	{
		Cursor.Mode = ECursorMode::Normal;
	}

	if (Input.GetShortcutUp("Recompile Shaders"))
	{
		Engine.ShaderMap.RecompileShaders();
	}
}