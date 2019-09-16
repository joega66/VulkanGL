#include <fstream>
#include <cstdarg>
#include <iostream>
#include <algorithm>
#include <filesystem>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define WINDOWS_MEAN_AND_LEAN
#include <Windows.h>
#include <GLFW/glfw3.h>

#include <Engine/Screen.h>
#include <Engine/Cursor.h>
#include <Engine/Input.h>

OS_Platform Platform;

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
	ENTRY(GLFW_KEY_LEFT_SHIFT, EKeyCode::LeftShift)
	ENTRY(GLFW_KEY_LEFT_CONTROL, EKeyCode::LeftControl)
	ENTRY(GLFW_KEY_PERIOD, EKeyCode::Period)
};

void OS_Platform::OpenWindow(int32 Width, int32 Height)
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	Window = glfwCreateWindow(Width, Height, "Vulkan Engine", nullptr, nullptr);

	glfwSetFramebufferSizeCallback(Window, WindowResizeCallback);
	glfwSetKeyCallback(Window, KeyboardCallback);
	glfwSetScrollCallback(Window, ScrollCallback);
	glfwSetCursorPosCallback(Window, MouseCallback);
	glfwSetMouseButtonCallback(Window, MouseButtonCallback);

	int32 ActualWidth, ActualHeight;
	glfwGetFramebufferSize(Window, &ActualWidth, &ActualHeight);
	Screen.Width = ActualWidth;
	Screen.Height = ActualHeight;
	Screen.CallScreenResChanged();
}

bool OS_Platform::WindowShouldClose()
{
	return glfwWindowShouldClose(Window);
}

void OS_Platform::PollEvents()
{
	// Process window events.
	glfwPollEvents();
}

std::string OS_Platform::FileRead(const std::string& Filename) const
{
	std::ifstream File(Filename, std::ios::ate | std::ios::binary);

	check(File.is_open(), "Failed to open file %s", Filename.c_str());

	size_t FileSize = static_cast<size_t>(File.tellg());
	std::string Buffer;
	Buffer.resize(FileSize);

	File.seekg(0);
	File.read(Buffer.data(), FileSize);
	File.close();

	return Buffer;
}

void OS_Platform::FileDelete(const std::string& Filename) const
{
	check(std::filesystem::remove(Filename), "Failed to remove file...");
}

void OS_Platform::FileRename(const std::string& Old, const std::string& New) const
{
	check(FileExists(Old), "Renaming file that doesn't exist...");
	std::filesystem::rename(Old, New);
}

bool OS_Platform::FileExists(const std::string& Filename) const
{
	return std::filesystem::is_regular_file(Filename);
}

void OS_Platform::WriteLog(const std::string& InLog)
{
	std::string Recent = InLog + '\n';
	std::cerr << Recent;
}

void OS_Platform::WriteLog(const std::string& File, const std::string& Func, int32 Line, const std::string& InLog)
{
	std::string Header = "[Debug] [" + SanitizeFile(File) + ":" + Func + ":" + std::to_string(Line) + "]\n";
	std::string Recent = Header + InLog + '\n';
	std::cerr << Recent;
}

void OS_Platform::WriteLog(const std::string& Expression, const std::string& File, const std::string& Func, int32 Line, const std::string& InLog)
{
	std::string Header = "[Warning] [" + SanitizeFile(File) + ":" + Func + ":" + std::to_string(Line) + ":" + Expression + "]\n";
	std::string Recent = Header + InLog + '\n';
	std::cerr << Recent;
}

std::string OS_Platform::SanitizeFile(const std::string& File)
{
	std::string FileSanitized = File;
	FileSanitized.erase(FileSanitized.begin(), FileSanitized.begin() + FileSanitized.find_last_of('\\') + 1);
	return FileSanitized;
}

std::string OS_Platform::FormatString(std::string Format, ...)
{
	va_list Args, ArgsCopy;
	va_start(Args, Format);
	va_copy(ArgsCopy, Args);

	const auto SZ = std::vsnprintf(nullptr, 0, Format.c_str(), Args) + 1;

	try
	{
		std::string Result(SZ, ' ');
		std::vsnprintf(&Result.front(), SZ, Format.c_str(), ArgsCopy);
		va_end(ArgsCopy);
		va_end(Args);
		Result.erase(std::remove(Result.begin(), Result.end(), '\0'), Result.end());
		return Result;
	}
	catch (const std::bad_alloc& E)
	{
		va_end(ArgsCopy);
		va_end(Args);
		fail(E.what());
	}
}

void OS_Platform::ForkProcess(const std::string& ExePath, const std::string& CmdArgs) const
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

	for (uint32 i = 0; i < ArrayLength(Pipes); i++)
	{
		CloseHandle(Pipes[i]);
	}
}


void OS_Platform::Memcpy(void* Dst, const void* Src, size_t Size) const
{
	memcpy(Dst, Src, Size);
}

uint8* OS_Platform::LoadImage(const std::string& Filename, int32& Width, int32& Height, int32& NumChannels) const
{
	uint8* Image = stbi_load(Filename.c_str(), &Width, &Height, &NumChannels, STBI_rgb_alpha);
	check(Image, "%s failed to load", Filename.c_str());
	return Image;
}

void OS_Platform::FreeImage(uint8* Pixels) const
{
	stbi_image_free(Pixels);
}

void OS_Platform::Finish()
{
	// Update the hardware state.
	UpdateCursorState(Cursor);
	UpdateInputState(Input);
}

void OS_Platform::UpdateCursorState(class Cursor& Cursor)
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

	Cursor.MouseScrollDelta = {};
	Cursor.Last = Cursor.Position;
}

void OS_Platform::UpdateInputState(class Input& Input)
{
	std::fill(Input.KeysPressed.begin(), Input.KeysPressed.end(), false);
}

void OS_Platform::WindowResizeCallback(GLFWwindow* Window, int32 X, int32 Y)
{
	Screen.Width = X;
	Screen.Height = Y;
	Screen.CallScreenResChanged();
}

void OS_Platform::KeyboardCallback(GLFWwindow* Window, int32 Key, int32 Scancode, int32 Action, int32 Mode)
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

void OS_Platform::ScrollCallback(GLFWwindow* Window, double XOffset, double YOffset)
{
	Cursor.MouseScrollDelta = glm::vec2(XOffset, YOffset);
}

void OS_Platform::MouseCallback(GLFWwindow* Window, double X, double Y)
{
	Cursor.Position = glm::vec2(X, Y);
}

void OS_Platform::MouseButtonCallback(GLFWwindow* Window, int32 Button, int32 Action, int32 Mods)
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