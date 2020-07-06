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
	Input(Platform& Platform);

	Input(const Input&) = delete;
	Input& operator=(const Input&) = delete;

	/** Test if the key is being pressed. */
	bool GetKeyDown(EKeyCode KeyCode) const;

	/** Test if the key was released. */
	bool GetKeyUp(EKeyCode KeyCode) const;

	/** Add a new shortcut. */
	void AddShortcut(std::string&& ShortcutName, std::vector<EKeyCode>&& Shortcut);

	/** Remove the shortcut. */
	bool RemoveShortcut(std::string&& ShortcutName);

	/** Test if the shortcut is being pressed. */
	bool GetShortcutDown(std::string&& ShortcutName) const;

	/** Test if the shortcut was released. */
	bool GetShortcutUp(std::string&& ShortcutName) const;

	/** Apply end-of-frame updates. */
	void Update(Platform& Platform);

private:
	static constexpr size_t NUM_KEYS = 1024;

	/** Keys being pressed. */
	std::array<bool, NUM_KEYS> Keys;

	/** Keys just released. */
	std::array<bool, NUM_KEYS> KeysPressed;

	/** Keyboard shortcuts. */
	std::unordered_map<std::string, std::vector<EKeyCode>> Shortcuts;
	std::unordered_map<Crc, std::string> ShortcutCrcs;

	static void GLFWKeyboardEvent(struct GLFWwindow* Window, int32 Key, int32 Scancode, int32 Action, int32 Mode);

	static void GLFWMouseButtonEvent(struct GLFWwindow* Window, int32 Button, int32 Action, int32 Mods);
};