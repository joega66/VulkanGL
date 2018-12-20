#pragma once
#include "Platform.h"

struct GLFWwindow;

class WindowsPlatform : public IPlatform
{
public:
	GLFWwindow* Window;

	virtual std::string GetPlatformName() final { return "Windows"; };
	virtual void OpenWindow(int32 Width, int32 Height) final;
	virtual bool WindowShouldClose() final;
	virtual glm::ivec2 GetWindowSize() final;
	virtual void PollEvents() final;
	virtual void RemoveNewlines(std::string& String) final;
	virtual void ForkProcess(const std::string& ExePath, const std::string& CmdArgs) const final;
	virtual void HideMouse(bool bHide) final;

	static void WindowResizeCallback(GLFWwindow* Window, int32 X, int32 Y);
	static void KeyboardCallback(GLFWwindow* Window, int32 Key, int32 Scancode, int32 Action, int32 Mode);
	static void ScrollCallback(GLFWwindow* Window, double XOffset, double YOffset);
	static void MouseCallback(GLFWwindow* Window, double X, double Y);
	static void MouseButtonCallback(GLFWwindow* Window, int32 Button, int32 Action, int32 Mods);
};

CLASS(WindowsPlatform);