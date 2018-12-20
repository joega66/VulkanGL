#include "EditorController.h"
#include "View.h"

void EditorController::Update(View& View)
{
	View.Translate();

	if (Input::GetKeyDown(Input::MouseLeft))
	{
		GPlatform->HideMouse(true);
		View.LookAround();
	}
	else
	{
		View.SetLastMousePosition();
		GPlatform->HideMouse(false);
	}
}