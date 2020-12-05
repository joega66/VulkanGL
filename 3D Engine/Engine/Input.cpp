#include "Input.h"
#include <GLFW/glfw3.h>

static std::unordered_map<int32, EKeyCode> GLFWKeyCodes =
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
	ENTRY(GLFW_KEY_DELETE, EKeyCode::Delete)
};

void Input::GLFWKeyboardEvent(GLFWwindow* Window, int32 Key, int32 Scancode, int32 Action, int32 Mode)
{
	Input* Input = static_cast<const GLFWWindowUserPointer*>(glfwGetWindowUserPointer(Window))->Input;

	if (GLFWKeyCodes.contains(Key))
	{
		const EKeyCode KeyCode = GLFWKeyCodes[Key];

		if (Action == GLFW_PRESS)
		{
			Input->Keys[static_cast<size_t>(KeyCode)] = true;
		}
		else if (Action == GLFW_RELEASE)
		{
			Input->Keys[static_cast<size_t>(KeyCode)] = false;
			Input->KeysPressed[static_cast<size_t>(KeyCode)] = true;
		}
	}
}

void Input::GLFWMouseButtonEvent(GLFWwindow* Window, int32 Button, int32 Action, int32 Mods)
{
	Input* Input = static_cast<const GLFWWindowUserPointer*>(glfwGetWindowUserPointer(Window))->Input;

	if (GLFWKeyCodes.contains(Button))
	{
		const EKeyCode KeyCode = GLFWKeyCodes[Button];

		if (Action == GLFW_PRESS)
		{
			Input->Keys[static_cast<size_t>(KeyCode)] = true;
		}
		else if (Action == GLFW_RELEASE)
		{
			Input->Keys[static_cast<size_t>(KeyCode)] = false;
			Input->KeysPressed[static_cast<size_t>(KeyCode)] = true;
		}
	}
}

Input::Input(Platform& Platform)
{
	glfwSetKeyCallback(Platform.Window, GLFWKeyboardEvent);

	glfwSetMouseButtonCallback(Platform.Window, GLFWMouseButtonEvent);
}

bool Input::GetKeyDown(EKeyCode KeyCode) const
{
	return static_cast<size_t>(KeyCode) < Keys.size() ? Keys[static_cast<size_t>(KeyCode)] : false;
}

bool Input::GetKeyUp(EKeyCode KeyCode) const
{
	return static_cast<size_t>(KeyCode) < KeysPressed.size() ? KeysPressed[static_cast<size_t>(KeyCode)] : false;
}

void Input::AddShortcut(std::string&& ShortcutName, std::vector<EKeyCode>&& Shortcut)
{
	check(!Shortcuts.contains(ShortcutName), "Shortcut name \"%s\" already taken.", ShortcutName.c_str());

	std::sort(Shortcut.begin(), Shortcut.end());

	Crc crc = 0;
	Platform::crc32_u32(crc, Shortcut.data(), Shortcut.size() * sizeof(Shortcut[0]));

	check(!ShortcutCrcs.contains(crc), "\"%s\" trying to remap shortcut \"%s\".", ShortcutName.c_str(), ShortcutCrcs[crc].c_str());

	ShortcutCrcs[crc] = ShortcutName;

	Shortcuts[ShortcutName] = Shortcut;
}

bool Input::RemoveShortcut(std::string&& ShortcutName)
{
	return Shortcuts.erase(ShortcutName);
}

bool Input::GetShortcutDown(std::string&& ShortcutName) const
{
	check(Shortcuts.contains(ShortcutName), "Shortcut %s not found.", ShortcutName.c_str());
	const auto& Shortcut = Shortcuts.at(ShortcutName);
	return std::all_of(Shortcut.begin(), Shortcut.end(), [this] (auto Key) { return GetKeyDown(Key); });
}

bool Input::GetShortcutUp(std::string&& ShortcutName) const
{
	check(Shortcuts.contains(ShortcutName), "Shortcut %s not found.", ShortcutName.c_str());
	const auto& Shortcut = Shortcuts.at(ShortcutName);
	const bool bAllKeysPressed = std::all_of(Shortcut.begin(), Shortcut.end() - 1, [this] (auto Key) { return GetKeyDown(Key); });
	return bAllKeysPressed && GetKeyUp(Shortcut.back());
}

void Input::Update(Platform& Platform)
{
	std::fill(KeysPressed.begin(), KeysPressed.end(), false);
}