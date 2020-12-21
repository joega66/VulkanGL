#include "Input.h"
#include <GLFW/glfw3.h>

static std::unordered_map<int32, EKeyCode> gGLFWKeyCodes =
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

void Input::GLFWKeyboardEvent(GLFWwindow* window, int32 key, int32 scancode, int32 action, int32 mode)
{
	Input* input = static_cast<const GLFWWindowUserPointer*>(glfwGetWindowUserPointer(window))->_Input;

	if (gGLFWKeyCodes.contains(key))
	{
		const EKeyCode keyCode = gGLFWKeyCodes[key];

		if (action == GLFW_PRESS)
		{
			input->_Keys[static_cast<size_t>(keyCode)] = true;
		}
		else if (action == GLFW_RELEASE)
		{
			input->_Keys[static_cast<size_t>(keyCode)] = false;
			input->_KeysPressed[static_cast<size_t>(keyCode)] = true;
		}
	}
}

void Input::GLFWMouseButtonEvent(GLFWwindow* window, int32 button, int32 action, int32 mods)
{
	Input* input = static_cast<const GLFWWindowUserPointer*>(glfwGetWindowUserPointer(window))->_Input;

	if (gGLFWKeyCodes.contains(button))
	{
		const EKeyCode keyCode = gGLFWKeyCodes[button];

		if (action == GLFW_PRESS)
		{
			input->_Keys[static_cast<size_t>(keyCode)] = true;
		}
		else if (action == GLFW_RELEASE)
		{
			input->_Keys[static_cast<size_t>(keyCode)] = false;
			input->_KeysPressed[static_cast<size_t>(keyCode)] = true;
		}
	}
}

Input::Input(Platform& platform)
{
	glfwSetKeyCallback(platform._Window, GLFWKeyboardEvent);

	glfwSetMouseButtonCallback(platform._Window, GLFWMouseButtonEvent);
}

bool Input::GetKeyDown(EKeyCode keyCode) const
{
	return static_cast<size_t>(keyCode) < _Keys.size() ? _Keys[static_cast<size_t>(keyCode)] : false;
}

bool Input::GetKeyUp(EKeyCode keyCode) const
{
	return static_cast<size_t>(keyCode) < _KeysPressed.size() ? _KeysPressed[static_cast<size_t>(keyCode)] : false;
}

void Input::AddShortcut(std::string&& shortcutName, std::vector<EKeyCode>&& shortcut)
{
	check(!_Shortcuts.contains(shortcutName), "Shortcut name \"%s\" already taken.", shortcutName.c_str());

	std::sort(shortcut.begin(), shortcut.end());

	Crc crc = 0;
	Platform::crc32_u32(crc, shortcut.data(), shortcut.size() * sizeof(shortcut[0]));

	check(!_ShortcutCrcs.contains(crc), "\"%s\" trying to remap shortcut \"%s\".", shortcutName.c_str(), _ShortcutCrcs[crc].c_str());

	_ShortcutCrcs[crc] = shortcutName;

	_Shortcuts[shortcutName] = shortcut;
}

bool Input::RemoveShortcut(std::string&& shortcutName)
{
	return _Shortcuts.erase(shortcutName);
}

bool Input::GetShortcutDown(std::string&& shortcutName) const
{
	check(_Shortcuts.contains(shortcutName), "Shortcut %s not found.", shortcutName.c_str());
	const auto& shortcut = _Shortcuts.at(shortcutName);
	return std::all_of(shortcut.begin(), shortcut.end(), [this] (auto key) { return GetKeyDown(key); });
}

bool Input::GetShortcutUp(std::string&& shortcutName) const
{
	check(_Shortcuts.contains(shortcutName), "Shortcut %s not found.", shortcutName.c_str());
	const auto& shortcut = _Shortcuts.at(shortcutName);
	const bool areAllKeysPressed = std::all_of(shortcut.begin(), shortcut.end() - 1, [this] (auto key) { return GetKeyDown(key); });
	return areAllKeysPressed && GetKeyUp(shortcut.back());
}

void Input::Update()
{
	std::fill(_KeysPressed.begin(), _KeysPressed.end(), false);
}