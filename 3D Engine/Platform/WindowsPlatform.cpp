#include "WindowsPlatform.h"
#define WINDOWS_MEAN_AND_LEAN
#include <Windows.h>
#include <GLFW/glfw3.h>
#include <Engine/Cursor.h>
#include <Engine/Input.h>

static HashTable<int32, EKeyCode> KeyCodes =
{
	ENTRY(GLFW_MOUSE_BUTTON_LEFT, EKeyCode::MouseLeft)
	ENTRY(GLFW_KEY_0, EKeyCode::Keypad0)
	ENTRY(GLFW_KEY_1, EKeyCode::Keypad1)
	ENTRY(GLFW_KEY_2, EKeyCode::Keypad2)
	ENTRY(GLFW_KEY_3, EKeyCode::Keypad3)
	ENTRY(GLFW_KEY_4, EKeyCode::Keypad4)
	ENTRY(GLFW_KEY_5, EKeyCode::Keypad5)
	ENTRY(GLFW_KEY_6, EKeyCode::Keypad6)
	ENTRY(GLFW_KEY_7, EKeyCode::Keypad7)
	ENTRY(GLFW_KEY_8, EKeyCode::Keypad8)
	ENTRY(GLFW_KEY_9, EKeyCode::Keypad9)
};

void WindowsPlatform::OpenWindow(int32 Width, int32 Height)
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	Window = glfwCreateWindow(Width, Height, "VulkanGL", nullptr, nullptr);

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
	Cursor.MouseScrollDelta = {};
	Cursor.Last = Cursor.Position;
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

void WindowsPlatform::MouseState(class Cursor& Cursor)
{
	uint32 InputMode;

	switch (Cursor.Mode)
	{
	case ECursorMode::Normal:
		InputMode = GLFW_CURSOR_NORMAL;
		break;
	case ECursorMode::Hidden:
		InputMode = GLFW_CURSOR_HIDDEN;
		break;
	case ECursorMode::Disabled:
		InputMode = GLFW_CURSOR_DISABLED;
		break;
	}

	glfwSetInputMode(Window, GLFW_CURSOR, InputMode);
}

void WindowsPlatform::WindowResizeCallback(GLFWwindow* Window, int32 X, int32 Y)
{
	GPlatform->NotifyWindowListeners(X, Y);
}

void WindowsPlatform::KeyboardCallback(GLFWwindow* Window, int32 Key, int32 Scancode, int32 Action, int32 Mode)
{
	if (Key >= 0 && Key <= Input::NUM_KEYS)
	{
		if (Contains(KeyCodes, Key))
		{
			EKeyCode KeyCode = GetValue(KeyCodes, Key);

			if (Action == GLFW_PRESS)
			{
				Input.Keys[(size_t)KeyCode] = true;
			}
			else if (Action == GLFW_RELEASE)
			{
				Input.Keys[(size_t)KeyCode] = false;
				Input.KeysPressed[(size_t)KeyCode] = true;
			}
		}
	}
}

void WindowsPlatform::ScrollCallback(GLFWwindow* Window, double XOffset, double YOffset)
{
	Cursor.MouseScrollDelta = glm::vec2(XOffset, YOffset);
}

void WindowsPlatform::MouseCallback(GLFWwindow* Window, double X, double Y)
{
	// @todo We can get around the annoying glfw cursor jump by tracking state in WindowsPlatform...
	Cursor.Position = glm::vec2(X, Y);
}

void WindowsPlatform::MouseButtonCallback(GLFWwindow* Window, int32 Button, int32 Action, int32 Mods)
{
	if (Contains(KeyCodes, Button))
	{
		EKeyCode KeyCode = GetValue(KeyCodes, Button);

		if (Action == GLFW_PRESS)
		{
			Input.Keys[(size_t)KeyCode] = true;
		}
		else if (Action == GLFW_RELEASE)
		{
			Input.Keys[(size_t)KeyCode] = false;
			Input.KeysPressed[(size_t)KeyCode] = true;
		}
	}
}