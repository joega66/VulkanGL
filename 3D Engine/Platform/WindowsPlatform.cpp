#include "WindowsPlatform.h"
#define WINDOWS_MEAN_AND_LEAN
#include <Windows.h>
#include <GLFW/glfw3.h>

Map<int32, Input::EButton> GLFWToEngineFormat
{
	ENTRY(GLFW_MOUSE_BUTTON_LEFT, Input::MouseLeft)
};

void WindowsPlatform::OpenWindow(int32 Width, int32 Height)
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	
	Window = glfwCreateWindow(Width, Height, "VulkanGL", nullptr, nullptr);
	
	glfwSetWindowUserPointer(Window, &Private);
	glfwSetFramebufferSizeCallback(Window, WindowResizeCallback);
	glfwSetKeyCallback(Window, KeyboardCallback);
	glfwSetScrollCallback(Window, ScrollCallback);
	glfwSetCursorPosCallback(Window, MouseCallback);
	glfwSetMouseButtonCallback(Window, MouseButtonCallback);
}

bool WindowsPlatform::WindowShouldClose()
{
	return glfwWindowShouldClose(Window);
}

glm::ivec2 WindowsPlatform::GetWindowSize()
{
	glm::ivec2 WindowSize;
	glfwGetFramebufferSize(Window, &WindowSize.x, &WindowSize.y);
	return WindowSize;
}

void WindowsPlatform::PollEvents()
{
	Private.ScrollOffset = {};
	glfwPollEvents();
}

void WindowsPlatform::RemoveNewlines(std::string& String)
{
	String.erase(std::remove(String.begin(), String.end(), '\r'), String.end());
	String.erase(std::remove(String.begin(), String.end(), '\n'), String.end());
}

void WindowsPlatform::ForkProcess(const std::string& ExePath, const std::string& CmdArgs) const
{
	enum { ParentRead, ParentWrite, ChildWrite, ChildRead, NumPipeTypes };

	SECURITY_ATTRIBUTES Security;
	Security.nLength = sizeof(Security);
	Security.bInheritHandle = TRUE;
	Security.lpSecurityDescriptor = NULL;
	
	HANDLE Pipes[NumPipeTypes];

	if (!CreatePipe(&Pipes[ParentWrite], &Pipes[ChildRead], &Security, 0))
		fail("%d", GetLastError());
	if (!CreatePipe(&Pipes[ParentRead], &Pipes[ChildWrite], &Security, 0))
		fail("%d", GetLastError());
	if (!SetHandleInformation(Pipes[ParentRead], HANDLE_FLAG_INHERIT, 0))
		fail("%d", GetLastError());
	if (!SetHandleInformation(Pipes[ParentWrite], HANDLE_FLAG_INHERIT, 0))
		fail("%d", GetLastError());

	STARTUPINFO StartupInfo;
	ZeroMemory(&StartupInfo, sizeof(STARTUPINFO));
	StartupInfo.cb = sizeof(STARTUPINFO);
	StartupInfo.wShowWindow = SW_SHOW;
	StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
	StartupInfo.hStdOutput = Pipes[ChildWrite];
	StartupInfo.hStdError = Pipes[ChildWrite];
	StartupInfo.hStdInput = Pipes[ChildRead];

	PROCESS_INFORMATION ProcessInfo;
	ZeroMemory(&ProcessInfo, sizeof(PROCESS_INFORMATION));

	std::vector<char> ExePathArr(ExePath.size());
	std::vector<char> CmdArgsArr(CmdArgs.size());

	ExePathArr.insert(ExePathArr.begin(), ExePath.begin(), ExePath.end());
	CmdArgsArr.insert(CmdArgsArr.begin(), CmdArgs.begin(), CmdArgs.end());
	
	if (CreateProcess(ExePathArr.data(), CmdArgsArr.data(),
		NULL, NULL, TRUE, 0, NULL,
		NULL, &StartupInfo, &ProcessInfo))
	{
		WaitForSingleObject(ProcessInfo.hProcess, INFINITE);
		CloseHandle(ProcessInfo.hThread);
		CloseHandle(ProcessInfo.hProcess);
	}
	else
	{
		fail("The process could not be started...");
	}

	for (uint32 i = 0; i < ARRAY_LENGTH(Pipes); i++)
	{
		CloseHandle(Pipes[i]);
	}
}

void WindowsPlatform::HideMouse(bool bHide)
{
	glfwSetInputMode(Window, GLFW_CURSOR, bHide ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
}

void WindowsPlatform::WindowResizeCallback(GLFWwindow* Window, int32 X, int32 Y)
{
	GPlatform->NotifyWindowListeners(X, Y);
}

void WindowsPlatform::KeyboardCallback(GLFWwindow* Window, int32 Key, int32 Scancode, int32 Action, int32 Mode)
{
	PlatformPrivate* Private = (PlatformPrivate*)glfwGetWindowUserPointer(Window);
	
	if (Key >= 0 && Key <= PlatformPrivate::NUM_KEYS)
	{
		if (!Contains(GLFWToEngineFormat, Key))
		{
			return;
		}

		Input::EButton EngineKey = GetValue(GLFWToEngineFormat, Key);

		if (Action == GLFW_PRESS)
		{
			Private->Keys[EngineKey] = true;
		}
		else if (Action == GLFW_RELEASE)
		{
			Private->Keys[EngineKey] = false;
			Private->KeysPressed[EngineKey] = true;
		}
	}
}

void WindowsPlatform::ScrollCallback(GLFWwindow* Window, double XOffset, double YOffset)
{
	PlatformPrivate* Private = (PlatformPrivate*)glfwGetWindowUserPointer(Window);
	Private->ScrollOffset = glm::dvec2(XOffset, YOffset);
}

void WindowsPlatform::MouseCallback(GLFWwindow* Window, double X, double Y)
{
	GPlatform->SetMousePosition(X, Y);
}

void WindowsPlatform::MouseButtonCallback(GLFWwindow * Window, int32 Button, int32 Action, int32 Mods)
{
	PlatformPrivate* Private = (PlatformPrivate*)glfwGetWindowUserPointer(Window);
	if (Button == GLFW_MOUSE_BUTTON_LEFT)
	{
		Input::EButton EngineKey = GetValue(GLFWToEngineFormat, Button);

		if (Action == GLFW_PRESS)
		{
			Private->Keys[EngineKey] = true;
		}
		else if (Action == GLFW_RELEASE)
		{
			Private->Keys[EngineKey] = false;
			Private->KeysPressed[EngineKey] = true;
		}
	}
}