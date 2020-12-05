#include "Platform.h"

#include <fstream>
#include <cstdarg>
#include <iostream>
#include <algorithm>
#include <cctype>

#include <stb_image.h>

#define WINDOWS_MEAN_AND_LEAN
#include <Windows.h>
#include <shlobj.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

WindowsPlatform::WindowsPlatform(int32 width, int32 height)
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	_Window = glfwCreateWindow(width, height, "Vulkan Engine", nullptr, nullptr);
}

bool WindowsPlatform::WindowShouldClose() const
{
	return glfwWindowShouldClose(_Window);
}

void WindowsPlatform::PollEvents() const
{
	// Process window events.
	glfwPollEvents();
}

void* WindowsPlatform::GetWindow() const
{
	return glfwGetWin32Window(_Window);
}

void WindowsPlatform::Exit()
{
	exit(-1);
}

std::string WindowsPlatform::FileRead(const std::filesystem::path& path, const std::string& prependText)
{
	std::ifstream file(path, std::ios::ate | std::ios::binary);

	check(file.is_open(), "Failed to open file %s", path.c_str());

	size_t fileSize = static_cast<size_t>(file.tellg());
	std::string buffer;
	buffer.resize(fileSize + prependText.size());

	memcpy(buffer.data(), prependText.data(), prependText.size());

	file.seekg(0);
	file.read(buffer.data() + prependText.size(), fileSize);
	file.close();

	return buffer;
}

void WindowsPlatform::FileDelete(const std::string& filename)
{
	check(std::filesystem::remove(filename), "Failed to remove file...");
}

void WindowsPlatform::FileRename(const std::string& oldName, const std::string& newName)
{
	check(FileExists(oldName), "Renaming file that doesn't exist...");
	std::filesystem::rename(oldName, newName);
}

bool WindowsPlatform::FileExists(const std::string& filename)
{
	return std::filesystem::is_regular_file(filename);
}

uint64 WindowsPlatform::GetLastWriteTime(const std::filesystem::path& path)
{
	const uint64 lastWriteTime = std::filesystem::last_write_time(path).time_since_epoch().count();
	return lastWriteTime;
}

void WindowsPlatform::WriteLog(const std::string& log)
{
	printf("%s\n", log.c_str());
}

static std::string SanitizeFile(const std::string& file)
{
	std::string fileSanitized = file;
	fileSanitized.erase(fileSanitized.begin(), fileSanitized.begin() + fileSanitized.find_last_of('\\') + 1);
	return fileSanitized;
}

void WindowsPlatform::WriteLog(const std::string& file, const std::string& func, int32 line, const std::string& log)
{
	std::string header = "[Debug] [" + SanitizeFile(file) + ":" + func + ":" + std::to_string(line) + "]\n";
	std::string recent = header + log + '\n';
	std::cerr << recent;
}

void WindowsPlatform::WriteLog(const std::string& expression, const std::string& file, const std::string& func, int32 line, const std::string& log)
{
	std::string header = "[Warning] [" + SanitizeFile(file) + ":" + func + ":" + std::to_string(line) + ":" + expression + "]\n";
	std::string recent = header + log + '\n';
	std::cerr << recent;
}

std::string WindowsPlatform::FormatString(std::string format, ...)
{
	va_list args, argsCopy;
	va_start(args, format);
	va_copy(argsCopy, args);

	const auto sz = std::vsnprintf(nullptr, 0, format.c_str(), args) + 1;

	try
	{
		std::string result(sz, ' ');
		std::vsnprintf(&result.front(), sz, format.c_str(), argsCopy);
		va_end(argsCopy);
		va_end(args);
		result.erase(std::remove(result.begin(), result.end(), '\0'), result.end());
		return result;
	}
	catch (const std::bad_alloc& e)
	{
		va_end(argsCopy);
		va_end(args);
		fail(e.what());
	}
}

void WindowsPlatform::ForkProcess(const std::string& exePath, const std::string& cmdArgs)
{
	enum { ParentRead, ParentWrite, ChildWrite, ChildRead, NumPipeTypes };

	SECURITY_ATTRIBUTES security;
	security.nLength = sizeof(security);
	security.bInheritHandle = TRUE;
	security.lpSecurityDescriptor = NULL;

	HANDLE pipes[NumPipeTypes];

	if (!CreatePipe(&pipes[ParentWrite], &pipes[ChildRead], &security, 0))
		fail("%d", GetLastError());
	if (!CreatePipe(&pipes[ParentRead], &pipes[ChildWrite], &security, 0))
		fail("%d", GetLastError());
	if (!SetHandleInformation(pipes[ParentRead], HANDLE_FLAG_INHERIT, 0))
		fail("%d", GetLastError());
	if (!SetHandleInformation(pipes[ParentWrite], HANDLE_FLAG_INHERIT, 0))
		fail("%d", GetLastError());

	STARTUPINFO startupInfo;
	ZeroMemory(&startupInfo, sizeof(STARTUPINFO));
	startupInfo.cb = sizeof(STARTUPINFO);
	startupInfo.wShowWindow = SW_SHOW;
	startupInfo.dwFlags = STARTF_USESHOWWINDOW;
	startupInfo.hStdOutput = pipes[ChildWrite];
	startupInfo.hStdError = pipes[ChildWrite];
	startupInfo.hStdInput = pipes[ChildRead];

	PROCESS_INFORMATION processInfo;
	ZeroMemory(&processInfo, sizeof(PROCESS_INFORMATION));

	std::vector<char> exePathArr(exePath.size());
	std::vector<char> cmdArgsArr(cmdArgs.size());

	exePathArr.insert(exePathArr.begin(), exePath.begin(), exePath.end());
	cmdArgsArr.insert(cmdArgsArr.begin(), cmdArgs.begin(), cmdArgs.end());

	if (CreateProcess(exePathArr.data(), cmdArgsArr.data(),
		NULL, NULL, TRUE, 0, NULL,
		NULL, &startupInfo, &processInfo))
	{
		WaitForSingleObject(processInfo.hProcess, INFINITE);
		CloseHandle(processInfo.hThread);
		CloseHandle(processInfo.hProcess);
	}
	else
	{
		fail("The process could not be started...");
	}

	for (uint32 i = 0; i < std::size(pipes); i++)
	{
		CloseHandle(pipes[i]);
	}
}

void WindowsPlatform::Memcpy(void* dst, const void* src, size_t size)
{
	memcpy(dst, src, size);
}

#undef LoadImage
uint8* WindowsPlatform::LoadImage(const std::filesystem::path& path, int32& width, int32& height, int32& numChannels)
{
	uint8* image = stbi_load(path.string().c_str(), &width, &height, &numChannels, STBI_rgb_alpha);
	return image;
}

void WindowsPlatform::FreeImage(uint8* pixels)
{
	stbi_image_free(pixels);
}

static const std::string configPath = "../Config/";

bool WindowsPlatform::GetBool(const std::string& filename, const std::string& section, const std::string& key, bool defaultVal)
{
	const std::string defaultString = defaultVal ? "true" : "false";
	
	std::string returnedString = GetString(filename, section, key, defaultString);

	std::transform(returnedString.begin(), returnedString.end(), returnedString.begin(), [] (char& c) { return std::tolower(c); });

	return returnedString == "true" ? true : false;
}

int32 WindowsPlatform::GetInt(const std::string& filename, const std::string& section, const std::string& key, int32 defaultVal)
{
	const std::string path = configPath + filename;

	return GetPrivateProfileIntA(section.c_str(), key.c_str(), defaultVal, path.c_str());
}

float WindowsPlatform::GetFloat(const std::string& filename, const std::string& section, const std::string& key, float defaultVal)
{
	const float value = static_cast<float>(GetFloat64(filename, section, key, defaultVal));
	return value;
}

float64 WindowsPlatform::GetFloat64(const std::string& filename, const std::string& section, const std::string& key, float defaultVal)
{
	std::string floatStr = GetString(filename, section, key, std::to_string(defaultVal));
	return std::atof(floatStr.c_str());
}

std::string WindowsPlatform::GetString(const std::string& filename, const std::string& section, const std::string& key, const std::string& defaultVal)
{
	const std::string path = configPath + filename;

	std::array<char, 256> returnedString;

	GetPrivateProfileStringA(section.c_str(), key.c_str(), defaultVal.c_str(), returnedString.data(), static_cast<DWORD>(returnedString.size()), path.c_str());

	return std::string(returnedString.data());
}

EMBReturn WindowsPlatform::DisplayMessageBox(EMBType type, EMBIcon icon, const std::string& text, const std::string& caption, EMBModality modality)
{
	static const std::unordered_map<EMBType, UINT> windowsMBType =
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

	static const std::unordered_map<EMBIcon, UINT> windowsMBIcon =
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

	static const std::unordered_map<EMBModality, UINT> windowsMBModality =
	{
		ENTRY(EMBModality::APPLMODAL, MB_APPLMODAL)
		ENTRY(EMBModality::SYSTEMMODAL, MB_SYSTEMMODAL)
		ENTRY(EMBModality::TASKMODAL, MB_TASKMODAL)
	};

	static const std::unordered_map<int32, EMBReturn> windowsMBReturn =
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

	const UINT windowsMessageBox = windowsMBType.at(type) | windowsMBIcon.at(icon) | windowsMBModality.at(modality);

	const int32 messageBoxID = MessageBoxA(
		nullptr,
		text.c_str(),
		caption.c_str(),
		windowsMessageBox
	);

	return windowsMBReturn.at(messageBoxID);
}

std::filesystem::path WindowsPlatform::DisplayFileExplorer()
{
	const HWND hwnd = (HWND)GetWindow();

	OPENFILENAME openFilename;
	std::array<char, 1024> file;

	ZeroMemory(&openFilename, sizeof(openFilename));
	openFilename.lStructSize = sizeof(openFilename);
	openFilename.hwndOwner = hwnd;
	openFilename.lpstrFile = file.data();
	openFilename.lpstrFile[0] = '\0';
	openFilename.nMaxFile = static_cast<DWORD>(file.size());
	openFilename.lpstrFilter = "All\0*.*\0Text\0*.TXT\0";
	openFilename.nFilterIndex = 1;
	openFilename.lpstrFileTitle = NULL;
	openFilename.nMaxFileTitle = 0;
	openFilename.lpstrInitialDir = NULL;
	openFilename.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

	if (GetOpenFileName(&openFilename) == TRUE)
	{
		return { openFilename.lpstrFile };
	}
	else
	{
		return {};
	}
}

void Platform::crc32_u8(Crc& crc, const void* data, std::size_t numBytes)
{
	const uint8* bytes = static_cast<const uint8*>(data);
	for (std::size_t i = 0; i < numBytes; ++i)
	{
		crc = _mm_crc32_u8(crc, bytes[i]);
	}
}

void Platform::crc32_u32(Crc& crc, const void* data, std::size_t numBytes)
{
	const std::size_t numDwords = numBytes / sizeof(uint32);
	const uint32* dwords = static_cast<const uint32*>(data);
	for (std::size_t i = 0; i < numDwords; ++i)
	{
		crc = _mm_crc32_u32(crc, dwords[i]);
	}
}