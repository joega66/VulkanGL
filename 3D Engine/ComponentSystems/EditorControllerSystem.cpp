#include "EditorControllerSystem.h"
#include <Renderer/Scene.h>

void EditorControllerSystem::Update()
{
	auto& View = Scene::Get().View;

	View.Translate();

	if (Input::GetKeyDown(Input::MouseLeft))
	{
		GPlatform->HideMouse(true);
		View.LookAround();
	}
	else
	{
		GPlatform->HideMouse(false);
		View.SetLastMousePosition();
	}
}