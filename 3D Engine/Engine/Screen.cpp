#include "Screen.h"
#include <GLFW/glfw3.h>

void Screen::GLFWScreenSizeChangedEvent(GLFWwindow* Window, int32 PixelWidth, int32 PixelHeight)
{
	const Screen* Screen = static_cast<const GLFWWindowUserPointer*>(glfwGetWindowUserPointer(Window))->Screen;
	Screen->FireScreenResizeEvents(PixelWidth, PixelHeight);
}

Screen::Screen(Platform& Platform)
	: Window(Platform.Window)
{
	glfwSetFramebufferSizeCallback(Window, GLFWScreenSizeChangedEvent);

	int32 PixelWidth, PixelHeight;
	glfwGetFramebufferSize(Window, &PixelWidth, &PixelHeight);

	FireScreenResizeEvents(PixelWidth, PixelHeight);
}

void Screen::ScreenResizeEvent(const std::function<void(int32 PixelWidth, int32 PixelHeight)>& Event)
{
	int32 PixelWidth, PixelHeight;
	glfwGetFramebufferSize(Window, &PixelWidth, &PixelHeight);

	Event(PixelWidth, PixelHeight);

	ScreenResizeEvents.push_back(Event);
}

void Screen::FireScreenResizeEvents(uint32 PixelWidth, uint32 PixelHeight) const
{
	for (auto& Event : ScreenResizeEvents)
	{
		Event(PixelWidth, PixelHeight);
	}
}