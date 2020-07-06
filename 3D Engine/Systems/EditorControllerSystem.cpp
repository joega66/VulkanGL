#include "EditorControllerSystem.h"
#include <Engine/Engine.h>
#include <Engine/Cursor.h>
#include <Engine/Input.h>
#include <GPU/GPUShader.h>
#include <imgui/imgui.h>

void EditorControllerSystem::Start(Engine& engine)
{
	engine._Input.AddShortcut("Recompile Shaders", { EKeyCode::LeftControl, EKeyCode::LeftShift, EKeyCode::Period });
}

void EditorControllerSystem::Update(Engine& engine)
{
	if (ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard)
	{
		return;
	}
	
	Cursor& cursor = engine._Cursor;
	Input& input = engine._Input;
	Camera& camera = engine.Camera;

	const float ds = cursor.MouseScrollSpeed * cursor.MouseScrollDelta.y;
	camera.TranslateBy(ds);

	if (input.GetKeyDown(EKeyCode::MouseLeft))
	{
		// Disable the mouse while looking around.
		cursor.Mode = ECursorMode::Disabled;

		if (!camera.bFreeze)
		{
			const glm::vec2 degrees = glm::vec2(cursor.Position.x - cursor.Last.x, -(cursor.Last.y - cursor.Position.y)) * cursor.Sensitivity;
			camera.RotateBy(degrees);
		}
	}
	else if (input.GetKeyUp(EKeyCode::MouseLeft))
	{
		cursor.Mode = ECursorMode::Normal;
	}

	if (input.GetShortcutUp("Recompile Shaders"))
	{
		engine.ShaderLibrary.RecompileShaders();
	}
}