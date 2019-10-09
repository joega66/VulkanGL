#include "Input.h"

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