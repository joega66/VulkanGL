#include "Screen.h"
#include <GLFW/glfw3.h>

void Screen::GLFWScreenSizeChangedEvent(GLFWwindow* window, int32 pixelWidth, int32 pixelHeight)
{
	Screen* screen = static_cast<const GLFWWindowUserPointer*>(glfwGetWindowUserPointer(window))->Screen;
	screen->_IsDirty = true;
	screen->_PixelWidth = pixelWidth;
	screen->_PixelHeight = pixelHeight;
}

Screen::Screen(Platform& platform)
	: _Window(platform.Window)
{
	glfwSetFramebufferSizeCallback(_Window, GLFWScreenSizeChangedEvent);

	int32 pixelWidth, pixelHeight;
	glfwGetFramebufferSize(_Window, &pixelWidth, &pixelHeight);

	_PixelWidth = pixelWidth;
	_PixelHeight = pixelHeight;
}

std::shared_ptr<ScreenResizeEvent> Screen::OnScreenResize(ScreenResizeEvent&& eventLambda)
{
	int32 pixelWidth, pixelHeight;
	glfwGetFramebufferSize(_Window, &pixelWidth, &pixelHeight);

	auto event = std::make_shared<ScreenResizeEvent>(eventLambda);

	(*event)(pixelWidth, pixelHeight);

	_ScreenResizeEvents.push_back(event);

	return event;
}

void Screen::CallEvents()
{
	if (!_IsDirty)
	{
		return;
	}

	_IsDirty = false;

	for (auto iter = _ScreenResizeEvents.begin(); iter != _ScreenResizeEvents.end();)
	{
		if (iter->expired())
		{
			iter = _ScreenResizeEvents.erase(iter);
		}
		else
		{
			auto event = iter->lock();
			(*event)(_PixelWidth, _PixelHeight);
			iter++;
		}
	}
}