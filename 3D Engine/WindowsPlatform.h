#pragma once
#include "Platform.h"

class WindowsPlatform : public IPlatform
{
public:
	struct GLFWwindow* Window;

	virtual std::string GetPlatformName() final { return "Windows"; };
	virtual void OpenWindow(int32 Width, int32 Height) final;
	virtual bool WindowShouldClose() final;
	virtual void PollEvents() final;
	virtual void RemoveNewlines(std::string& String) final;
	virtual glm::ivec2 GetWindowSize() final;
	virtual void ForkProcess(const std::string& ExePath, const std::string& CmdArgs) const final;
	static void WindowResizeCallback(GLFWwindow* Window, int32 X, int32 Y);
};

CLASS(WindowsPlatform);