#pragma once
#include <Platform/Platform.h>

using ScreenResChangedCallback = std::function<void(int32 Width, int32 Height)>;

class Screen
{
	friend class OS_Platform;
public:
	int32 GetWidth() const { return Width; }

	int32 GetHeight() const { return Height; }

	void RegisterScreenResChangedCallback(const ScreenResChangedCallback& Callback);

private:
	int32 Width;
	int32 Height;
	std::list<ScreenResChangedCallback> ScreenResChangedCallbacks;

	void CallScreenResChanged();
};

extern class Screen Screen;