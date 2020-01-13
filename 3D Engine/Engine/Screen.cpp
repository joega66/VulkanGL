#include "Screen.h"
#include <GLFW/glfw3.h>

void Screen::GLFWScreenSizeChangedEvent(GLFWwindow* Window, int32 PixelWidth, int32 PixelHeight)
{
	Screen* Screen = static_cast<class Screen*>(glfwGetWindowUserPointer(Window));
	Screen->FireScreenResizeEvents(PixelWidth, PixelHeight);
}

Screen::Screen()
{
	glfwSetWindowUserPointer(Platform.Window, this);

	glfwSetFramebufferSizeCallback(Platform.Window, GLFWScreenSizeChangedEvent);

	int32 PixelWidth, PixelHeight;
	glfwGetFramebufferSize(Platform.Window, &PixelWidth, &PixelHeight);

	FireScreenResizeEvents(PixelWidth, PixelHeight);
}

void Screen::ScreenResizeEvent(const std::function<void(int32 PixelWidth, int32 PixelHeight)>& Event)
{
	int32 PixelWidth, PixelHeight;
	glfwGetFramebufferSize(Platform.Window, &PixelWidth, &PixelHeight);

	Event(PixelWidth, PixelHeight);

	ScreenResizeEvents.push_back(Event);
}

void Screen::FireScreenResizeEvents(uint32 PixelWidth, uint32 PixelHeight)
{
	for (auto& Event : ScreenResizeEvents)
	{
		Event(PixelWidth, PixelHeight);
	}
}