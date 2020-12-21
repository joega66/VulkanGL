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
	
	auto& cursor = engine._Cursor;
	auto& input = engine._Input;
	auto& ecs = engine._ECS;

	for (auto entity : ecs.GetEntities<Camera>())
	{
		auto& camera = ecs.GetComponent<Camera>(entity);

		const float ds = cursor._MouseScrollSpeed * cursor._MouseScrollDelta.y;

		camera.TranslateBy(ds);

		if (input.GetKeyDown(EKeyCode::MouseLeft))
		{
			// Disable the mouse while looking around.
			cursor._Mode = ECursorMode::Disabled;

			if (!camera.bFreeze)
			{
				const glm::vec2 degrees = glm::vec2(cursor._Position.x - cursor._Last.x, -(cursor._Last.y - cursor._Position.y)) * cursor._Sensitivity;
				camera.RotateBy(degrees);
			}
		}
	}

	if (input.GetKeyUp(EKeyCode::MouseLeft))
	{
		cursor._Mode = ECursorMode::Normal;
	}

	if (input.GetShortcutUp("Recompile Shaders"))
	{
		engine._Device.RecompileShaders();
	}
}