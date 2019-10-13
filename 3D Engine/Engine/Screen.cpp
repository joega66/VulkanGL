#include "Screen.h"
#include <GLFW/glfw3.h>

Screen gScreen;

void Screen::WindowResizeCallback(GLFWwindow* Window, int32 X, int32 Y)
{
	gScreen.Width = X;
	gScreen.Height = Y;
	gScreen.CallScreenResChanged();
}

void Screen::Init()
{
	glfwSetFramebufferSizeCallback(Platform.Window, WindowResizeCallback);

	int32 ActualWidth, ActualHeight;
	glfwGetFramebufferSize(Platform.Window, &ActualWidth, &ActualHeight);

	Width = ActualWidth;
	Height = ActualHeight;

	CallScreenResChanged();
}

void Screen::RegisterScreenResChangedCallback(const ScreenResChangedCallback& Callback)
{
	Callback(Width, Height);
	ScreenResChangedCallbacks.push_back(Callback);
}

void Screen::CallScreenResChanged()
{
	for (auto& Callback : ScreenResChangedCallbacks)
	{
		Callback(Width, Height);
	}
}