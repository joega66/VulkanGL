#include "EditorControllerSystem.h"
#include "Renderer/Scene.h"

void EditorControllerSystem::Update(Scene& Scene)
{
	View& View = Scene.View;

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