#pragma once
#include <Platform/Platform.h>

enum class EKeyCode : int32
{
	MouseLeft,
	Keypad1,
	Keypad2,
	Keypad3,
	Keypad4,
	Keypad5,
	Keypad6,
	Keypad7,
	Keypad8,
	Keypad9,
	Keypad0,
	LeftShift,
	LeftControl,
	Period,
	W,
	A,
	S,
	D,
	Delete,
};

/** Abstraction around GLFW input callbacks. */
class Input
{
public:
	/** Set GLFW callbacks. */
	Input(Platform& platform);

	Input(const Input&) = delete;
	Input& operator=(const Input&) = delete;

	/** Test if the key is being pressed. */
	bool GetKeyDown(EKeyCode keyCode) const;

	/** Test if the key was released. */
	bool GetKeyUp(EKeyCode keyCode) const;

	/** Add a new shortcut. */
	void AddShortcut(std::string&& shortcutName, std::vector<EKeyCode>&& shortcut);

	/** Remove the shortcut. */
	bool RemoveShortcut(std::string&& shortcutName);

	/** Test if the shortcut is being pressed. */
	bool GetShortcutDown(std::string&& shortcutName) const;

	/** Test if the shortcut was released. */
	bool GetShortcutUp(std::string&& shortcutName) const;

	/** Apply end-of-frame updates. */
	void Update(Platform& platform);

private:
	static constexpr size_t NUM_KEYS = 1024;

	/** Keys being pressed. */
	std::array<bool, NUM_KEYS> _Keys;

	/** Keys just released. */
	std::array<bool, NUM_KEYS> _KeysPressed;

	/** Keyboard shortcuts. */
	std::unordered_map<std::string, std::vector<EKeyCode>> _Shortcuts;
	std::unordered_map<Crc, std::string> _ShortcutCrcs;

	static void GLFWKeyboardEvent(struct GLFWwindow* window, int32 key, int32 scancode, int32 action, int32 mode);

	static void GLFWMouseButtonEvent(struct GLFWwindow* window, int32 button, int32 action, int32 mods);
};