#pragma once
#include <Platform/Platform.h>

enum class EKeyCode
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
	Keypad0
};

class Input
{
	friend class IPlatform;
	friend class WindowsPlatform;
public:
	static constexpr size_t NUM_KEYS = 1024;

	inline bool GetKeyDown(EKeyCode KeyCode)
	{
		return (size_t)KeyCode < Keys.size() ? Keys[(size_t)KeyCode] : false;
	}
	inline bool GetKeyUp(EKeyCode KeyCode)
	{
		return (size_t)KeyCode < KeysPressed.size() ? KeysPressed[(size_t)KeyCode] : false;
	}

private:
	std::array<bool, NUM_KEYS> Keys;
	std::array<bool, NUM_KEYS> KeysPressed;
};

extern class Input Input;