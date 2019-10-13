#pragma once
#include <Platform/Platform.h>

using ScreenResChangedCallback = std::function<void(int32 Width, int32 Height)>;

class Screen
{
public:
	void Init();

	int32 GetWidth() const { return Width; }

	int32 GetHeight() const { return Height; }

	float GetAspectRatio() const { return (float)Width / Height; }

	void RegisterScreenResChangedCallback(const ScreenResChangedCallback& Callback);

private:
	int32 Width;
	int32 Height;
	std::list<ScreenResChangedCallback> ScreenResChangedCallbacks;

	void CallScreenResChanged();

	static void WindowResizeCallback(struct GLFWwindow* Window, int32 X, int32 Y);
};

extern Screen gScreen;