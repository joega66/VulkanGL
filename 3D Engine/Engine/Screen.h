#pragma once
#include <Platform/Platform.h>
#include <functional>

using ScreenResizeEvent = std::function<void(uint32 pixelWidth, uint32 pixelHeight)>;

/** Abstraction of GLFW window resize events. */
class Screen
{
public:
	/** Set GLFW screen resize callback. */
	Screen(Platform& platform);
	
	/** Set a screen resize event. */
	[[nodiscard]] std::shared_ptr<ScreenResizeEvent> OnScreenResize(ScreenResizeEvent&& eventLambda);

	/** Call window resize events. */
	void CallEvents();

	inline uint32 GetWidth() const { return _PixelWidth; }
	inline uint32 GetHeight() const { return _PixelHeight; }

private:
	struct GLFWwindow* _Window;

	bool _IsDirty = false;
	uint32 _PixelWidth;
	uint32 _PixelHeight;

	/** List of events to be fired. */
	std::list<std::weak_ptr<ScreenResizeEvent>> _ScreenResizeEvents;

	/** The GLFW callback. */
	static void GLFWScreenSizeChangedEvent(struct GLFWwindow* window, int32 pixelWidth, int32 pixelHeight);
};