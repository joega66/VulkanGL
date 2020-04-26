#pragma once
#include <Platform/Platform.h>
#include <functional>

/** Abstraction of GLFW window resize events. */
class Screen
{
public:
	/** Set GLFW screen resize callback. */
	Screen(Platform& Platform);
	
	/** Set an event to be fired. */
	void ScreenResizeEvent(const std::function<void(int32 PixelWidth, int32 PixelHeight)>& Event);

private:
	struct GLFWwindow* Window;

	/** List of events to be fired. */
	std::list<std::function<void(int32 PixelWidth, int32 PixelHeight)>> ScreenResizeEvents;

	/** Fire all window resize events. */
	void FireScreenResizeEvents(uint32 PixelWidth, uint32 PixelHeight) const;

	/** The GLFW callback. */
	static void GLFWScreenSizeChangedEvent(struct GLFWwindow* Window, int32 PixelWidth, int32 PixelHeight);
};