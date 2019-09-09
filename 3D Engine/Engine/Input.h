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

	inline bool GetKeyDown(EKeyCode KeyCode) const
	{
		return (size_t)KeyCode < Keys.size() ? Keys[(size_t)KeyCode] : false;
	}

	inline bool GetKeyUp(EKeyCode KeyCode) const
	{
		return (size_t)KeyCode < KeysPressed.size() ? KeysPressed[(size_t)KeyCode] : false;
	}

	void AddShortcut(std::string&& ShortcutName, std::vector<EKeyCode>&& Shortcut)
	{
		check(!Contains(Shortcuts, ShortcutName), "Shortcut name \"%s\" already taken.", ShortcutName.c_str());
		std::sort(Shortcut.begin(), Shortcut.end());
		check(!HasValue(Shortcuts, Shortcut), "\"%s\" trying to remap shortcut \"%s\".", ShortcutName.c_str(), GetKey(Shortcuts, Shortcut).c_str());
		Shortcuts[ShortcutName] = Shortcut;
	}

	bool RemoveShortcut(std::string&& ShortcutName)
	{
		return Shortcuts.erase(ShortcutName);
	}

	bool GetShortcutDown(std::string&& ShortcutName) const
	{
		check(Contains(Shortcuts, ShortcutName), "Shortcut %s not found.", ShortcutName.c_str());
		const auto& Shortcut = GetValue(Shortcuts, ShortcutName);
		return std::all_of(Shortcut.begin(), Shortcut.end(), [this](auto Key) { return GetKeyDown(Key); });
	}

	bool GetShortcutUp(std::string&& ShortcutName) const
	{
		check(Contains(Shortcuts, ShortcutName), "Shortcut %s not found.", ShortcutName.c_str());
		const auto& Shortcut = GetValue(Shortcuts, ShortcutName);
		const bool bAllKeysPressed = std::all_of(Shortcut.begin(), Shortcut.end() - 1, [this](auto Key) { return GetKeyDown(Key); });
		return bAllKeysPressed && GetKeyUp(Shortcut.back());
	}

private:
	std::array<bool, NUM_KEYS> Keys;
	std::array<bool, NUM_KEYS> KeysPressed;
	HashTable<std::string, std::vector<EKeyCode>> Shortcuts;
};

extern class Input Input;