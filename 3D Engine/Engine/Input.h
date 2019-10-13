#pragma once
#include <Platform/Platform.h>
#include <future>

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
};

class Input
{
public:
	Input() = default;
	Input(const Input&) = delete;
	Input& operator=(const Input&) = delete;

	void Init() const;

	bool GetKeyDown(EKeyCode KeyCode) const;

	bool GetKeyUp(EKeyCode KeyCode) const;

	void AddShortcut(std::string&& ShortcutName, std::vector<EKeyCode>&& Shortcut);

	bool RemoveShortcut(std::string&& ShortcutName);

	bool GetShortcutDown(std::string&& ShortcutName) const;

	bool GetShortcutUp(std::string&& ShortcutName) const;

	void Update();

private:
	static constexpr size_t NUM_KEYS = 1024;
	std::array<bool, NUM_KEYS> Keys;
	std::array<bool, NUM_KEYS> KeysPressed;
	HashTable<std::string, std::vector<EKeyCode>> Shortcuts;

	static void KeyboardCallback(struct GLFWwindow* Window, int32 Key, int32 Scancode, int32 Action, int32 Mode);
	static void MouseButtonCallback(struct GLFWwindow* Window, int32 Button, int32 Action, int32 Mods);
};

extern Input gInput;