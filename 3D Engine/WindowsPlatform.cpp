#include "WindowsPlatform.h"
#include <algorithm>
#define WINDOWS_MEAN_AND_LEAN
#include <Windows.h>
#include <GLFW/glfw3.h>

void WindowsPlatform::OpenWindow(int32 Width, int32 Height)
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	Window = glfwCreateWindow(Width, Height, "VulkanGL", nullptr, nullptr);
	glfwSetFramebufferSizeCallback(Window, WindowResizeCallback);
}

bool WindowsPlatform::WindowShouldClose()
{
	return glfwWindowShouldClose(Window);
}

void WindowsPlatform::PollEvents()
{
	glfwPollEvents();
}

void WindowsPlatform::RemoveNewlines(std::string& String)
{
	String.erase(std::remove(String.begin(), String.end(), '\r'), String.end());
	String.erase(std::remove(String.begin(), String.end(), '\n'), String.end());
}

glm::ivec2 WindowsPlatform::GetWindowSize()
{
	glm::ivec2 WindowSize;
	glfwGetFramebufferSize(Window, &WindowSize.x, &WindowSize.y);
	return WindowSize;
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

void WindowsPlatform::WindowResizeCallback(GLFWwindow* Window, int32 X, int32 Y)
{
	GPlatform->NotifyWindowListeners(X, Y);
}