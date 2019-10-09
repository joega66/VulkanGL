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
	Period
};

class Input
{
	friend class OS_Platform;
public:
	static constexpr size_t NUM_KEYS = 1024;

	bool GetKeyDown(EKeyCode KeyCode) const;

	bool GetKeyUp(EKeyCode KeyCode) const;

	void AddShortcut(std::string&& ShortcutName, std::vector<EKeyCode>&& Shortcut);

	bool RemoveShortcut(std::string&& ShortcutName);

	bool GetShortcutDown(std::string&& ShortcutName) const;

	bool GetShortcutUp(std::string&& ShortcutName) const;

	void Update();

private:
	std::array<bool, NUM_KEYS> Keys;
	std::array<bool, NUM_KEYS> KeysPressed;
	HashTable<std::string, std::vector<EKeyCode>> Shortcuts;
};

extern class Input Input;