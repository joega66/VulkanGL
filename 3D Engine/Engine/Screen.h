#pragma once
#include <Platform/Platform.h>

using ScreenResChangedCallback = std::function<void(int32 Width, int32 Height)>;

class Screen
{
public:
	void Init();

	inline int32 GetWidth() const { return Width; }

	inline int32 GetHeight() const { return Height; }

	inline float GetAspectRatio() const { return static_cast<float>(Width) / static_cast<float>(Height); }

	void RegisterScreenResChangedCallback(const ScreenResChangedCallback& Callback);

private:
	int32 Width;
	int32 Height;
	std::list<ScreenResChangedCallback> ScreenResChangedCallbacks;

	void CallScreenResChanged();

	static void WindowResizeCallback(struct GLFWwindow* Window, int32 X, int32 Y);
};

extern Screen gScreen;