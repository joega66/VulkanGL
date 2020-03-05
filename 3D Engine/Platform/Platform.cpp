#include "Platform.h"

#include <fstream>
#include <cstdarg>
#include <iostream>
#include <algorithm>
#include <filesystem>
#include <cctype>

#include <stb_image.h>

#define WINDOWS_MEAN_AND_LEAN
#include <Windows.h>
#include <shlobj.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

WindowsPlatform::WindowsPlatform(int32 Width, int32 Height)
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	Window = glfwCreateWindow(Width, Height, "Vulkan Engine", nullptr, nullptr);
}

bool WindowsPlatform::WindowShouldClose() const
{
	return glfwWindowShouldClose(Window);
}

void WindowsPlatform::PollEvents() const
{
	// Process window events.
	glfwPollEvents();
}

void WindowsPlatform::Exit()
{
	exit(-1);
}

std::string WindowsPlatform::FileRead(const std::string& Filename)
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

void WindowsPlatform::FileDelete(const std::string& Filename)
{
	check(std::filesystem::remove(Filename), "Failed to remove file...");
}

void WindowsPlatform::FileRename(const std::string& Old, const std::string& New)
{
	check(FileExists(Old), "Renaming file that doesn't exist...");
	std::filesystem::rename(Old, New);
}

bool WindowsPlatform::FileExists(const std::string& Filename)
{
	return std::filesystem::is_regular_file(Filename);
}

uint64 WindowsPlatform::GetLastWriteTime(const std::string& Filename)
{
	const uint64 LastWriteTime = std::filesystem::last_write_time(Filename).time_since_epoch().count();
	return LastWriteTime;
}

void WindowsPlatform::WriteLog(const std::string& InLog)
{
	printf("%s\n", InLog.c_str());
}

static std::string SanitizeFile(const std::string& File)
{
	std::string FileSanitized = File;
	FileSanitized.erase(FileSanitized.begin(), FileSanitized.begin() + FileSanitized.find_last_of('\\') + 1);
	return FileSanitized;
}

void WindowsPlatform::WriteLog(const std::string& File, const std::string& Func, int32 Line, const std::string& InLog)
{
	std::string Header = "[Debug] [" + SanitizeFile(File) + ":" + Func + ":" + std::to_string(Line) + "]\n";
	std::string Recent = Header + InLog + '\n';
	std::cerr << Recent;
}

void WindowsPlatform::WriteLog(const std::string& Expression, const std::string& File, const std::string& Func, int32 Line, const std::string& InLog)
{
	std::string Header = "[Warning] [" + SanitizeFile(File) + ":" + Func + ":" + std::to_string(Line) + ":" + Expression + "]\n";
	std::string Recent = Header + InLog + '\n';
	std::cerr << Recent;
}

std::string WindowsPlatform::FormatString(std::string Format, ...)
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

void WindowsPlatform::ForkProcess(const std::string& ExePath, const std::string& CmdArgs)
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

	for (uint32 i = 0; i < ARRAY_SIZE(Pipes); i++)
	{
		CloseHandle(Pipes[i]);
	}
}

void WindowsPlatform::Memcpy(void* Dst, const void* Src, size_t Size)
{
	memcpy(Dst, Src, Size);
}

#undef LoadImage
uint8* WindowsPlatform::LoadImage(const std::string& Filename, int32& Width, int32& Height, int32& NumChannels)
{
	uint8* Image = stbi_load(Filename.c_str(), &Width, &Height, &NumChannels, STBI_rgb_alpha);
	check(Image, "%s failed to load", Filename.c_str());
	return Image;
}

void WindowsPlatform::FreeImage(uint8* Pixels)
{
	stbi_image_free(Pixels);
}

static const std::string ConfigPath = "../Config/";

bool WindowsPlatform::GetBool(const std::string& Filename, const std::string& Section, const std::string& Key, bool Default)
{
	const std::string DefaultString = Default ? "true" : "false";
	
	std::string ReturnedString = GetString(Filename, Section, Key, DefaultString);

	std::transform(ReturnedString.begin(), ReturnedString.end(), ReturnedString.begin(), [] (char& c) { return std::tolower(c); });

	return ReturnedString == "true" ? true : false;
}

int32 WindowsPlatform::GetInt(const std::string& Filename, const std::string& Section, const std::string& Key, int32 Default)
{
	const std::string Path = ConfigPath + Filename;

	return GetPrivateProfileIntA(Section.c_str(), Key.c_str(), Default, Path.c_str());
}

float64 WindowsPlatform::GetFloat64(const std::string& Filename, const std::string& Section, const std::string& Key, float Default)
{
	std::string FloatStr = GetString(Filename, Section, Key, std::to_string(Default));
	return std::atof(FloatStr.c_str());
}

std::string WindowsPlatform::GetString(const std::string& Filename, const std::string& Section, const std::string& Key, const std::string& Default)
{
	const std::string Path = ConfigPath + Filename;

	std::array<char, 256> ReturnedString;

	GetPrivateProfileStringA(Section.c_str(), Key.c_str(), Default.c_str(), ReturnedString.data(), ReturnedString.size(), Path.c_str());

	return std::string(ReturnedString.data());
}

EMBReturn WindowsPlatform::DisplayMessageBox(EMBType Type, EMBIcon Icon, const std::string& Text, const std::string& Caption, EMBModality Modality)
{
	static const HashTable<EMBType, UINT> WindowsMBType =
	{
		ENTRY(EMBType::ABORTRETRYIGNORE, MB_ABORTRETRYIGNORE)
		ENTRY(EMBType::CANCELTRYCONTINUE, MB_CANCELTRYCONTINUE)
		ENTRY(EMBType::HELP, MB_HELP)
		ENTRY(EMBType::OK, MB_OK)
		ENTRY(EMBType::OKCANCEL, MB_OKCANCEL)
		ENTRY(EMBType::RETRYCANCEL, MB_RETRYCANCEL)
		ENTRY(EMBType::RETRYCANCEL, MB_RETRYCANCEL)
		ENTRY(EMBType::YESNO, MB_YESNO)
		ENTRY(EMBType::YESNOCANCEL, MB_YESNOCANCEL)
	};

	static const HashTable<EMBIcon, UINT> WindowsMBIcon =
	{
		ENTRY(EMBIcon::EXCLAMATION, MB_ICONEXCLAMATION)
		ENTRY(EMBIcon::WARNING, MB_ICONWARNING)
		ENTRY(EMBIcon::INFORMATION, MB_ICONINFORMATION)
		ENTRY(EMBIcon::ASTERISK, MB_ICONASTERISK)
		ENTRY(EMBIcon::QUESTION, MB_ICONQUESTION)
		ENTRY(EMBIcon::STOP, MB_ICONSTOP)
		ENTRY(EMBIcon::FEL, MB_ICONERROR)
		ENTRY(EMBIcon::HAND, MB_ICONHAND)
	};

	static const HashTable<EMBModality, UINT> WindowsMBModality =
	{
		ENTRY(EMBModality::APPLMODAL, MB_APPLMODAL)
		ENTRY(EMBModality::SYSTEMMODAL, MB_SYSTEMMODAL)
		ENTRY(EMBModality::TASKMODAL, MB_TASKMODAL)
	};

	static const HashTable<int32, EMBReturn> WindowsMBReturn =
	{
		ENTRY(IDABORT, EMBReturn::ABORT)
		ENTRY(IDCANCEL, EMBReturn::CANCEL)
		ENTRY(IDCONTINUE, EMBReturn::CONTINUE)
		ENTRY(IDIGNORE, EMBReturn::IGNORERA)
		ENTRY(IDNO, EMBReturn::NO)
		ENTRY(IDOK, EMBReturn::OK)
		ENTRY(IDRETRY, EMBReturn::RETRY)
		ENTRY(IDTRYAGAIN, EMBReturn::TRYAGAIN)
		ENTRY(IDYES, EMBReturn::YES)
	};

	const UINT WindowsMessageBox = WindowsMBType.at(Type) | WindowsMBIcon.at(Icon) | WindowsMBModality.at(Modality);

	const int32 MessageBoxID = MessageBoxA(
		nullptr,
		Text.c_str(),
		Caption.c_str(),
		WindowsMessageBox
	);

	return WindowsMBReturn.at(MessageBoxID);
}

std::filesystem::path WindowsPlatform::DisplayFileExplorer()
{
	const HWND Hwnd = glfwGetWin32Window(Window);

	OPENFILENAME OFN;
	std::array<char, 1024> File;

	ZeroMemory(&OFN, sizeof(OFN));
	OFN.lStructSize = sizeof(OFN);
	OFN.hwndOwner = Hwnd;
	OFN.lpstrFile = File.data();
	OFN.lpstrFile[0] = '\0';
	OFN.nMaxFile = File.size();
	OFN.lpstrFilter = "All\0*.*\0Text\0*.TXT\0";
	OFN.nFilterIndex = 1;
	OFN.lpstrFileTitle = NULL;
	OFN.nMaxFileTitle = 0;
	OFN.lpstrInitialDir = NULL;
	OFN.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

	if (GetOpenFileName(&OFN) == TRUE)
	{
		return { OFN.lpstrFile };
	}
	else
	{
		return {};
	}
}