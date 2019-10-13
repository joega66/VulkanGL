#include "Input.h"
#include <GLFW/glfw3.h>

Input gInput;

static const HashTable<int32, EKeyCode> GLFWKeyCodes =
{
	ENTRY(GLFW_MOUSE_BUTTON_LEFT, EKeyCode::MouseLeft)
	ENTRY(GLFW_KEY_0, EKeyCode::Keypad0)
	ENTRY(GLFW_KEY_1, EKeyCode::Keypad1)
	ENTRY(GLFW_KEY_2, EKeyCode::Keypad2)
	ENTRY(GLFW_KEY_3, EKeyCode::Keypad3)
	ENTRY(GLFW_KEY_4, EKeyCode::Keypad4)
	ENTRY(GLFW_KEY_5, EKeyCode::Keypad5)
	ENTRY(GLFW_KEY_6, EKeyCode::Keypad6)
	ENTRY(GLFW_KEY_7, EKeyCode::Keypad7)
	ENTRY(GLFW_KEY_8, EKeyCode::Keypad8)
	ENTRY(GLFW_KEY_9, EKeyCode::Keypad9)
	ENTRY(GLFW_KEY_LEFT_SHIFT, EKeyCode::LeftShift)
	ENTRY(GLFW_KEY_LEFT_CONTROL, EKeyCode::LeftControl)
	ENTRY(GLFW_KEY_PERIOD, EKeyCode::Period)
	ENTRY(GLFW_KEY_W, EKeyCode::W)
	ENTRY(GLFW_KEY_A, EKeyCode::A)
	ENTRY(GLFW_KEY_S, EKeyCode::S)
	ENTRY(GLFW_KEY_D, EKeyCode::D)
};

void Input::KeyboardCallback(GLFWwindow* Window, int32 Key, int32 Scancode, int32 Action, int32 Mode)
{
	if (Contains(GLFWKeyCodes, Key))
	{
		EKeyCode KeyCode = GetValue(GLFWKeyCodes, Key);

		if (Action == GLFW_PRESS)
		{
			gInput.Keys[(size_t)KeyCode] = true;
		}
		else if (Action == GLFW_RELEASE)
		{
			gInput.Keys[(size_t)KeyCode] = false;
			gInput.KeysPressed[(size_t)KeyCode] = true;
		}
	}
}

void Input::MouseButtonCallback(GLFWwindow* Window, int32 Button, int32 Action, int32 Mods)
{
	if (Contains(GLFWKeyCodes, Button))
	{
		EKeyCode KeyCode = GetValue(GLFWKeyCodes, Button);

		if (Action == GLFW_PRESS)
		{
			gInput.Keys[(size_t)KeyCode] = true;
		}
		else if (Action == GLFW_RELEASE)
		{
			gInput.Keys[(size_t)KeyCode] = false;
			gInput.KeysPressed[(size_t)KeyCode] = true;
		}
	}
}

void Input::Init() const
{
	glfwSetKeyCallback(Platform.Window, KeyboardCallback);
	glfwSetMouseButtonCallback(Platform.Window, MouseButtonCallback);
}

bool Input::GetKeyDown(EKeyCode KeyCode) const
{
	return (size_t)KeyCode < Keys.size() ? Keys[(size_t)KeyCode] : false;
}

bool Input::GetKeyUp(EKeyCode KeyCode) const
{
	return (size_t)KeyCode < KeysPressed.size() ? KeysPressed[(size_t)KeyCode] : false;
}

void Input::AddShortcut(std::string&& ShortcutName, std::vector<EKeyCode>&& Shortcut)
{
	check(!Contains(Shortcuts, ShortcutName), "Shortcut name \"%s\" already taken.", ShortcutName.c_str());
	std::sort(Shortcut.begin(), Shortcut.end());
	check(!HasValue(Shortcuts, Shortcut), "\"%s\" trying to remap shortcut \"%s\".", ShortcutName.c_str(), GetKey(Shortcuts, Shortcut).c_str());
	Shortcuts[ShortcutName] = Shortcut;
}

bool Input::RemoveShortcut(std::string&& ShortcutName)
{
	return Shortcuts.erase(ShortcutName);
}

bool Input::GetShortcutDown(std::string&& ShortcutName) const
{
	check(Contains(Shortcuts, ShortcutName), "Shortcut %s not found.", ShortcutName.c_str());
	const auto& Shortcut = GetValue(Shortcuts, ShortcutName);
	return std::all_of(Shortcut.begin(), Shortcut.end(), [this] (auto Key) { return GetKeyDown(Key); });
}

bool Input::GetShortcutUp(std::string&& ShortcutName) const
{
	check(Contains(Shortcuts, ShortcutName), "Shortcut %s not found.", ShortcutName.c_str());
	const auto& Shortcut = GetValue(Shortcuts, ShortcutName);
	const bool bAllKeysPressed = std::all_of(Shortcut.begin(), Shortcut.end() - 1, [this] (auto Key) { return GetKeyDown(Key); });
	return bAllKeysPressed && GetKeyUp(Shortcut.back());
}

void Input::Update()
{
	std::fill(KeysPressed.begin(), KeysPressed.end(), false);
}